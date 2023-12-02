#include "core/app.h"
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef PLATFORM_WINDOWS
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#endif  // PLATFORM_WINDOWS

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb/stb_image.h>

#include "utils/load_shader.h"
#include "utils/log.h"

#include "shader_header/device.h"
#include "shader_header/vertex_info.h"


bool g_init_app = false;

static constexpr const char* k_window_title  = "GPU Driven";
static constexpr uint32_t    k_window_width  = 1280;
static constexpr uint32_t    k_window_height = 720;

App::App()
    : m_window(this, k_window_width, k_window_height, k_window_title)
    , m_gfx(m_window)
{
    assert(!g_init_app);

    GLFWwindow* wnd = m_window.getNativeWindow();

    glfwSetCharCallback(wnd, [](GLFWwindow* w, unsigned int codepoint) { ((App*)glfwGetWindowUserPointer(w))->onChar(codepoint); });
    // glfwSetCharModsCallback();
    glfwSetCursorEnterCallback(wnd, [](GLFWwindow* w, int entered) { ((App*)glfwGetWindowUserPointer(w))->onCursorEnter(entered); });
    glfwSetCursorPosCallback(wnd,
                             [](GLFWwindow* w, double xpos, double ypos) { ((App*)glfwGetWindowUserPointer(w))->onCursorPos(xpos, ypos); });
    glfwSetDropCallback(wnd, [](GLFWwindow* w, int path_count, const char* paths[]) {
        ((App*)glfwGetWindowUserPointer(w))->onDrop(path_count, paths);
    });
    glfwSetFramebufferSizeCallback(wnd, [](GLFWwindow* w, int width, int height) {
        ((App*)glfwGetWindowUserPointer(w))->onFramebufferSize(width, height);
    });
    // glfwSetJoystickCallback();
    glfwSetKeyCallback(wnd, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        ((App*)glfwGetWindowUserPointer(w))->onKey(key, scancode, action, mods);
    });
    // glfwSetMonitorCallback();
    glfwSetMouseButtonCallback(wnd, [](GLFWwindow* w, int button, int action, int mods) {
        ((App*)glfwGetWindowUserPointer(w))->onMouseButton(button, action, mods);
    });
    glfwSetScrollCallback(wnd, [](GLFWwindow* w, double xoffset, double yoffset) {
        ((App*)glfwGetWindowUserPointer(w))->onScroll(xoffset, yoffset);
    });
    glfwSetWindowCloseCallback(wnd, [](GLFWwindow* w) { ((App*)glfwGetWindowUserPointer(w))->onWindowClose(); });
    glfwSetWindowFocusCallback(wnd, [](GLFWwindow* w, int focused) { ((App*)glfwGetWindowUserPointer(w))->onWindowFocus(focused); });
    // glfwSetWindowMaximizeCallback();
    // glfwSetWindowPosCallback();
    // glfwSetWindowRefreshCallback();
    // glfwSetWindowSizeCallback();

    g_init_app = true;
}

