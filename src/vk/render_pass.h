#pragma once

#include <vulkan/vulkan.hpp>

namespace nvvk
{

/**
  # functions in nvvk

  - findSupportedFormat : returns supported VkFormat from a list of candidates (returns first match)
  - findDepthFormat : returns supported depth format (24, 32, 16-bit)
  - findDepthStencilFormat : returns supported depth-stencil format (24/8, 32/8, 16/8-bit)
  - createRenderPass : wrapper for vkCreateRenderPass

*/
VkFormat findSupportedFormat(VkPhysicalDevice             physicalDevice,
                             const std::vector<VkFormat>& candidates,
                             VkImageTiling                tiling,
                             VkFormatFeatureFlags         features);
VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
VkFormat findDepthStencilFormat(VkPhysicalDevice physicalDevice);

//////////////////////////////////////////////////////////////////////////

VkRenderPass createRenderPass(VkDevice                     device,
                              const std::vector<VkFormat>& colorAttachmentFormats,
                              VkFormat                     depthAttachmentFormat,
                              uint32_t                     subpassCount  = 1,
                              bool                         clearColor    = true,
                              bool                         clearDepth    = true,
                              VkImageLayout                initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                              VkImageLayout                finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


}  // namespace nvvk
