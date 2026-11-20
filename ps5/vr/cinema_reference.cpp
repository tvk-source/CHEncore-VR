// SPDX-License-Identifier: MIT
#include "cinema_reference.hpp"
#include "cinema_geometry.hpp"
#include "cinema_source.hpp"
#include <volk.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <memory>
#if defined(ENCORE_VR_CINEMA)
#include "ghost/psvr2.h"
#include "probe_retirement.h"
#include "probe_timing.h"
#include "../src/platform.h"
#endif

namespace {
constexpr uint32_t eyeSize=1024;
constexpr uint32_t width=2*eyeSize,height=eyeSize;
constexpr VkDeviceSize bytes=VkDeviceSize(width)*height*4;
struct Image {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};};
void check(VkResult rc,const char* what){
    if(rc!=VK_SUCCESS)throw std::runtime_error(std::string(what)+" rc="+std::to_string(rc));
}
#define CHECK(call) check((call),#call)
struct Resources {
    VkDevice d;VkPhysicalDeviceMemoryProperties memory{};
    Image source{},target{};Buffer vertices{},readback{};
    VkSampler sampler{};VkDescriptorSetLayout setLayout{};VkDescriptorPool descPool{};
    VkDescriptorSet set{};VkPipelineLayout layout{};VkRenderPass pass{};
    VkFramebuffer framebuffer{};VkPipeline pipeline{};VkShaderModule vert{},frag{};
    VkCommandPool cmdPool{};VkCommandBuffer cmd{};VkFence fence{};
    bool pending{};
    Resources(VkPhysicalDevice p,VkDevice dev):d(dev){vkGetPhysicalDeviceMemoryProperties(p,&memory);}
    ~Resources(){
        // Host-only diagnostic. Never destroy resources if completion failed.
        if(pending)return;
        if(fence)vkDestroyFence(d,fence,nullptr);
        if(cmdPool)vkDestroyCommandPool(d,cmdPool,nullptr);
        if(pipeline)vkDestroyPipeline(d,pipeline,nullptr);
        if(framebuffer)vkDestroyFramebuffer(d,framebuffer,nullptr);
        if(pass)vkDestroyRenderPass(d,pass,nullptr);
        if(vert)vkDestroyShaderModule(d,vert,nullptr);
        if(frag)vkDestroyShaderModule(d,frag,nullptr);
        if(layout)vkDestroyPipelineLayout(d,layout,nullptr);
        if(descPool)vkDestroyDescriptorPool(d,descPool,nullptr);
        if(setLayout)vkDestroyDescriptorSetLayout(d,setLayout,nullptr);
        if(sampler)vkDestroySampler(d,sampler,nullptr);
        for(auto* i:{&source,&target}){
            if(i->view)vkDestroyImageView(d,i->view,nullptr);
            if(i->image)vkDestroyImage(d,i->image,nullptr);
            if(i->memory)vkFreeMemory(d,i->memory,nullptr);
        }
        for(auto* b:{&vertices,&readback}){
            if(b->buffer)vkDestroyBuffer(d,b->buffer,nullptr);
            if(b->memory)vkFreeMemory(d,b->memory,nullptr);
        }
    }
    VkDeviceMemory allocate(VkMemoryRequirements req,VkMemoryPropertyFlags flags){
        for(uint32_t i=0;i<memory.memoryTypeCount;++i){
            if(!(req.memoryTypeBits&(1u<<i))||(memory.memoryTypes[i].propertyFlags&flags)!=flags)continue;
            VkMemoryAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize=req.size;ai.memoryTypeIndex=i;
            VkDeviceMemory m{};CHECK(vkAllocateMemory(d,&ai,nullptr,&m));return m;
        }
        throw std::runtime_error("missing compatible memory type");
    }
    void image(Image& i,VkFormat format,VkExtent2D extent,VkImageUsageFlags usage){
        VkImageCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType=VK_IMAGE_TYPE_2D;ci.format=format;ci.extent={extent.width,extent.height,1};
        ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
        ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;ci.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        CHECK(vkCreateImage(d,&ci,nullptr,&i.image));
        VkMemoryRequirements req{};vkGetImageMemoryRequirements(d,i.image,&req);
        i.memory=allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        CHECK(vkBindImageMemory(d,i.image,i.memory,0));
        VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image=i.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=format;
        vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        CHECK(vkCreateImageView(d,&vi,nullptr,&i.view));
    }
    void buffer(Buffer& b,VkDeviceSize size,VkBufferUsageFlags usage){
        VkBufferCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size=size;ci.usage=usage;ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        CHECK(vkCreateBuffer(d,&ci,nullptr,&b.buffer));
        VkMemoryRequirements req{};vkGetBufferMemoryRequirements(d,b.buffer,&req);
        b.memory=allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CHECK(vkBindBufferMemory(d,b.buffer,b.memory,0));
    }
    VkShaderModule shader(const std::string& path){
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        auto n=file.tellg();
        if(n<=0||n>1048576||static_cast<size_t>(n)%4)throw std::runtime_error("invalid shader "+path);
        std::vector<uint32_t> words(static_cast<size_t>(n)/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),n)||words[0]!=0x07230203)
            throw std::runtime_error("bad SPIR-V "+path);
        VkShaderModuleCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize=words.size()*4;ci.pCode=words.data();
        VkShaderModule out{};CHECK(vkCreateShaderModule(d,&ci,nullptr,&out));return out;
    }
};
}