void App::run()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto prev  = start;
    
    while (m_is_running)
    {
        glfwPollEvents();

        if (m_is_paused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        auto  curr = std::chrono::high_resolution_clock::now();

        float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(curr - prev).count();
        float total_time = std::chrono::duration<float, std::chrono::seconds::period>(curr - start).count();

        update(delta_time, total_time);

        prev = curr;
    }

    m_gfx.waitIdle();
}
//
//void App::init()
//{
//    m_alloc = std::make_unique<Allocator>(m_device);
//
//    {
//        VkPhysicalDeviceProperties props;
//        vkGetPhysicalDeviceProperties(m_device.activeGPU(), &props);
//
//        VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
//        if (counts & VK_SAMPLE_COUNT_2_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_2_BIT;
//        }
//        if (counts & VK_SAMPLE_COUNT_4_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_4_BIT;
//        }
//        if (counts & VK_SAMPLE_COUNT_8_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_8_BIT;
//        }
//        if (counts & VK_SAMPLE_COUNT_16_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_16_BIT;
//        }
//        if (counts & VK_SAMPLE_COUNT_32_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_32_BIT;
//        }
//        if (counts & VK_SAMPLE_COUNT_64_BIT)
//        {
//            m_msaa_samples = VK_SAMPLE_COUNT_64_BIT;
//        }
//    }
//
//
//    m_swapchain.querySwapchainInfo(m_device, k_window_width, k_window_height);
//    m_swapchain.init(m_device);
//
//
//    m_depth_format = m_alloc->findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
//                                                  VK_IMAGE_TILING_OPTIMAL,
//                                                  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
//
//
//    using VET = vertex::AttributeType;
//    m_layout.append(VET::Pos3d);      // pos
//    m_layout.append(VET::TexCoords);  // uv
//
//    createCmdPoolAndAllocateBuffer();
//
//    createVertexBuffer();
//    createIndexBuffer();
//    createUniformBuffers();
//
//    createTextureImageAndSampler();
//
//    createAttachmentBuffer();
//
//    createRenderPass();
//
//    createFramebuffer();
//
//    createGraphicsPipeline();
//
//    createSyncObjects();
//}
//
//void App::exit()
//{
//    vkDeviceWaitIdle(m_device.device());
//
//    destroySyncObjects();
//
//    destroyGraphicsPipeline();
//
//    destroyFramebuffer();
//
//    destroyRenderPass();
//
//    destroyAttachmentBuffer();
//
//    destroyTextureImageAndSampler();
//
//    destroyUniformBuffers();
//    destroyIndexBuffer();
//    destroyVertexBuffer();
//
//    destroyCmdPoolAndDeallocateBuffer();
//
//    m_swapchain.deinit(m_device);
//
//    m_device.deinit();
//}
//
//void App::createGraphicsPipeline()
//{
//    m_dset = std::make_unique<nvvk::DescriptorSetContainer>(m_device.device());
//    m_dset->addBinding(BINDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL);
//    m_dset->addBinding(BINDING_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
//    m_dset->initLayout();
//    m_dset->initPool(k_max_in_flight_count);
//    m_dset->initPipeLayout();
//
//
//    for (uint32_t i = 0; i < k_max_in_flight_count; ++i)
//    {
//        VkDescriptorBufferInfo buffer_info{};
//        buffer_info.buffer = m_uniform_buffers[i].buffer;
//        buffer_info.offset = 0;
//        buffer_info.range  = sizeof(UniformBufferObject);
//
//        VkDescriptorImageInfo img_info{};
//        img_info.sampler     = m_sampler;
//        img_info.imageView   = m_texture.view;
//        img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//        std::vector<VkWriteDescriptorSet> writes;
//        writes.push_back(m_dset->makeWrite(i, BINDING_UBO, &buffer_info));
//        writes.push_back(m_dset->makeWrite(i, BINDING_SAMPLER, &img_info));
//        vkUpdateDescriptorSets(m_device.device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
//    }
//
//
//    const uint32_t binding = 0;
//
//    VkVertexInputBindingDescription binding_desc{};
//    binding_desc.binding   = binding;
//    binding_desc.stride    = static_cast<uint32_t>(m_layout.getStride());
//    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//    std::vector<VkVertexInputAttributeDescription> attribute_descs;
//    m_layout.getAttributeDescs(binding, attribute_descs);
//
//    nvvk::GraphicsPipelineState pstate{};
//    pstate.rasterizationState.cullMode           = VK_CULL_MODE_NONE;
//    pstate.multisampleState.rasterizationSamples = m_msaa_samples;
//    pstate.addBindingDescription(binding_desc);
//    pstate.addAttributeDescriptions(attribute_descs);
//    pstate.depthStencilState.depthTestEnable       = VK_TRUE;
//    pstate.depthStencilState.depthWriteEnable      = VK_TRUE;
//    pstate.depthStencilState.depthCompareOp        = VK_COMPARE_OP_LESS;
//    pstate.depthStencilState.depthBoundsTestEnable = VK_FALSE;
//    pstate.depthStencilState.stencilTestEnable     = VK_FALSE;
//    pstate.depthStencilState.minDepthBounds        = 0.0f;
//    pstate.depthStencilState.maxDepthBounds        = 1.0f;
//
//    nvvk::GraphicsPipelineGenerator pgen(m_device.device(), m_dset->getPipeLayout(), m_render_pass, pstate);
//    pgen.addShader(loadShaderCode("test.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT, "main");
//    pgen.addShader(loadShaderCode("test.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");
//
//    m_pipeline = pgen.createPipeline();
//
//    pgen.clearShaders();
//}
//
//void App::destroyGraphicsPipeline()
//{
//    vkDestroyPipeline(m_device.device(), m_pipeline, nullptr);
//    m_dset.reset();
//}
//
//void App::createFramebuffer()
//{
//    const size_t count = m_swapchain.getSwapchainImageCount();
//
//    VkExtent2D extent = m_swapchain.getSwapchainImageExtent();
//
//    m_framebuffers.resize(count);
//
//    for (size_t i = 0; i < count; ++i)
//    {
//        std::array<VkImageView, 3> attachments = { m_color_buffer.view, m_depth_buffer.view, m_swapchain.getSwapchainImageView(i) };
//
//        VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
//        info.pNext                   = nullptr;
//        info.flags                   = 0;
//        info.renderPass              = m_render_pass;
//        info.attachmentCount         = static_cast<uint32_t>(attachments.size());
//        info.pAttachments            = attachments.data();
//        info.width                   = extent.width;
//        info.height                  = extent.height;
//        info.layers                  = 1;
//
//        NVVK_CHECK(vkCreateFramebuffer(m_device.device(), &info, nullptr, &m_framebuffers[i]));
//    }
//}
//
//void App::destroyFramebuffer()
//{
//    for (VkFramebuffer& fb : m_framebuffers)
//    {
//        vkDestroyFramebuffer(m_device.device(), fb, nullptr);
//        fb = VK_NULL_HANDLE;
//    }
//}
//
//void App::createCmdPoolAndAllocateBuffer()
//{
//    const size_t count = m_swapchain.getSwapchainImageCount();
//
//    VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
//    info.pNext                   = nullptr;
//    info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//    info.queueFamilyIndex        = m_device.getQueueGraphicsIndex();
//
//    NVVK_CHECK(vkCreateCommandPool(m_device.device(), &info, nullptr, &m_cmd_pool));
//
//
//    VkCommandBufferAllocateInfo alloc_info{};
//    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    alloc_info.pNext              = 0;
//    alloc_info.commandPool        = m_cmd_pool;
//    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    alloc_info.commandBufferCount = count;
//
//    m_cmds.resize(count);
//    NVVK_CHECK(vkAllocateCommandBuffers(m_device.device(), &alloc_info, m_cmds.data()));
//}
//
//void App::destroyCmdPoolAndDeallocateBuffer()
//{
//    vkFreeCommandBuffers(m_device.device(), m_cmd_pool, m_swapchain.getSwapchainImageCount(), m_cmds.data());
//    vkDestroyCommandPool(m_device.device(), m_cmd_pool, nullptr);
//}
//
//void App::createSyncObjects()
//{
//    m_img_draw_finished_semaphores.resize(k_max_in_flight_count);
//    m_img_available_semaphores.resize(k_max_in_flight_count);
//    m_cmd_available_fences.resize(k_max_in_flight_count);
//
//    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
//    semaphore_info.pNext                 = nullptr;
//    semaphore_info.flags                 = 0;
//
//    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
//    fence_info.pNext             = nullptr;
//    fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
//
//    for (uint32_t i = 0; i < k_max_in_flight_count; ++i)
//    {
//        NVVK_CHECK(vkCreateSemaphore(m_device.device(), &semaphore_info, nullptr, &m_img_draw_finished_semaphores[i]));
//        NVVK_CHECK(vkCreateSemaphore(m_device.device(), &semaphore_info, nullptr, &m_img_available_semaphores[i]));
//        NVVK_CHECK(vkCreateFence(m_device.device(), &fence_info, nullptr, &m_cmd_available_fences[i]));
//    }
//}
//
//void App::destroySyncObjects()
//{
//    for (VkSemaphore s : m_img_draw_finished_semaphores)
//    {
//        vkDestroySemaphore(m_device.device(), s, nullptr);
//    }
//    for (VkSemaphore s : m_img_available_semaphores)
//    {
//        vkDestroySemaphore(m_device.device(), s, nullptr);
//    }
//    for (VkFence f : m_cmd_available_fences)
//    {
//        vkDestroyFence(m_device.device(), f, nullptr);
//    }
//}
//
//void App::createVertexBuffer()
//{
//    using VET = vertex::AttributeType;
//
//    vertex::Buffer vb(m_layout, 8);
//    vb[0].attr<VET::Pos3d>()     = { -0.5f, -0.5f, +0.0f };
//    vb[1].attr<VET::Pos3d>()     = { +0.5f, -0.5f, +0.0f };
//    vb[2].attr<VET::Pos3d>()     = { +0.5f, +0.5f, +0.0f };
//    vb[3].attr<VET::Pos3d>()     = { -0.5f, +0.5f, +0.0f };
//    vb[4].attr<VET::Pos3d>()     = { -0.5f, -0.5f, -0.5f };
//    vb[5].attr<VET::Pos3d>()     = { +0.5f, -0.5f, -0.5f };
//    vb[6].attr<VET::Pos3d>()     = { +0.5f, +0.5f, -0.5f };
//    vb[7].attr<VET::Pos3d>()     = { -0.5f, +0.5f, -0.5f };
//    vb[0].attr<VET::TexCoords>() = { 1.0f, 0.0f };
//    vb[1].attr<VET::TexCoords>() = { 0.0f, 0.0f };
//    vb[2].attr<VET::TexCoords>() = { 0.0f, 1.0f };
//    vb[3].attr<VET::TexCoords>() = { 1.0f, 1.0f };
//    vb[4].attr<VET::TexCoords>() = { 1.0f, 0.0f };
//    vb[5].attr<VET::TexCoords>() = { 0.0f, 0.0f };
//    vb[6].attr<VET::TexCoords>() = { 0.0f, 1.0f };
//    vb[7].attr<VET::TexCoords>() = { 1.0f, 1.0f };
//
//    m_vertex_buffer = m_alloc->createVertexBuffer(vb);
//
//
//    Buffer staging_buffer = m_alloc->createStagingBuffer(vb.data());
//    {
//        VkCommandBuffer cmd = createTempCommandBuffer();
//        Allocator::copyBuffer(cmd, staging_buffer.buffer, m_vertex_buffer.buffer, vb.sizeOf());
//        submitAndWaitTempCommandBuffer(cmd, m_device.queueGraphics());
//        freeTempCommandBuffer(cmd);
//    }
//    m_alloc->destroyBuffer(staging_buffer);
//}
//
//void App::destroyVertexBuffer()
//{
//    m_alloc->destroyBuffer(m_vertex_buffer);
//}
//
//void App::createIndexBuffer()
//{
//    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };
//
//    m_index_buffer = m_alloc->createIndexBuffer(indices);
//
//
//    VkDeviceSize buffer_size    = static_cast<VkDeviceSize>(sizeof(uint16_t) * indices.size());
//    Buffer       staging_buffer = m_alloc->createStagingBuffer(buffer_size, indices.data());
//    {
//        VkCommandBuffer cmd = createTempCommandBuffer();
//        Allocator::copyBuffer(cmd, staging_buffer.buffer, m_index_buffer.buffer, buffer_size);
//        submitAndWaitTempCommandBuffer(cmd, m_device.queueGraphics());
//        freeTempCommandBuffer(cmd);
//    }
//    m_alloc->destroyBuffer(staging_buffer);
//}
//
//void App::destroyIndexBuffer()
//{
//    m_alloc->destroyBuffer(m_index_buffer);
//}
//
//void App::createUniformBuffers()
//{
//    VkDeviceSize buffer_size = static_cast<VkDeviceSize>(sizeof(UniformBufferObject));
//
//    m_uniform_buffers.reserve(k_max_in_flight_count);
//    m_uniform_buffers_mapped.resize(k_max_in_flight_count);
//
//    for (size_t i = 0; i < k_max_in_flight_count; i++)
//    {
//        m_uniform_buffers.push_back(m_alloc->createUniformBuffer(buffer_size));
//
//        m_uniform_buffers_mapped[i] = m_device.mapMemory(m_uniform_buffers[i].memory, 0, buffer_size, 0);
//    }
//}
//
//void App::destroyUniformBuffers()
//{
//    for (size_t i = 0; i < k_max_in_flight_count; i++)
//    {
//        m_device.unmapMemory(m_uniform_buffers[i].memory);
//        m_alloc->destroyBuffer(m_uniform_buffers[i]);
//    }
//}
//
//void App::createAttachmentBuffer()
//{
//    const auto [width, height] = m_swapchain.getSwapchainImageExtent();
//
//    m_color_buffer = m_alloc->createImage(width,
//                                          height,
//                                          m_swapchain.getSurfaceFormat(),
//                                          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
//                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                          VK_IMAGE_ASPECT_COLOR_BIT,
//                                          m_msaa_samples,
//                                          1);
//
//    m_depth_buffer = m_alloc->createImage(width,
//                                          height,
//                                          m_depth_format,
//                                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
//                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                          VK_IMAGE_ASPECT_DEPTH_BIT,
//                                          m_msaa_samples,
//                                          1);
//}
//
//void App::destroyAttachmentBuffer()
//{
//    m_alloc->destroyImage(m_depth_buffer);
//    m_alloc->destroyImage(m_color_buffer);
//}
//
//void App::createTextureImageAndSampler()
//{
//    int w;
//    int h;
//    int c;
//
//    stbi_uc* pixels = stbi_load("res/texture/texture.jpg", &w, &h, &c, STBI_rgb_alpha);
//    assert(pixels);
//
//    const uint32_t width  = (uint32_t)w;
//    const uint32_t height = (uint32_t)h;
//
//    VkDeviceSize imageSize      = static_cast<VkDeviceSize>(width * height * 4);
//    Buffer       staging_buffer = m_alloc->createStagingBuffer(imageSize, pixels);
//    stbi_image_free(pixels);
//
//    uint32_t mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
//
//    m_texture = m_alloc->createImage(width,
//                                     height,
//                                     VK_FORMAT_R8G8B8A8_SRGB,
//                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                     VK_IMAGE_ASPECT_COLOR_BIT,
//                                     VK_SAMPLE_COUNT_1_BIT,
//                                     mip_levels);
//
//    {
//        VkCommandBuffer cmd = createTempCommandBuffer();
//
//        Allocator::transitionImageLayout(cmd, m_texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
//
//        Allocator::copyBufferToImage(cmd, staging_buffer.buffer, m_texture.image, width, height);
//
//        // Generator mipmap.
//        {
//            VkFormatProperties format_props;
//            vkGetPhysicalDeviceFormatProperties(m_device.activeGPU(), VK_FORMAT_R8G8B8A8_SRGB, &format_props);
//            if (!(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
//            {
//                throw std::runtime_error("texture image format does not support linear blitting!");
//            }
//
//
//            VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
//            barrier.pNext                = nullptr;
//            barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
//            barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
//            barrier.image                = m_texture.image;
//            barrier.subresourceRange     = { .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
//                                             .baseMipLevel   = 0,
//                                             .levelCount     = 1,
//                                             .baseArrayLayer = 0,
//                                             .layerCount     = 1 };
//
//            int32_t mip_w = width;
//            int32_t mip_h = height;
//
//            for (uint32_t i = 1; i < mip_levels; ++i)
//            {
//                // Transfer prev level image's layout.
//                barrier.subresourceRange.baseMipLevel = i - 1;
//                barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
//                barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;
//                barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//                barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//
//                vkCmdPipelineBarrier(cmd,
//                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
//                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
//                                     0,
//                                     0,
//                                     nullptr,
//                                     0,
//                                     nullptr,
//                                     1,
//                                     &barrier);
//
//
//                // Blit prev level image to curr level.
//                VkImageBlit blit = {
//                    .srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i - 1, .baseArrayLayer = 0, .layerCount = 1 },
//                    .srcOffsets     = { { 0, 0, 0 }, { mip_w, mip_h, 1 } },
//                    .dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1 },
//                    .dstOffsets     = { { 0, 0, 0 }, { std::max(mip_w / 2, 1), std::max(mip_h / 2, 1), 1 } }
//                };
//
//                vkCmdBlitImage(cmd,
//                               m_texture.image,
//                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                               m_texture.image,
//                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                               1,
//                               &blit,
//                               VK_FILTER_LINEAR);
//
//
//                // Transfer prev level image's layout to shader read only.
//                barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//                barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//                vkCmdPipelineBarrier(cmd,
//                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
//                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//                                     0,
//                                     0,
//                                     nullptr,
//                                     0,
//                                     nullptr,
//                                     1,
//                                     &barrier);
//
//                mip_w = std::max(mip_w / 2, 1);
//                mip_h = std::max(mip_h / 2, 1);
//            }
//
//            // Transfer the last level image layout.
//            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
//            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//            barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
//            barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;
//
//            vkCmdPipelineBarrier(cmd,
//                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
//                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//                                 0,
//                                 0,
//                                 nullptr,
//                                 0,
//                                 nullptr,
//                                 1,
//                                 &barrier);
//        }
//
//
//        submitAndWaitTempCommandBuffer(cmd, m_device.queueGraphics());
//    }
//
//    m_alloc->destroyBuffer(staging_buffer);
//
//
//    VkPhysicalDeviceProperties properties{};
//    vkGetPhysicalDeviceProperties(m_device.activeGPU(), &properties);
//
//    VkSamplerCreateInfo sampler_info     = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//    sampler_info.pNext                   = nullptr;
//    sampler_info.flags                   = 0;
//    sampler_info.magFilter               = VK_FILTER_LINEAR;
//    sampler_info.minFilter               = VK_FILTER_LINEAR;
//    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//    sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    sampler_info.mipLodBias              = 0.0f;
//    sampler_info.anisotropyEnable        = VK_TRUE;
//    sampler_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
//    sampler_info.compareEnable           = VK_FALSE;
//    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
//    sampler_info.minLod                  = 0.0f;
//    sampler_info.maxLod                  = static_cast<float>(mip_levels);
//    sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//    sampler_info.unnormalizedCoordinates = VK_FALSE;
//
//    NVVK_CHECK(vkCreateSampler(m_device.device(), &sampler_info, nullptr, &m_sampler));
//}
//
//void App::destroyTextureImageAndSampler()
//{
//    vkDestroySampler(m_device.device(), m_sampler, nullptr);
//    m_alloc->destroyImage(m_texture);
//}
//
//VkCommandBuffer App::createTempCommandBuffer() const
//{
//    VkCommandBufferAllocateInfo allo_iInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
//    allo_iInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    allo_iInfo.commandPool                 = m_cmd_pool;
//    allo_iInfo.commandBufferCount          = 1;
//
//    VkCommandBuffer cmd;
//    NVVK_CHECK(vkAllocateCommandBuffers(m_device.device(), &allo_iInfo, &cmd));
//
//    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
//    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//
//    NVVK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
//
//    return cmd;
//}
//
//void App::submitAndWaitTempCommandBuffer(VkCommandBuffer cmd, VkQueue queue) const
//{
//    NVVK_CHECK(vkEndCommandBuffer(cmd));
//
//    VkSubmitInfo submitInfo       = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers    = &cmd;
//
//    NVVK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
//    NVVK_CHECK(vkQueueWaitIdle(queue));
//}
//
//void App::freeTempCommandBuffer(VkCommandBuffer cmd) const
//{
//    vkFreeCommandBuffers(m_device.device(), m_cmd_pool, 1, &cmd);
//}
//
void App::update(float delta_time, float total_time)
{
    //UniformBufferObject ubo{};
    //ubo.model       = glm::rotate(glm::mat4(1.0f), total_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.view        = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.proj        = glm::perspective(glm::radians(45.0f), m_window.getAspectRatio(), 0.1f, 10.0f);
    //ubo.proj[1][1] *= -1;

    //std::memcpy(m_uniform_buffers_mapped[m_curr_frame_idx], &ubo, sizeof(ubo));

    m_gfx.beginFrame();
    m_gfx.drawTestData();
    m_gfx.endFrame();
}

