#include "graphics/bindable/graphics_pipeline.h"

GraphicsPipeline::GraphicsPipeline(Graphics& gfx)
    : m_dset(getDevice(gfx))
{}

void GraphicsPipeline::reinit(Graphics& gfx, vulkan::GraphicsPipelineGenerator& pgen)
{
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(getDevice(gfx), m_pipeline, nullptr);
    }
    m_pipeline = pgen.createPipeline();
}

void GraphicsPipeline::bind_impl(Graphics& gfx) noexcept
{
    vkCmdBindDescriptorSets(getCurrSwapchainCmd(gfx),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_dset.getPipeLayout(),
                            0,
                            1,
                            m_dset.getSets(getCurrFrameIndex(gfx)),
                            0,
                            nullptr);

    vkCmdBindPipeline(getCurrSwapchainCmd(gfx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void GraphicsPipeline::destroy_impl(Graphics& gfx) noexcept
{
    m_dset.deinit();
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(getDevice(gfx), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}
