#pragma once

#include "graphics/graphics.h"

class GraphicsAvailable
{
protected:
    static VkInstance               getInstance(Graphics& gfx) noexcept { return gfx.m_instance; }
    static VkDebugUtilsMessengerEXT getDebugMsgr(Graphics& gfx) noexcept { return gfx.m_debug_msgr; }

    static VkSurfaceKHR getSurface(Graphics& gfx) noexcept { return gfx.m_surface; }

    static VkPhysicalDevice getActiveGpu(Graphics& gfx) noexcept { return gfx.m_active_gpu; }
    static VkDevice         getDevice(Graphics& gfx) noexcept { return gfx.m_device; }

    static uint32_t getQueueFamilyIndexGraphics(Graphics& gfx) noexcept { return gfx.m_queue_family_index_graphics; }
    static uint32_t getQueueFamilyIndexPresent(Graphics& gfx) noexcept { return gfx.m_queue_family_index_present; }
    static VkQueue  getQueueGraphics(Graphics& gfx) noexcept { return gfx.m_queue_graphics; }
    static VkQueue  getQueuePresent(Graphics& gfx) noexcept { return gfx.m_queue_present; }

    static VkSwapchainKHR getSwapchain(Graphics& gfx) noexcept { return gfx.m_swapchain; }
    static VkImage        getCurrSwapchainImage(Graphics& gfx) noexcept { return gfx.m_swapchain_images[gfx.m_curr_sc_img_index]; }
    static VkImageView    getCurrSwapchainImageView(Graphics& gfx) noexcept { return gfx.m_swapchain_image_views[gfx.m_curr_sc_img_index]; }

    static VkCommandPool   getSwapchainCmdPool(Graphics& gfx) noexcept { return gfx.m_swapchain_image_present_cmd_pool; }
    static VkCommandBuffer getCurrSwapchainCmd(Graphics& gfx) noexcept
    {
        return gfx.m_swapchain_image_present_cmds[gfx.m_curr_frame_index];
    }

    static uint32_t getCurrFrameIndex(Graphics& gfx) noexcept { return gfx.m_curr_frame_index; }
};
