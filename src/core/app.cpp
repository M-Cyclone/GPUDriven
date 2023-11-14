#include "core/app.h"
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef PLATFORM_WINDOWS
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#endif  // PLATFORM_WINDOWS

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk/error.h"
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
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

    auto start = std::chrono::high_resolution_clock::now();
    auto prev  = start;
    
    while (m_is_running)
    {
        glfwPollEvents();

        auto  curr = std::chrono::high_resolution_clock::now();

        float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(curr - prev).count();
        float total_time = std::chrono::duration<float, std::chrono::seconds::period>(curr - start).count();

        update(delta_time, total_time);
        render();

        prev = curr;
    }

    exit();
}

void App::onResize(uint32_t width, uint32_t height)
{
    m_width        = width;
    m_height       = height;
    m_aspect_ratio = (float)width / (float)height;


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

    using VET = vertex::AttributeType;
    m_layout.append(VET::Pos2d);   // pos
    m_layout.append(VET::Color3);  // color

    m_alloc = std::make_unique<Allocator>(m_device);

    createCmdPoolAndAllocateBuffer();

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();

    createFramebuffer();

    createGraphicsPipeline();

    createSyncObjects();
}

void App::exit()
{
    vkDeviceWaitIdle(m_device.device());

    destroySyncObjects();

    destroyGraphicsPipeline();

    destroyFramebuffer();

    destroyUniformBuffers();
    destroyIndexBuffer();
    destroyVertexBuffer();

    destroyCmdPoolAndDeallocateBuffer();

    vkDestroyRenderPass(m_device.device(), m_render_pass, nullptr);

    m_swapchain.deinit(m_device);

    m_device.deinit();
}

void App::createGraphicsPipeline()
{
    m_dset = std::make_unique<nvvk::DescriptorSetContainer>(m_device.device());
    m_dset->addBinding(BINDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL);
    m_dset->initLayout();
    m_dset->initPool(k_max_in_flight_count);
    m_dset->initPipeLayout();


    for (uint32_t i = 0; i < k_max_in_flight_count; ++i)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = m_uniform_buffers[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range  = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write = m_dset->makeWrite(i, BINDING_UBO, &buffer_info);
        vkUpdateDescriptorSets(m_device.device(), 1, &write, 0, nullptr);
    }


    const uint32_t binding = 0;

    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding   = binding;
    binding_desc.stride    = static_cast<uint32_t>(m_layout.getStride());
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attribute_descs;
    m_layout.getAttributeDescs(binding, attribute_descs);


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
    m_img_draw_finished_semaphores.resize(k_max_in_flight_count);
    m_img_available_semaphores.resize(k_max_in_flight_count);
    m_cmd_available_fences.resize(k_max_in_flight_count);

    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_info.pNext                 = nullptr;
    semaphore_info.flags                 = 0;

    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.pNext             = nullptr;
    fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < k_max_in_flight_count; ++i)
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
    using VET = vertex::AttributeType;

    vertex::Buffer vb(m_layout, 4);
    vb[0].attr<VET::Pos2d>()  = { -0.5f, -0.5f };
    vb[1].attr<VET::Pos2d>()  = { +0.5f, -0.5f };
    vb[2].attr<VET::Pos2d>()  = { +0.5f, +0.5f };
    vb[3].attr<VET::Pos2d>()  = { -0.5f, +0.5f };
    vb[0].attr<VET::Color3>() = { 1.0f, 0.0f, 0.0f };
    vb[1].attr<VET::Color3>() = { 0.0f, 1.0f, 1.0f };
    vb[2].attr<VET::Color3>() = { 0.0f, 1.0f, 0.0f };
    vb[3].attr<VET::Color3>() = { 0.0f, 0.0f, 1.0f };

    m_vertex_buffer = m_alloc->createVertexBuffer(vb);


    Buffer staging_buffer = m_alloc->createStagingBuffer(vb.data());
    {
        VkCommandBuffer cmd = createTempCommandBuffer();
        Allocator::copyBuffer(cmd, staging_buffer, m_vertex_buffer, vb.sizeOf());
        submitAndWaitTempCommandBuffer(cmd, m_device.queueGraphics());
        freeTempCommandBuffer(cmd);
    }
    m_alloc->destroyBuffer(staging_buffer);
}

