// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
namespace encore::vr {
// Copy two RGBA8 single-sample eye images into one linear side-by-side buffer.
// Source images must already be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and the
// buffer allocated HOST_VISIBLE|HOST_COHERENT (prefer DEVICE_LOCAL) with
// TRANSFER_DST usage. Use the same device/queue as the game. Caller records
// image transitions and waits for its submission's fence before Hmd2 submit.
// vkGetBufferDeviceAddress is NOT the pointer expected by this GhostVR path.
inline bool copyEyes(VkCommandBuffer cb,VkImage left,VkImage right,VkBuffer dst,
                      uint32_t eyeW,uint32_t eyeH,PFN_vkCmdCopyImageToBuffer copy,
                      PFN_vkCmdPipelineBarrier barrier){
 if(!cb||!left||!right||!dst||!eyeW||!eyeH||eyeW>8192||eyeH>8192||!copy||!barrier)return false;
 VkImage images[2]={left,right};
 for(uint32_t e=0;e<2;++e){
    VkBufferImageCopy region{};region.bufferOffset=VkDeviceSize(e)*eyeW*4;
    region.bufferRowLength=2*eyeW;region.bufferImageHeight=eyeH;
    region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.imageExtent={eyeW,eyeH,1};
    copy(cb,images[e],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,dst,1,&region);
 }
 VkBufferMemoryBarrier b{};b.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
 b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_HOST_READ_BIT|VK_ACCESS_MEMORY_READ_BIT;
 b.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.buffer=dst;b.size=VK_WHOLE_SIZE;
 barrier(cb,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT|VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,1,&b,0,nullptr);
 return true;
}
}
