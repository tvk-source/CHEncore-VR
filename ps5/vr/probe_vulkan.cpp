// SPDX-License-Identifier: GPL-3.0-or-later
#include "probe_vulkan.h"
#include "probe_raster.hpp"
#include "vulkan_linear.hpp"
#include "../src/platform.h"
#include <volk.h>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance,const char*);
namespace {
struct EyeImage {
    VkImage image{};
    VkDeviceMemory memory{};
    VkImageView view{};
    VkFramebuffer framebuffer{};
};
struct State {
    VkInstance instance{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t family{};
    VkPhysicalDeviceMemoryProperties memory{};
    VkRenderPass pass{};
    EyeImage eye[2]{};
    VkBuffer readback{};
    VkDeviceMemory readbackMemory{};
    VkCommandPool pool{};
    VkCommandBuffer cb{};
    VkFence fence{};
    void* mapped{};
    bool pending{};
} g;
bool check(VkResult r,const char* name) {
    if(r==VK_SUCCESS)return true;
    say("VR_PROBE GPU failure %s rc=%d",name,int(r));
    return false;
}
#define GPU_CHECK(call) do { if(!check((call),#call)) return -1; } while(0)

bool allocate(VkMemoryRequirements req,VkMemoryPropertyFlags flags,VkDeviceMemory* out) {
    for(uint32_t i=0;i<g.memory.memoryTypeCount;++i) {
        if((req.memoryTypeBits&(1u<<i)) && (g.memory.memoryTypes[i].propertyFlags&flags)==flags) {
            VkMemoryAllocateInfo a{};
            a.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            a.allocationSize=req.size; a.memoryTypeIndex=i;
            return check(vkAllocateMemory(g.device,&a,nullptr,out),"vkAllocateMemory");
        }
    }
    say("VR_PROBE GPU no compatible memory type flags=0x%x",unsigned(flags));
    return false;
}
int createEye(EyeImage& e,unsigned size) {
    VkImageCreateInfo ii{};
    ii.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType=VK_IMAGE_TYPE_2D; ii.format=VK_FORMAT_R8G8B8A8_UNORM;
    ii.extent={size,size,1}; ii.mipLevels=1; ii.arrayLayers=1;
    ii.samples=VK_SAMPLE_COUNT_1_BIT; ii.tiling=VK_IMAGE_TILING_OPTIMAL;
    ii.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ii.sharingMode=VK_SHARING_MODE_EXCLUSIVE; ii.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
    GPU_CHECK(vkCreateImage(g.device,&ii,nullptr,&e.image));
    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(g.device,e.image,&req);
    if(!allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,&e.memory))return -1;
    GPU_CHECK(vkBindImageMemory(g.device,e.image,e.memory,0));
    VkImageViewCreateInfo vi{};
    vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image=e.image; vi.viewType=VK_IMAGE_VIEW_TYPE_2D; vi.format=ii.format;
    vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    GPU_CHECK(vkCreateImageView(g.device,&vi,nullptr,&e.view));
    VkFramebufferCreateInfo fi{};
    fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fi.renderPass=g.pass; fi.attachmentCount=1; fi.pAttachments=&e.view;
    fi.width=size;fi.height=size;fi.layers=1;
    GPU_CHECK(vkCreateFramebuffer(g.device,&fi,nullptr,&e.framebuffer));
    return 0;
}
}
extern "C" int encore_probe_gpu_init(void) {
    if(g.instance||g.device)return -1;
    say("VR_PROBE GPU init: native RADV before Hmd2, no surface");
    volkInitializeCustom(vk_icdGetInstanceProcAddr);
    VkApplicationInfo ai{};
    ai.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName="Encore VR optical GPU probe"; ai.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{};
    ci.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ci.pApplicationInfo=&ai;
    GPU_CHECK(vkCreateInstance(&ci,nullptr,&g.instance));
    volkLoadInstance(g.instance);
    uint32_t n=0;
    GPU_CHECK(vkEnumeratePhysicalDevices(g.instance,&n,nullptr));
    if(!n || n>32)return -1;
    std::vector<VkPhysicalDevice> devices(n);
    GPU_CHECK(vkEnumeratePhysicalDevices(g.instance,&n,devices.data()));
    for(uint32_t i=0;i<n&&!g.physical;++i) {
        auto p=devices[i];
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(p,&props);
        if(props.vendorID!=0x1002 || props.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
        VkFormatProperties fmt{};
        vkGetPhysicalDeviceFormatProperties(p,VK_FORMAT_R8G8B8A8_UNORM,&fmt);
        const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        if((fmt.optimalTilingFeatures&required)!=required)continue;
        uint32_t qn=0;
        vkGetPhysicalDeviceQueueFamilyProperties(p,&qn,nullptr);
        if(!qn||qn>64)continue;
        std::vector<VkQueueFamilyProperties> qs(qn);
        vkGetPhysicalDeviceQueueFamilyProperties(p,&qn,qs.data());
        for(uint32_t q=0;q<qn;++q)if(qs[q].queueCount&&(qs[q].queueFlags&VK_QUEUE_GRAPHICS_BIT)){
            g.physical=p;g.family=q;
            say("VR_PROBE GPU device=%s vendor=0x%x graphics_family=%u",props.deviceName,unsigned(props.vendorID),q);
            break;
        }
    }
    if(!g.physical){say("VR_PROBE GPU no compatible AMD graphics device");return -1;}
    vkGetPhysicalDeviceMemoryProperties(g.physical,&g.memory);
    float priority=1.f;
    VkDeviceQueueCreateInfo qi{};
    qi.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qi.queueFamilyIndex=g.family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{};
    di.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
    GPU_CHECK(vkCreateDevice(g.physical,&di,nullptr,&g.device));
    volkLoadDevice(g.device);
    vkGetDeviceQueue(g.device,g.family,0,&g.queue);
    if(!g.queue)return -1;
    say("VR_PROBE GPU init PASS");
    return 0;
}
extern "C" int encore_probe_gpu_render(const encore_probe_eye eyes[2],unsigned size,void *const *buffers,int count) {
    if(!g.device||g.pass||!eyes||!buffers||size!=1024||count!=3)return -1;
    for(int i=0;i<count;++i)if(!buffers[i])return -1;
    const VkDeviceSize bytes=VkDeviceSize(size)*size*2*4;
    VkAttachmentDescription ad{};
    ad.format=VK_FORMAT_R8G8B8A8_UNORM;ad.samples=VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;ad.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    ad.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;ad.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ad.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;ad.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference ar{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ar;
    VkSubpassDependency dep{};
    dep.srcSubpass=0;dep.dstSubpass=VK_SUBPASS_EXTERNAL;
    dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
    dep.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dep.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo pi{};
    pi.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pi.attachmentCount=1;pi.pAttachments=&ad;pi.subpassCount=1;pi.pSubpasses=&sub;
    pi.dependencyCount=1;pi.pDependencies=&dep;
    GPU_CHECK(vkCreateRenderPass(g.device,&pi,nullptr,&g.pass));
    for(auto& e:g.eye)if(createEye(e,size)!=0)return -1;
    VkBufferCreateInfo bi{};
    bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;bi.size=bytes;
    bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    GPU_CHECK(vkCreateBuffer(g.device,&bi,nullptr,&g.readback));
    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(g.device,g.readback,&req);
    if(!allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,&g.readbackMemory))return -1;
    GPU_CHECK(vkBindBufferMemory(g.device,g.readback,g.readbackMemory,0));
    GPU_CHECK(vkMapMemory(g.device,g.readbackMemory,0,bytes,0,&g.mapped));
    VkCommandPoolCreateInfo pci{};
    pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.queueFamilyIndex=g.family;
    GPU_CHECK(vkCreateCommandPool(g.device,&pci,nullptr,&g.pool));
    VkCommandBufferAllocateInfo cai{};
    cai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;cai.commandPool=g.pool;
    cai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cai.commandBufferCount=1;
    GPU_CHECK(vkAllocateCommandBuffers(g.device,&cai,&g.cb));
    VkFenceCreateInfo fi{};fi.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    GPU_CHECK(vkCreateFence(g.device,&fi,nullptr,&g.fence));
    VkCommandBufferBeginInfo begin{};
    begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    GPU_CHECK(vkBeginCommandBuffer(g.cb,&begin));
    for(int s=0;s<2;++s) {
        auto runs=encore::vr::probeRuns(eyes[s],size);
        if(runs.empty()||runs.size()>size_t(size)*size)return -1;
        VkClearValue bg{};
        bg.color.float32[0]=bg.color.float32[1]=bg.color.float32[2]=32.f/255.f;
        bg.color.float32[3]=1.f;
        VkRenderPassBeginInfo r{};
        r.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;r.renderPass=g.pass;r.framebuffer=g.eye[s].framebuffer;
        r.renderArea={{0,0},{size,size}};r.clearValueCount=1;r.pClearValues=&bg;
        vkCmdBeginRenderPass(g.cb,&r,VK_SUBPASS_CONTENTS_INLINE);
        VkClearAttachment a{};a.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        a.clearValue.color.float32[0]=a.clearValue.color.float32[1]=a.clearValue.color.float32[2]=128.f/255.f;
        a.clearValue.color.float32[3]=1.f;
        // Actual GPU color-attachment operations, batched in small groups.
        VkClearRect rects[64]{};
        uint32_t n=0;
        for(const auto& run:runs) {
            rects[n++]={{{int32_t(run.x),int32_t(run.y)},{run.width,1}},0,1};
            if(n==64){vkCmdClearAttachments(g.cb,1,&a,n,rects);n=0;}
        }
        if(n)vkCmdClearAttachments(g.cb,1,&a,n,rects);
        vkCmdEndRenderPass(g.cb);
        say("VR_PROBE GPU eye=%d raster_runs=%u",s,unsigned(runs.size()));
    }
    if(!encore::vr::copyEyes(g.cb,g.eye[0].image,g.eye[1].image,g.readback,size,size,
                            vkCmdCopyImageToBuffer,vkCmdPipelineBarrier))return -1;
    GPU_CHECK(vkEndCommandBuffer(g.cb));
    VkSubmitInfo submit{};
    submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&g.cb;
    g.pending=true;
    if(!check(vkQueueSubmit(g.queue,1,&submit,g.fence),"vkQueueSubmit"))return -1;
    if(!check(vkWaitForFences(g.device,1,&g.fence,VK_TRUE,5000000000ull),"vkWaitForFences"))return -1;
    g.pending=false;
    const auto* pixels=static_cast<const uint32_t*>(g.mapped);
    size_t bad=0,lit[2]{};
    for(unsigned y=0;y<size;++y)for(unsigned x=0;x<2*size;++x) {
        unsigned side=x/size;
        uint32_t expected=encore_probe_scene_pixel(&eyes[side],(float(x%size)+.5f)/float(size),(float(y)+.5f)/float(size));
        uint32_t actual=pixels[size_t(y)*2*size+x];
        if(actual!=expected)++bad;
        if(actual==UINT32_C(0xff808080))++lit[side];
    }
    say("VR_PROBE GPU readback pixels=%u mismatches=%u litL=%u litR=%u",unsigned(bytes/4),unsigned(bad),unsigned(lit[0]),unsigned(lit[1]));
    if(bad||!lit[0]||!lit[1])return -1;
    for(int i=0;i<count;++i)std::memcpy(buffers[i],g.mapped,size_t(bytes));
    say("VR_PROBE GPU raster/copy/fence/readback PASS; immutable headset buffers ready");
    return 0;
}
extern "C" void encore_probe_gpu_stop(void) {
    if(g.pending){say("VR_PROBE GPU fence not confirmed; retain resources until title closure");return;}
    if(g.device) {
        if(g.fence)vkDestroyFence(g.device,g.fence,nullptr);
        if(g.pool)vkDestroyCommandPool(g.device,g.pool,nullptr);
        if(g.mapped)vkUnmapMemory(g.device,g.readbackMemory);
        if(g.readback)vkDestroyBuffer(g.device,g.readback,nullptr);
        if(g.readbackMemory)vkFreeMemory(g.device,g.readbackMemory,nullptr);
        for(auto& e:g.eye) {
            if(e.framebuffer)vkDestroyFramebuffer(g.device,e.framebuffer,nullptr);
            if(e.view)vkDestroyImageView(g.device,e.view,nullptr);
            if(e.image)vkDestroyImage(g.device,e.image,nullptr);
            if(e.memory)vkFreeMemory(g.device,e.memory,nullptr);
        }
        if(g.pass)vkDestroyRenderPass(g.device,g.pass,nullptr);
        vkDestroyDevice(g.device,nullptr);
    }
    if(g.instance)vkDestroyInstance(g.instance,nullptr);
    g={};
    say("VR_PROBE GPU shutdown complete");
}
#undef GPU_CHECK
