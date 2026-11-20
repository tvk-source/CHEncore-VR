// SPDX-License-Identifier: MIT
#pragma once
#include <vulkan/vulkan.h>
namespace encore::vr {
// Record a same-format, same-size copy of the completed original game image
// into a *private* sampled texture. No presentation or headset memory here.
// Requirements: same queue family, single-sample 2D color images, source
// TRANSFER_SRC usage, destination TRANSFER_DST|SAMPLED usage. Caller owns
// both allocations and has acquired the source and waited for all previous
// destination samples; source render completion must precede this operation.
// Pass UNDEFINED for a fresh destination, SHADER_READ_ONLY_OPTIMAL for one
// previously used by the fragment shader. Caller guarantees equal formats
// and exact extents. Source is restored for the original game's next step.
inline bool recordCinemaSourceCopy(VkCommandBuffer cmd,VkImage source,VkImage dest,
    VkExtent2D extent,VkImageLayout sourceLayout,VkImageLayout destLayout,
    PFN_vkCmdPipelineBarrier barrier,PFN_vkCmdCopyImage copy)
{
    if(!cmd||!source||!dest||source==dest||!extent.width||!extent.height||
       extent.width>8192||extent.height>8192||!barrier||!copy)return false;
    if(sourceLayout!=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR&&
       sourceLayout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)return false;
    if(destLayout!=VK_IMAGE_LAYOUT_UNDEFINED&&
       destLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)return false;
    auto imageBarrier=[](VkImage image,VkImageLayout oldLayout,VkImageLayout newLayout,
                         VkAccessFlags src,VkAccessFlags dst){
        VkImageMemoryBarrier b{};
        b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask=src;b.dstAccessMask=dst;
        b.oldLayout=oldLayout;b.newLayout=newLayout;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        b.image=image;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        return b;
    };
    VkImageMemoryBarrier before[2]={
        imageBarrier(source,sourceLayout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT),
        imageBarrier(dest,destLayout,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     destLayout==VK_IMAGE_LAYOUT_UNDEFINED?VkAccessFlags(0):VkAccessFlags(VK_ACCESS_SHADER_READ_BIT),
                     VK_ACCESS_TRANSFER_WRITE_BIT)
    };
    barrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,before);
    VkImageCopy region{};
    region.srcSubresource=region.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
    region.extent={extent.width,extent.height,1};
    copy(cmd,source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
         dest,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
    VkImageMemoryBarrier after[2]={
        imageBarrier(source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,sourceLayout,
                     VK_ACCESS_TRANSFER_READ_BIT,sourceLayout==VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?
                     VkAccessFlags(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT):VkAccessFlags(0)),
        imageBarrier(dest,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT)
    };
    barrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,0,nullptr,0,nullptr,2,after);
    return true;
}
}
