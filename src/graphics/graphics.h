#pragma once
#include <numeric>
#include <vector>

#include <vulkan/vulkan.h>

#include "graphics/vertex.h"

#include "utils/exception.h"

class Window;

class Graphics
{
public:
    static constexpr uint32_t kInvalidQueueIndex = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t kMaxInFlightCount  = 2;

public:
    class VkException : public EngineDefaultException
    {
    public:
        VkException(int line, const char* file, VkResult result) noexcept;

        const char* what() const noexcept override;
        const char* getType() const noexcept override;

    private:
        VkResult m_result;
    };

public:
    explicit Graphics(Window& window);
    Graphics(const Graphics&)            = delete;
    Graphics& operator=(const Graphics&) = delete;
    ~Graphics() noexcept;

    void WaitIdle();

    void BeginFrame();
    void EndFrame();

    void DrawTestData(float total_time);

private:
    Window& m_window;

    VkInstance m_instance = VK_NULL_HANDLE;
#ifdef USE_VULKAN_VALIDATION_LAYER
    PFN_vkCreateDebugUtilsMessengerEXT  m_vkCreateDebugUtilsMessengerEXT  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT m_vkDestroyDebugUtilsMessengerEXT = nullptr;
    VkDebugUtilsMessengerEXT            m_debugMsgr                       = VK_NULL_HANDLE;
#endif  // USE_VULKAN_VALIDATION_LAYER

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_activeGPU                = VK_NULL_HANDLE;
    uint32_t         m_queueFamilyIndexGraphics = kInvalidQueueIndex;
    uint32_t         m_queueFamilyIndexPresent  = kInvalidQueueIndex;
    VkDevice         m_device                   = VK_NULL_HANDLE;

    VkQueue m_queueGraphics = VK_NULL_HANDLE;
    VkQueue m_queuePresent  = VK_NULL_HANDLE;

    VkSurfaceFormatKHR         m_swapchainSurfaceFormat;
    uint32_t                   m_swapchainImageCount = 0;
    VkExtent2D                 m_swapchainImageExtent;
    VkSurfaceTransformFlagsKHR m_swapchainTransform;
    VkPresentModeKHR           m_swapchainPresentMode;

    VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     m_swapchainImages;      // Swapchain image is created by swapchain.
    std::vector<VkImageView> m_swapchainImageViews;  // Swapchain image view is created by Graphics.

    VkCommandPool                m_swapchainImagePresentCmdPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_swapchainImagePresentCmds;
    std::vector<VkSemaphore>     m_swapchainRenderFinishedSemaphores;
    std::vector<VkSemaphore>     m_swapchainImageAvailableSemaphores;
    std::vector<VkFence>         m_cmdAvailableFences;
    uint32_t                     m_currFrameIndex = 0;

    uint32_t        m_currSCImgIndex              = 0;
    VkSemaphore     m_currSCRenderFinishSemaphore = VK_NULL_HANDLE;
    VkSemaphore     m_currSCImgAvailableSemaphore = VK_NULL_HANDLE;
    VkFence         m_currCmdAvailableFence       = VK_NULL_HANDLE;
    VkCommandBuffer m_currCmd                     = VK_NULL_HANDLE;

private:
    void CreateRenderPass();
    void DestroyRenderPass();

    void CreateFramebuffers();
    void DestroyFramebuffers();

    void CreateResources();
    void DestroyResources();

    void CreateDescriptorSet();
    void DestroyDescriptorSet();

    void CreatePipeline();
    void DestroyPipeline();

private:
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_framebuffers;

    vertex::Layout              m_vertexLayout;
    VkBuffer                    m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory              m_vertexMemory = VK_NULL_HANDLE;
    VkBuffer                    m_indexBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory              m_indexMemory  = VK_NULL_HANDLE;
    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformMemorys;
    std::vector<void*>          m_uniformMemPtrs;

    VkDescriptorPool             m_descriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout        m_descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkPipelineLayout m_pipelineLayout   = VK_NULL_HANDLE;
    VkPipeline       m_graphicsPipeline = VK_NULL_HANDLE;
};
