// SPDX-License-Identifier: MIT
#pragma once
#include <vulkan/vulkan.h>
#include <string>
// Host-only reference. Source is acquired, completed, same queue family,
// single-sample, and has TRANSFER_SRC usage. It is restored to PRESENT_SRC.
// Generates an actual GPU-rendered SBS reference from the original game.
bool encoreCinemaReference(VkPhysicalDevice physical,VkDevice device,VkQueue queue,
    uint32_t family,VkImage source,VkFormat format,VkExtent2D extent,
    const std::string& shaders,const std::string& output);
