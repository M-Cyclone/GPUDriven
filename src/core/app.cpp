#include "core/app.h"
#include <array>
#include <cassert>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef PLATFORM_WINDOWS
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#endif  // PLATFORM_WINDOWS

#include <glm/glm.hpp>

#include "vk/error_vk.h"
#include "vk/pipeline.h"
#include "vk/render_pass.h"

#include "utils/load_shader.h"

#include "shader_header/device.h"
#include "shader_header/vertex_info.h"

bool g_init_app = false;

App::App()
    : m_window{ nullptr, glfwDestroyWindow }
{
    assert(!g_init_app);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    {
        m_window.reset(
            glfwCreateWindow(static_cast<int>(k_window_width), static_cast<int>(k_window_height), k_window_title, nullptr, nullptr));

        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create window.");
        }

        // clang-format off
        glfwSetWindowUserPointer(m_window.get(), this);

        glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow* wnd)
        {
            reinterpret_cast<App*>(glfwGetWindowUserPointer(wnd))->m_is_running = false;
        });

        glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow* wnd, int w, int h)
        {
            reinterpret_cast<App*>(glfwGetWindowUserPointer(wnd))->onResize(w, h);
        });
        // clang-format on
    }

    g_init_app = true;
}

App::~App()
{
    m_window.reset();

    glfwTerminate();
}

void App::run()
{
    init();

    while (m_is_running)
    {
        glfwPollEvents();

        update();
        render();
    }

    exit();
}

void App::onResize(uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(m_device.device());

    destroyFramebuffer();
    m_swapchain.deinit(m_device);


    m_swapchain.querySwapchainInfo(m_device, width, height);
    m_swapchain.init(m_device);
    createFramebuffer();
}