namespace {
using namespace encore::vr;
// Own the GPU resources once; every later frame updates only vertices and
// the private sampled source. No headset buffers are exposed to Vulkan.
struct CinemaRenderer {
    VkDevice device;VkQueue queue;VkExtent2D extent; Resources r;
    bool initialized=false;
    CinemaRenderer(VkPhysicalDevice physical,VkDevice dev,VkQueue q,uint32_t family,VkFormat format,VkExtent2D size,const std::string& shaders):
        device(dev),queue(q),extent(size),r(physical,dev) {
        if(!extent.width||!extent.height||extent.width>8192||extent.height>8192)
            throw std::runtime_error("invalid game image");
        VkFormatProperties src{},dst{};
        vkGetPhysicalDeviceFormatProperties(physical,format,&src);
        vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_R8G8B8A8_UNORM,&dst);
        const VkFormatFeatureFlags srcNeed=VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        const VkFormatFeatureFlags dstNeed=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        if((src.optimalTilingFeatures&srcNeed)!=srcNeed||(dst.optimalTilingFeatures&dstNeed)!=dstNeed)
            throw std::runtime_error("unsupported cinema formats");
        r.image(r.source,format,extent,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
        r.image(r.target,VK_FORMAT_R8G8B8A8_UNORM,{width,height},VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        r.buffer(r.vertices,sizeof(CinemaVertex)*12,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        r.buffer(r.readback,bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.minFilter=si.magFilter=VK_FILTER_NEAREST;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        CHECK(vkCreateSampler(device,&si,nullptr,&r.sampler));
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo sli{};sli.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        sli.bindingCount=1;sli.pBindings=&binding;
        CHECK(vkCreateDescriptorSetLayout(device,&sli,nullptr,&r.setLayout));
        VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1};
        VkDescriptorPoolCreateInfo dpi{};dpi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpi.maxSets=1;dpi.poolSizeCount=1;dpi.pPoolSizes=&poolSize;
        CHECK(vkCreateDescriptorPool(device,&dpi,nullptr,&r.descPool));
        VkDescriptorSetAllocateInfo dai{};dai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dai.descriptorPool=r.descPool;dai.descriptorSetCount=1;dai.pSetLayouts=&r.setLayout;
        CHECK(vkAllocateDescriptorSets(device,&dai,&r.set));
        VkDescriptorImageInfo di{r.sampler,r.source.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{};write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet=r.set;write.descriptorCount=1;write.descriptorType=binding.descriptorType;write.pImageInfo=&di;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
        VkPipelineLayoutCreateInfo li{};li.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        li.setLayoutCount=1;li.pSetLayouts=&r.setLayout;
        CHECK(vkCreatePipelineLayout(device,&li,nullptr,&r.layout));
        VkAttachmentDescription ad{};ad.format=VK_FORMAT_R8G8B8A8_UNORM;ad.samples=VK_SAMPLE_COUNT_1_BIT;
        ad.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;ad.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        ad.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;ad.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ad.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;ad.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference ar{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount=1;sub.pColorAttachments=&ar;
        VkSubpassDependency dep{};dep.srcSubpass=0;dep.dstSubpass=VK_SUBPASS_EXTERNAL;
        dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
        dep.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dep.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        VkRenderPassCreateInfo ri{};ri.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        ri.attachmentCount=1;ri.pAttachments=&ad;ri.subpassCount=1;ri.pSubpasses=&sub;ri.dependencyCount=1;ri.pDependencies=&dep;
        CHECK(vkCreateRenderPass(device,&ri,nullptr,&r.pass));
        VkFramebufferCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fi.renderPass=r.pass;fi.attachmentCount=1;fi.pAttachments=&r.target.view;fi.width=width;fi.height=height;fi.layers=1;
        CHECK(vkCreateFramebuffer(device,&fi,nullptr,&r.framebuffer));
        r.vert=r.shader(shaders+"/cinema.vert.spv");r.frag=r.shader(shaders+"/cinema.frag.spv");
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(int i=0;i<2;++i){stages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stages[i].pName="main";}
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=r.vert;
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=r.frag;
        VkVertexInputBindingDescription vb{0,sizeof(CinemaVertex),VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription va[2]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(CinemaVertex,clip)},
                                               {1,0,VK_FORMAT_R32G32_SFLOAT,offsetof(CinemaVertex,u)}};
        VkPipelineVertexInputStateCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&vb;vi.vertexAttributeDescriptionCount=2;vi.pVertexAttributeDescriptions=va;
        VkPipelineInputAssemblyStateCreateInfo ia{};ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vp{};vp.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{};rs.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1.f;
        VkPipelineMultisampleStateCreateInfo ms{};ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;
        VkPipelineColorBlendStateCreateInfo bs{};bs.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;bs.attachmentCount=1;bs.pAttachments=&ba;
        VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo ds{};ds.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;ds.dynamicStateCount=2;ds.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;
        pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pColorBlendState=&bs;pi.pDynamicState=&ds;pi.layout=r.layout;pi.renderPass=r.pass;
        CHECK(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&r.pipeline));
        VkCommandPoolCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;ci.queueFamilyIndex=family;
        CHECK(vkCreateCommandPool(device,&ci,nullptr,&r.cmdPool));
        VkCommandBufferAllocateInfo cai{};cai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cai.commandPool=r.cmdPool;cai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cai.commandBufferCount=1;
        CHECK(vkAllocateCommandBuffers(device,&cai,&r.cmd));
        VkFenceCreateInfo fc{};fc.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;CHECK(vkCreateFence(device,&fc,nullptr,&r.fence));

    }
    void render(VkImage original,VkImageLayout sourceLayout,const CinemaScreen& screen,const Eye (&eyes)[2]){
        std::array<CinemaVertex,12> verts{};
        for(int i=0;i<2;++i){
            auto v=cinemaVertices(screen,eyes[i]);if(!v)throw std::runtime_error("bad eye projection");
            std::copy(v->begin(),v->end(),verts.begin()+6*i);
        }
        void* mapped=nullptr;
        CHECK(vkMapMemory(device,r.vertices.memory,0,sizeof verts,0,&mapped));
        std::memcpy(mapped,verts.data(),sizeof verts);vkUnmapMemory(device,r.vertices.memory);
        VkCommandBufferBeginInfo begin{};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CHECK(vkResetCommandPool(device,r.cmdPool,0));
        CHECK(vkResetFences(device,1,&r.fence));
        CHECK(vkBeginCommandBuffer(r.cmd,&begin));
        if(!recordCinemaSourceCopy(r.cmd,original,r.source.image,extent,sourceLayout,
             initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,vkCmdPipelineBarrier,vkCmdCopyImage))throw std::runtime_error("source copy rejected");
        VkClearValue clear{};clear.color.float32[0]=clear.color.float32[1]=clear.color.float32[2]=.03f;clear.color.float32[3]=1;
        VkRenderPassBeginInfo rb{};rb.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;rb.renderPass=r.pass;rb.framebuffer=r.framebuffer;
        rb.renderArea={{0,0},{width,height}};rb.clearValueCount=1;rb.pClearValues=&clear;
        vkCmdBeginRenderPass(r.cmd,&rb,VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(r.cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r.pipeline);
        vkCmdBindDescriptorSets(r.cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r.layout,0,1,&r.set,0,nullptr);
        VkDeviceSize offset=0;vkCmdBindVertexBuffers(r.cmd,0,1,&r.vertices.buffer,&offset);
        for(uint32_t i=0;i<2;++i){
            VkViewport viewport{float(i*eyeSize),0,float(eyeSize),float(eyeSize),0,1};
            VkRect2D scissor{{int32_t(i*eyeSize),0},{eyeSize,eyeSize}};
            vkCmdSetViewport(r.cmd,0,1,&viewport);vkCmdSetScissor(r.cmd,0,1,&scissor);
            vkCmdDraw(r.cmd,6,1,i*6,0);
        }
        vkCmdEndRenderPass(r.cmd);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};
        vkCmdCopyImageToBuffer(r.cmd,r.target.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r.readback.buffer,1,&copy);
        VkBufferMemoryBarrier bb{};bb.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bb.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;bb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        bb.srcQueueFamilyIndex=bb.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;bb.buffer=r.readback.buffer;bb.size=VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(r.cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&bb,0,nullptr);
        CHECK(vkEndCommandBuffer(r.cmd));
        VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&r.cmd;
        CHECK(vkQueueSubmit(queue,1,&submit,r.fence));r.pending=true;
        // This reference runs once, outside interactive VR. Bounded wait;
        // if it fails, do not continue normal presentation of the source.
        VkResult waited=vkWaitForFences(device,1,&r.fence,VK_TRUE,5000000000ull);
        if(waited!=VK_SUCCESS){
            std::fprintf(stderr,"CINEMA_REFERENCE fatal fence rc=%d\n",int(waited));
            std::fflush(stderr);
#if defined(ENCORE_VR_CINEMA)
            psvr2_shutdown();
            for(;;)platform_sleep_us(100000);
#else
            std::_Exit(2);
#endif
        }
        r.pending=false;initialized=true;

    }
    void copy(void* dst){
        if(!initialized||!dst)throw std::runtime_error("no completed cinema frame");
        void* mapped=nullptr;CHECK(vkMapMemory(device,r.readback.memory,0,bytes,0,&mapped));
        std::memcpy(dst,mapped,bytes);vkUnmapMemory(device,r.readback.memory);
    }
};
}
bool encoreCinemaReference(VkPhysicalDevice physical,VkDevice device,VkQueue queue,
    uint32_t family,VkImage original,VkFormat format,VkExtent2D extent,
    const std::string& shaders,const std::string& output)
{
    try {
        CinemaRenderer renderer(physical,device,queue,family,format,extent,shaders);
        Pose head{{0,1.2f,0},{0,0,0,1},true};
        auto screen=cinemaScreen(head,2.4f,float(extent.width)/float(extent.height),2.5f);
        if(!screen)throw std::runtime_error("invalid reference geometry");
        Eye eyes[2]={{{{-.0295f,1.2f,0},{},true},{1.8418f,.9472f,1.329f,1.329f}},
                     {{{ .0295f,1.2f,0},{},true},{.9472f,1.8418f,1.329f,1.329f}}};
        // Exercise the persistent resource path as well as first use.
        // Same source is restored each time; the sampled image is reused.
        for(unsigned frame=0;frame<3;++frame)
            renderer.render(original,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,*screen,eyes);
        std::fprintf(stderr,"CINEMA_REFERENCE persistent_frames=3\n");
        std::vector<uint8_t> rgba(bytes),rgb(size_t(width)*height*3);
        renderer.copy(rgba.data());
        const auto* p=rgba.data();
        size_t nonbackground[2]{};
        for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
            size_t i=size_t(y)*width+x;
            for(int c=0;c<3;++c)rgb[3*i+c]=p[4*i+c];
            if(p[4*i]!=8||p[4*i+1]!=8||p[4*i+2]!=8)++nonbackground[x/eyeSize];
        }


        if(nonbackground[0]<100||nonbackground[1]<100)throw std::runtime_error("empty stereo reference");
        std::ofstream file(output,std::ios::binary);
        file<<"P6\n"<<width<<" "<<height<<"\n255\n";
        file.write(reinterpret_cast<const char*>(rgb.data()),rgb.size());
        if(!file)throw std::runtime_error("cannot write "+output);
        std::fprintf(stderr,"CINEMA_REFERENCE PASS source=%ux%u stereo=%ux%u nonbackground=%zu/%zu output=%s\n",
            extent.width,extent.height,width,height,nonbackground[0],nonbackground[1],output.c_str());
        return true;
    } catch(const std::exception& e){
        std::fprintf(stderr,"CINEMA_REFERENCE FAIL %s\n",e.what());return false;
    }
}
#undef CHECK

#if defined(ENCORE_VR_CINEMA)
namespace {
struct NativeCinema {
    std::unique_ptr<CinemaRenderer> renderer;
    void* buffers[ENCORE_LIVE_SLOTS]{};
    encore_live_slot slots[ENCORE_LIVE_SLOTS]{};
    psvr2_info optics{};
    std::optional<CinemaScreen> screen;
    uint64_t frame=0,lastShown=0,start=0;
    bool initialized=false,stopped=false;
} native;
Pose convert(const psvr2_pose& p){return {{p.pos[0],p.pos[1],p.pos[2]},
    {p.quat[0],p.quat[1],p.quat[2],p.quat[3]},true};}
}
void encoreCinemaStop(){
    if(native.stopped)return;
    native.stopped=true;
    psvr2_shutdown();
    native.renderer.reset();
    say("VR_CINEMA stopped after %llu frames; registered direct memory retained until title closure",
        (unsigned long long)native.frame);
}
bool encoreCinemaNativeFrame(VkPhysicalDevice physical,VkDevice device,VkQueue queue,
    uint32_t family,VkImage image,VkFormat format,VkExtent2D extent,const std::string& shaders){
    if(native.stopped)return false;
    uint64_t pending=0;
    try{
        if(!native.initialized){
            native.renderer=std::make_unique<CinemaRenderer>(physical,device,queue,family,format,extent,shaders);
            // Game Vulkan/AGC is initialized before Hmd2, as in V12.
            check(vkQueueWaitIdle(queue),"game queue idle before Hmd2");
            psvr2_set_dualsense(0);psvr2_set_mirror(0);
            if(psvr2_init_output()!=0)throw std::runtime_error("Hmd2 init");
            psvr2_get_info(&native.optics);
            if(native.optics.status||!psvr2_worn())throw std::runtime_error("headset not ready/worn");
            for(float x:native.optics.fov)if(!std::isfinite(x)||x<=0)throw std::runtime_error("invalid FOV");
            for(auto& b:native.buffers){b=psvr2_cinema_alloc(size_t(bytes));if(!b)throw std::runtime_error("direct buffer allocation");}
            if(psvr2_output_start(eyeSize,eyeSize,native.buffers,ENCORE_LIVE_SLOTS,PSVR2_MODE_120_HALF)!=0)
                throw std::runtime_error("VR output start");
            say("VR_CINEMA settling tracking; look forward and keep head still");
            const uint64_t begin=psvr2_now_us();encore_origin_gate gate{};
            while(psvr2_now_us()-begin<10000000ull&&psvr2_worn()&&!psvr2_exit_requested()){
                psvr2_pose head{};
                if(psvr2_head(psvr2_now_us(),&head,nullptr,nullptr,nullptr)&&
                   encore_origin_sample(&gate,psvr2_now_us()-begin,head.pos,head.quat)){
                    native.screen=cinemaScreen(convert(head),3.6f,float(extent.width)/extent.height,2.f);break;
                }
                platform_sleep_us(10000);
            }
            if(!native.screen)throw std::runtime_error("tracking origin did not settle");
            native.start=psvr2_now_us();native.initialized=true;
            say("VR_CINEMA ready 1080p game / 1024x1024 per eye, 60 FPS / 120 Hz output; DualSense or guitar/drums, hold Sense menu to recenter");
        }
        if(!psvr2_worn()||psvr2_exit_requested()||psvr2_now_us()-native.start>=300000000ull)
            throw std::runtime_error("wear/exit/5-minute alpha limit");
        if(extent.width!=native.renderer->extent.width||extent.height!=native.renderer->extent.height)
            throw std::runtime_error("game extent changed; restart title");
        const uint64_t id=native.frame+1;uint64_t display=0;
        psvr2_stats before{},after{};psvr2_get_stats(&before);
        if(psvr2_begin_frame(id,&display)!=0)throw std::runtime_error("begin frame");
        pending=id;psvr2_get_stats(&after);
        if(after.errors!=before.errors||after.queue_full!=before.queue_full||after.repeated!=before.repeated)
            throw std::runtime_error("unexpected reprojection queue activity");
        psvr2_pose head{},poses[2]{};
        if(!psvr2_head(display,&head,poses,nullptr,nullptr)||!validTrackedPose(convert(head)))
            throw std::runtime_error("tracking lost");
        psvr2_poll();
        if(psvr2_take_recenter()){
            auto screen=cinemaScreen(convert(head),3.6f,float(extent.width)/extent.height,2.f);
            if(screen)native.screen=screen;
        }
        const auto& f=native.optics.fov;
        Eye eyes[2]={{convert(poses[0]),{f[0],f[1],f[2],f[3]}},
                     {convert(poses[1]),{f[1],f[0],f[2],f[3]}}};
        uint64_t shown=0;int slot=1;
        if(native.frame){
            int rc=psvr2_cinema_flip(&shown);
            if(shown<native.lastShown)throw std::runtime_error("regressed flip marker");
            slot=encore_live_choose(native.slots,shown,native.frame,psvr2_now_us(),rc);
            if(slot<0)throw std::runtime_error("pre-render buffer gate");
        }
        const uint64_t gpu=psvr2_now_us();
        native.renderer->render(image,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,*native.screen,eyes);
        psvr2_note_gpu(psvr2_now_us()-gpu);
        if(native.frame){
            uint64_t latest=0;int rc=psvr2_cinema_flip(&latest);
            if(latest<shown||!encore_live_eligible(&native.slots[slot],latest,native.frame,psvr2_now_us(),rc))
                throw std::runtime_error("post-fence buffer gate");
            native.lastShown=latest;
        }
        if(!psvr2_worn()||psvr2_exit_requested())throw std::runtime_error("headset removed");
        // No public retirement fence: fail closed and never reuse this storage after shutdown.
        native.renderer->copy(native.buffers[slot]);
        const float left[4]={f[0],f[1],f[2],f[3]},right[4]={f[1],f[0],f[2],f[3]};
        int rc=psvr2_submit(slot,id,display,&head,left,right);pending=0;
        if(rc)throw std::runtime_error("submit failed");
        native.slots[slot]={id,display};native.frame=id;
        if(id<=4||id%300==0)say("VR_CINEMA frame=%llu slot=%d shown=%llu compositor_us=%llu",
            (unsigned long long)id,slot,(unsigned long long)native.lastShown,(unsigned long long)(psvr2_now_us()-gpu));
        return true;
    }catch(const std::exception& e){
        if(pending)psvr2_repeat_frame(pending);
        say("VR_CINEMA STOP %s",e.what());encoreCinemaStop();return false;
    }
}
#endif
