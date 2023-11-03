#pragma once
#include <vector>

#include <vulkan/vulkan.h>

class Device;

class Swapchain final
{
    friend class Device;

public:
    Swapchain()                            = default;
    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&)                 = delete;
    void operator=(Swapchain&&)            = delete;
    ~Swapchain() noexcept                  = default;

public:
    void init(Device& device);
    void deinit(Device& device);
    void querySwapchainInfo(Device& device, uint32_t w, uint32_t h);

public:
    VkSwapchainKHR  get() const { return m_swapchain; }

    VkFormat        getSurfaceFormat() const { return m_surface_format.format; }
    VkColorSpaceKHR getSurfaceColorSpace() const { return m_surface_format.colorSpace; }
    uint32_t        getSwapchainImageCount() const { return m_img_count; }
    VkExtent2D      getSwapchainImageExtent() const { return m_img_extent; }
    VkImageView     getSwapchainImageView(uint32_t idx) const { return m_swapchain_img_views[idx]; }

private:
    VkSurfaceFormatKHR         m_surface_format    = {};
    uint32_t                   m_img_count         = 0;
    VkExtent2D                 m_img_extent        = {};
    VkSurfaceTransformFlagsKHR m_surface_transform = {};
    VkPresentModeKHR           m_present_mode      = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     m_swapchain_imgs;
    std::vector<VkImageView> m_swapchain_img_views;

    bool m_is_initialized = false;
};