void App::onChar(unsigned int codepoint)
{
    m_kbd.onChar(codepoint);
}

void App::onCursorEnter(int entered)
{
    if (entered == GLFW_TRUE)
    {
        m_mouse.onMouseEnter();
    }
    else
    {
        m_mouse.onMouseLeave();
    }
}

void App::onCursorPos(double xpos, double ypos)
{
    m_mouse.onMouseMove((float)xpos, (float)ypos);
}

void App::onDrop(int path_count, const char* paths[])
{}

void App::onFramebufferSize(int width, int height)
{
    m_window.m_width  = width;
    m_window.m_height = height;


    //vkDeviceWaitIdle(m_device.device());

    //destroyFramebuffer();
    //destroyAttachmentBuffer();
    //m_swapchain.deinit(m_device);


    //m_swapchain.querySwapchainInfo(m_device, width, height);
    //m_swapchain.init(m_device);
    //createAttachmentBuffer();
    //createFramebuffer();
}

void App::onKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        m_kbd.onKeyPressed(key);
    }
    else if (action == GLFW_RELEASE)
    {
        m_kbd.onKeyReleased(key);
    }
}

void App::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_mouse.onLeftPressed();
        }
        else
        {
            m_mouse.onLeftReleased();
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            m_mouse.onRightPressed();
        }
        else
        {
            m_mouse.onRightReleased();
        }
    }
}

void App::onScroll(double xoffset, double yoffset)
{
    m_mouse.onWheelDelta((int)yoffset);
}

void App::onWindowClose()
{
    m_is_running = false;
}

void App::onWindowFocus(int focused)
{
    if (focused == GLFW_TRUE)
    {
        m_is_paused = false;
    }
    else
    {
        m_is_paused = true;
    }
}