void App::destroyVertexBuffer()
{
    m_alloc->destroyBuffer(m_vertex_buffer);
}

void App::createIndexBuffer()
{
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    m_index_buffer = m_alloc->createIndexBuffer(indices);


    VkDeviceSize buffer_size    = static_cast<VkDeviceSize>(sizeof(uint16_t) * indices.size());
    Buffer       staging_buffer = m_alloc->createStagingBuffer(buffer_size, indices.data());
    {
        VkCommandBuffer cmd = createTempCommandBuffer();
        Allocator::copyBuffer(cmd, staging_buffer, m_index_buffer, buffer_size);
        submitAndWaitTempCommandBuffer(cmd, m_device.queueGraphics());
        freeTempCommandBuffer(cmd);
    }
    m_alloc->destroyBuffer(staging_buffer);
}

void App::destroyIndexBuffer()
{
    m_alloc->destroyBuffer(m_index_buffer);
}

void App::createUniformBuffers()
{
    VkDeviceSize buffer_size = static_cast<VkDeviceSize>(sizeof(UniformBufferObject));

    m_uniform_buffers.reserve(k_max_in_flight_count);
    m_uniform_buffers_mapped.resize(k_max_in_flight_count);

    for (size_t i = 0; i < k_max_in_flight_count; i++)
    {
        m_uniform_buffers.push_back(m_alloc->createUniformBuffer(buffer_size));

        m_uniform_buffers_mapped[i] = m_device.mapMemory(m_uniform_buffers[i].memory, 0, buffer_size, 0);
    }
}

void App::destroyUniformBuffers()
{
    for (size_t i = 0; i < k_max_in_flight_count; i++)
    {
        m_device.unmapMemory(m_uniform_buffers[i].memory);
        m_alloc->destroyBuffer(m_uniform_buffers[i]);
    }
}

VkCommandBuffer App::createTempCommandBuffer() const
{
    VkCommandBufferAllocateInfo allo_iInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allo_iInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allo_iInfo.commandPool                 = m_cmd_pool;
    allo_iInfo.commandBufferCount          = 1;

    VkCommandBuffer cmd;
    NVVK_CHECK(vkAllocateCommandBuffers(m_device.device(), &allo_iInfo, &cmd));

    return cmd;
}

void App::submitAndWaitTempCommandBuffer(VkCommandBuffer cmd, VkQueue queue) const
{
    VkSubmitInfo submitInfo       = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    NVVK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    NVVK_CHECK(vkQueueWaitIdle(queue));
}

void App::freeTempCommandBuffer(VkCommandBuffer cmd) const
{
    vkFreeCommandBuffers(m_device.device(), m_cmd_pool, 1, &cmd);
}

void App::update(float delta_time, float total_time)
{
    UniformBufferObject ubo{};
    ubo.model       = glm::rotate(glm::mat4(1.0f), total_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view        = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj        = glm::perspective(glm::radians(45.0f), m_aspect_ratio, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    std::memcpy(m_uniform_buffers_mapped[m_curr_frame_idx], &ubo, sizeof(ubo));
}

void App::render()
{
    VkDevice       device    = m_device.device();
    VkSwapchainKHR swapchain = m_swapchain.swapchain();


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
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_dset->getPipeLayout(),
                                    0,
                                    1,
                                    m_dset->getSets(m_curr_frame_idx),
                                    0,
                                    nullptr);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &area);

            VkBuffer     vertex_buffers[] = { m_vertex_buffer.buffer };
            VkDeviceSize offsets[]        = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

            vkCmdBindIndexBuffer(cmd, m_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
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


    m_curr_frame_idx = (m_curr_frame_idx + 1) % k_max_in_flight_count;
}
