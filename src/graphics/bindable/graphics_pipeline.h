#pragma once

#include "graphics/vulkan_helper/descriptorsets_helper.h"
#include "graphics/vulkan_helper/pipeline_helper.h"

#include "graphics/bindable/bindable.h"

class GraphicsPipeline : public Bindable<GraphicsPipeline>
{
    friend class Bindable<GraphicsPipeline>;

public:
    explicit GraphicsPipeline(Graphics& gfx);

public:
    void reinit(Graphics& gfx, vulkan::GraphicsPipelineGenerator& pgen);

    vulkan::DescriptorSetContainer&       getDescriptorSetContainer() { return m_dset; }
    const vulkan::DescriptorSetContainer& getDescriptorSetContainer() const { return m_dset; }

private:
    void bind_impl(Graphics& gfx) noexcept;
    void destroy_impl(Graphics& gfx) noexcept;

private:
    vulkan::DescriptorSetContainer m_dset;
    VkPipeline                     m_pipeline = VK_NULL_HANDLE;
};
