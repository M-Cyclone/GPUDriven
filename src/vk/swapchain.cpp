#include "vk/swapchain.h"
#include <algorithm>

#include "vk/device.h"
#include "vk/error.h"

void Swapchain::init(Device& device)
{
    VkDevice vk_device = device.device();

    std::vector<uint32_t> unique_queues = device.getUniqueQueueFamilyIndices();

    VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    info.pNext                    = nullptr;
    info.flags                    = 0;
    info.surface                  = device.surface();
    info.minImageCount            = m_img_count;
    info.imageFormat              = m_surface_format.format;
    info.imageColorSpace          = m_surface_format.colorSpace;
    info.imageExtent              = m_img_extent;
    info.imageArrayLayers         = 1;
    info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode         = unique_queues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount    = static_cast<uint32_t>(unique_queues.size());
    info.pQueueFamilyIndices      = unique_queues.data();
    info.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode              = m_present_mode;
    info.clipped                  = VK_TRUE;
    info.oldSwapchain             = VK_NULL_HANDLE;

    NVVK_CHECK(vkCreateSwapchainKHR(vk_device, &info, nullptr, &m_swapchain));


    uint32_t count;
    NVVK_CHECK(vkGetSwapchainImagesKHR(vk_device, m_swapchain, &count, nullptr));
    m_swapchain_imgs.resize(count);
    NVVK_CHECK(vkGetSwapchainImagesKHR(vk_device, m_swapchain, &count, m_swapchain_imgs.data()));


    m_swapchain_img_views.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        VkComponentMapping mapping{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };

        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageViewCreateInfo info{};
        info.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext            = nullptr;
        info.flags            = 0;
        info.image            = m_swapchain_imgs[i];
        info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        info.format           = m_surface_format.format;
        info.subresourceRange = range;
        info.components       = mapping;

        NVVK_CHECK(vkCreateImageView(vk_device, &info, nullptr, &m_swapchain_img_views[i]));
    }


    m_is_initialized = true;
}

void Swapchain::deinit(Device& device)
{
    VkDevice vk_device = device.device();

    for (VkImageView& img_view : m_swapchain_img_views)
    {
        vkDestroyImageView(vk_device, img_view, nullptr);
        img_view = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(vk_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;

    m_is_initialized = false;
}

void Swapchain::querySwapchainInfo(Device& device, uint32_t w, uint32_t h)
{
    VkPhysicalDevice gpu     = device.activeGPU();
    VkSurfaceKHR     surface = device.surface();

    {
        uint32_t count;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, formats.data()));

        m_surface_format = formats[0];
        for (const VkSurfaceFormatKHR& format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_surface_format = format;
                break;
            }
        }
    }


    {
        VkSurfaceCapabilitiesKHR capas;
        NVVK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capas));

        m_img_count         = std::clamp<uint32_t>(2, capas.minImageCount, capas.maxImageCount);
        m_img_extent.width  = std::clamp(w, capas.minImageExtent.width, capas.maxImageExtent.width);
        m_img_extent.height = std::clamp(w, capas.minImageExtent.height, capas.maxImageExtent.height);
        m_surface_transform = capas.currentTransform;
    }


    {
        uint32_t count;
        NVVK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> presents(count);
        NVVK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, presents.data()));

        m_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& present : presents)
        {
            if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_present_mode = present;
                break;
            }
        }
    }
}