void App::init()
{
    m_device.setVulkanApiVersion(VK_API_VERSION_1_3);
#ifdef USE_VULKAN_VALIDATION_LAYER
    m_device.addInstanceLayer("VK_LAYER_KHRONOS_validation");
#endif  // USE_VULKAN_VALIDATION_LAYER
    m_device.setCreateSurfaceFunc([wnd = m_window.get()](VkInstance instance, VkSurfaceKHR& surface) {
        return glfwCreateWindowSurface(instance, wnd, nullptr, &surface);
    });
    m_device.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    m_device.init();


    m_swapchain.querySwapchainInfo(m_device, k_window_width, k_window_height);
    m_swapchain.init(m_device);


    m_render_pass = nvvk::createRenderPass(m_device.device(),
                                           { m_swapchain.getSurfaceFormat() },
                                           VK_FORMAT_UNDEFINED,
                                           1,
                                           true,
                                           false,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    createFramebuffer();
    
    createGraphicsPipeline();
    
    createCmdPoolAndAllocateBuffer();

    createSyncObjects();

    createVertexBuffer();
}

void App::exit()
{
    vkDeviceWaitIdle(m_device.device());

    destroyVertexBuffer();

    destroySyncObjects();

    destroyCmdPoolAndDeallocateBuffer();

    destroyGraphicsPipeline();

    destroyFramebuffer();

    vkDestroyRenderPass(m_device.device(), m_render_pass, nullptr);

    m_swapchain.deinit(m_device);

    m_device.deinit();
}

void App::createGraphicsPipeline()
{
    m_dset = std::make_unique<nvvk::DescriptorSetContainer>(m_device.device());
    m_dset->initLayout();
    m_dset->initPool(1);
    m_dset->initPipeLayout();


    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding   = 0;
    binding_desc.stride    = sizeof(Vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attribute_descs(2);
    attribute_descs[0].location = LOCATION_VERTEX_IN_POSITION;
    attribute_descs[0].binding  = 0;
    attribute_descs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attribute_descs[0].offset   = offsetof(Vertex, pos);
    attribute_descs[1].location = LOCATION_VERTEX_IN_COLOR;
    attribute_descs[1].binding  = 0;
    attribute_descs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descs[1].offset   = offsetof(Vertex, color);

    nvvk::GraphicsPipelineState pstate{};
    pstate.rasterizationState.cullMode = VK_CULL_MODE_NONE;
    pstate.addBindingDescription(binding_desc);
    pstate.addAttributeDescriptions(attribute_descs);


    nvvk::GraphicsPipelineGenerator pgen(m_device.device(), m_dset->getPipeLayout(), m_render_pass, pstate);
    pgen.addShader(loadShaderCode("test.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT, "main");
    pgen.addShader(loadShaderCode("test.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

    m_pipeline = pgen.createPipeline();

    pgen.clearShaders();
}

void App::destroyGraphicsPipeline()
{
    vkDestroyPipeline(m_device.device(), m_pipeline, nullptr);
    m_dset.reset();
}

void App::createFramebuffer()
{
    const size_t count = m_swapchain.getSwapchainImageCount();

    VkExtent2D extent = m_swapchain.getSwapchainImageExtent();

    m_framebuffers.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        std::array<VkImageView, 1> attachments = { m_swapchain.getSwapchainImageView(i) };

        VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        info.pNext                   = nullptr;
        info.flags                   = 0;
        info.renderPass              = m_render_pass;
        info.attachmentCount         = static_cast<uint32_t>(attachments.size());
        info.pAttachments            = attachments.data();
        info.width                   = extent.width;
        info.height                  = extent.height;
        info.layers                  = 1;

        NVVK_CHECK(vkCreateFramebuffer(m_device.device(), &info, nullptr, &m_framebuffers[i]));
    }
}

void App::destroyFramebuffer()
{
    for (VkFramebuffer& fb : m_framebuffers)
    {
        vkDestroyFramebuffer(m_device.device(), fb, nullptr);
        fb = VK_NULL_HANDLE;
    }
}

void App::createCmdPoolAndAllocateBuffer()
{
    const size_t count = m_swapchain.getSwapchainImageCount();

    VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    info.pNext                   = nullptr;
    info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex        = m_device.getQueueGraphicsIndex();

    NVVK_CHECK(vkCreateCommandPool(m_device.device(), &info, nullptr, &m_cmd_pool));


    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext              = 0;
    alloc_info.commandPool        = m_cmd_pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;

    m_cmds.resize(count);
    NVVK_CHECK(vkAllocateCommandBuffers(m_device.device(), &alloc_info, m_cmds.data()));
}

void App::destroyCmdPoolAndDeallocateBuffer()
{
    vkFreeCommandBuffers(m_device.device(), m_cmd_pool, m_swapchain.getSwapchainImageCount(), m_cmds.data());
    vkDestroyCommandPool(m_device.device(), m_cmd_pool, nullptr);
}

void App::createSyncObjects()
{
    const size_t count = m_swapchain.getSwapchainImageCount();

    m_img_draw_finished_semaphores.resize(count);
    m_img_available_semaphores.resize(count);
    m_cmd_available_fences.resize(count);

    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_info.pNext                 = nullptr;
    semaphore_info.flags                 = 0;

    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.pNext             = nullptr;
    fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; ++i)
    {
        NVVK_CHECK(vkCreateSemaphore(m_device.device(), &semaphore_info, nullptr, &m_img_draw_finished_semaphores[i]));
        NVVK_CHECK(vkCreateSemaphore(m_device.device(), &semaphore_info, nullptr, &m_img_available_semaphores[i]));
        NVVK_CHECK(vkCreateFence(m_device.device(), &fence_info, nullptr, &m_cmd_available_fences[i]));
    }
}

void App::destroySyncObjects()
{
    for (VkSemaphore s : m_img_draw_finished_semaphores)
    {
        vkDestroySemaphore(m_device.device(), s, nullptr);
    }
    for (VkSemaphore s : m_img_available_semaphores)
    {
        vkDestroySemaphore(m_device.device(), s, nullptr);
    }
    for (VkFence f : m_cmd_available_fences)
    {
        vkDestroyFence(m_device.device(), f, nullptr);
    }
}

void App::createVertexBuffer()
{
    const std::vector<Vertex> vertices = {
        {{ +0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        {{ +0.5f, +0.5f }, { 0.0f, 1.0f, 0.0f }},
        {{ -0.5f, +0.5f }, { 0.0f, 0.0f, 1.0f }}
    };

    VkBufferCreateInfo info    = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.pNext                 = nullptr;
    info.flags                 = 0;
    info.size                  = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size());
    info.usage                 = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices   = nullptr;

    NVVK_CHECK(vkCreateBuffer(m_device.device(), &info, nullptr, &m_vertex_buffer));


    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(m_device.device(), m_vertex_buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.pNext                = nullptr;
    alloc_info.allocationSize       = mem_req.size;
    alloc_info.memoryTypeIndex =
        m_device.findMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    NVVK_CHECK(vkAllocateMemory(m_device.device(), &alloc_info, nullptr, &m_vertex_buffer_memory));


    NVVK_CHECK(vkBindBufferMemory(m_device.device(), m_vertex_buffer, m_vertex_buffer_memory, 0));


    void* data;
    NVVK_CHECK(vkMapMemory(m_device.device(), m_vertex_buffer_memory, 0, info.size, 0, &data));
    std::memcpy(data, vertices.data(), (size_t)info.size);
    vkUnmapMemory(m_device.device(), m_vertex_buffer_memory);
}

void App::destroyVertexBuffer()
{
    vkFreeMemory(m_device.device(), m_vertex_buffer_memory, nullptr);
    vkDestroyBuffer(m_device.device(), m_vertex_buffer, nullptr);
}

void App::update()
{}

void App::render()
{
    VkDevice       device    = m_device.device();
    VkSwapchainKHR swapchain = m_swapchain.get();

    constexpr uint64_t k_max_wait_time = std::numeric_limits<uint64_t>::max();

    VkSemaphore img_draw_finished_semaphore = m_img_draw_finished_semaphores[m_curr_frame_idx];
    VkSemaphore img_available_semaphore     = m_img_available_semaphores[m_curr_frame_idx];
    VkFence     cmd_available_fence         = m_cmd_available_fences[m_curr_frame_idx];

    NVVK_CHECK(vkWaitForFences(device, 1, &cmd_available_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
    NVVK_CHECK(vkResetFences(device, 1, &cmd_available_fence));

    uint32_t img_idx;
    NVVK_CHECK(vkAcquireNextImageKHR(device, swapchain, k_max_wait_time, img_available_semaphore, VK_NULL_HANDLE, &img_idx));


    VkCommandBuffer cmd = m_cmds[m_curr_frame_idx];
    NVVK_CHECK(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext            = nullptr;
    begin_info.flags            = 0;
    begin_info.pInheritanceInfo = nullptr;
    NVVK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
    {
        VkExtent2D extent = m_swapchain.getSwapchainImageExtent();
        VkRect2D   area{
              .offset{ 0, 0 },
              .extent{ extent }
        };
        VkClearValue clear_value{ .color{ .float32{ 0.1f, 0.1f, 0.1f, 1.0f } } };
        VkViewport   viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);

        VkRenderPassBeginInfo render_pass_begin{};
        render_pass_begin.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext           = nullptr;
        render_pass_begin.renderPass      = m_render_pass;
        render_pass_begin.framebuffer     = m_framebuffers[img_idx];
        render_pass_begin.renderArea      = area;
        render_pass_begin.clearValueCount = 1;
        render_pass_begin.pClearValues    = &clear_value;
        vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &area);

            VkBuffer     vertex_buffers[] = { m_vertex_buffer };
            VkDeviceSize offsets[]       = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

            vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmd);
    }
    NVVK_CHECK(vkEndCommandBuffer(cmd));


    VkPipelineStageFlags submit_wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &img_available_semaphore;
    submit_info.pWaitDstStageMask    = submit_wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &img_draw_finished_semaphore;
    NVVK_CHECK(vkQueueSubmit(m_device.queueGraphics(), 1, &submit_info, cmd_available_fence));

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &img_draw_finished_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &img_idx;
    present_info.pResults           = nullptr;
    NVVK_CHECK(vkQueuePresentKHR(m_device.queuePresent(), &present_info));


    m_curr_frame_idx = (m_curr_frame_idx + 1) % m_swapchain.getSwapchainImageCount();
}
