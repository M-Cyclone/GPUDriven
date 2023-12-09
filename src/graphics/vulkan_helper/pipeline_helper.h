#pragma once
#include <vector>

#include <vulkan/vulkan.hpp>

namespace vulkan
{

//--------------------------------------------------------------------------------------------------
/**
\struct nvvk::GraphicsPipelineState

Most graphic pipelines have similar states, therefore the helper `GraphicsPipelineStage` holds all the elements and
initialize the structures with the proper default values, such as the primitive type,
`PipelineColorBlendAttachmentState` with their mask, `DynamicState` for viewport and scissor, adjust depth test if
enabled, line width to 1 pixel, for example.

Example of usage :
\code{.cpp}
nvvk::GraphicsPipelineState pipelineState();
pipelineState.depthStencilState.setDepthTestEnable(true);
pipelineState.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineState.addBindingDescription({0, sizeof(Vertex)});
pipelineState.addAttributeDescriptions ({
    {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
    {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
    {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});
\endcode
*/


struct GraphicsPipelineState
{
    // Initialize the state to common values: triangle list topology, depth test enabled,
    // dynamic viewport and scissor, one render target, blending disabled
    GraphicsPipelineState();

    GraphicsPipelineState(const GraphicsPipelineState& src) = default;

    // Attach the pointer values of the structures to the internal arrays
    void update();

    static VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(
        VkColorComponentFlags colorWriteMask_ = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                                VK_COLOR_COMPONENT_A_BIT,
        VkBool32      blendEnable_         = 0,
        VkBlendFactor srcColorBlendFactor_ = VK_BLEND_FACTOR_ZERO,
        VkBlendFactor dstColorBlendFactor_ = VK_BLEND_FACTOR_ZERO,
        VkBlendOp     colorBlendOp_        = VK_BLEND_OP_ADD,
        VkBlendFactor srcAlphaBlendFactor_ = VK_BLEND_FACTOR_ZERO,
        VkBlendFactor dstAlphaBlendFactor_ = VK_BLEND_FACTOR_ZERO,
        VkBlendOp     alphaBlendOp_        = VK_BLEND_OP_ADD);

    static VkVertexInputBindingDescription makeVertexInputBinding(uint32_t          binding,
                                                                  uint32_t          stride,
                                                                  VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);

    static VkVertexInputAttributeDescription makeVertexInputAttribute(uint32_t location,
                                                                      uint32_t binding,
                                                                      VkFormat format,
                                                                      uint32_t offset);


    void clearBlendAttachmentStates() { blendAttachmentStates.clear(); }
    void setBlendAttachmentCount(uint32_t attachmentCount) { blendAttachmentStates.resize(attachmentCount); }

    void setBlendAttachmentState(uint32_t attachment, const VkPipelineColorBlendAttachmentState& blendState)
    {
        assert(attachment < blendAttachmentStates.size());
        if (attachment <= blendAttachmentStates.size())
        {
            blendAttachmentStates[attachment] = blendState;
        }
    }

    uint32_t addBlendAttachmentState(const VkPipelineColorBlendAttachmentState& blendState)
    {
        blendAttachmentStates.push_back(blendState);
        return (uint32_t)(blendAttachmentStates.size() - 1);
    }

    void clearDynamicStateEnables() { dynamicStateEnables.clear(); }
    void setDynamicStateEnablesCount(uint32_t dynamicStateCount) { dynamicStateEnables.resize(dynamicStateCount); }

    void setDynamicStateEnable(uint32_t state, VkDynamicState dynamicState)
    {
        assert(state < dynamicStateEnables.size());
        if (state <= dynamicStateEnables.size())
        {
            dynamicStateEnables[state] = dynamicState;
        }
    }

    uint32_t addDynamicStateEnable(VkDynamicState dynamicState)
    {
        dynamicStateEnables.push_back(dynamicState);
        return (uint32_t)(dynamicStateEnables.size() - 1);
    }


    void clearBindingDescriptions() { bindingDescriptions.clear(); }
    void setBindingDescriptionsCount(uint32_t bindingDescriptionCount) { bindingDescriptions.resize(bindingDescriptionCount); }
    void setBindingDescription(uint32_t binding, VkVertexInputBindingDescription bindingDescription)
    {
        assert(binding < bindingDescriptions.size());
        if (binding <= bindingDescriptions.size())
        {
            bindingDescriptions[binding] = bindingDescription;
        }
    }

    uint32_t addBindingDescription(const VkVertexInputBindingDescription& bindingDescription)
    {
        bindingDescriptions.push_back(bindingDescription);
        return (uint32_t)(bindingDescriptions.size() - 1);
    }

    void addBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions_)
    {
        bindingDescriptions.insert(bindingDescriptions.end(), bindingDescriptions_.begin(), bindingDescriptions_.end());
    }

    void clearAttributeDescriptions() { attributeDescriptions.clear(); }
    void setAttributeDescriptionsCount(uint32_t attributeDescriptionCount) { attributeDescriptions.resize(attributeDescriptionCount); }

    void setAttributeDescription(uint32_t attribute, const VkVertexInputAttributeDescription& attributeDescription)
    {
        assert(attribute < attributeDescriptions.size());
        if (attribute <= attributeDescriptions.size())
        {
            attributeDescriptions[attribute] = attributeDescription;
        }
    }


    uint32_t addAttributeDescription(const VkVertexInputAttributeDescription& attributeDescription)
    {
        attributeDescriptions.push_back(attributeDescription);
        return (uint32_t)(attributeDescriptions.size() - 1);
    }

    void addAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions_)
    {
        attributeDescriptions.insert(attributeDescriptions.end(), attributeDescriptions_.begin(), attributeDescriptions_.end());
    }


    void clearViewports() { viewports.clear(); }
    void setViewportsCount(uint32_t viewportCount) { viewports.resize(viewportCount); }
    void setViewport(uint32_t attribute, VkViewport viewport)
    {
        assert(attribute < viewports.size());
        if (attribute <= viewports.size())
        {
            viewports[attribute] = viewport;
        }
    }
    uint32_t addViewport(VkViewport viewport)
    {
        viewports.push_back(viewport);
        return (uint32_t)(viewports.size() - 1);
    }


    void clearScissors() { scissors.clear(); }
    void setScissorsCount(uint32_t scissorCount) { scissors.resize(scissorCount); }
    void setScissor(uint32_t attribute, VkRect2D scissor)
    {
        assert(attribute < scissors.size());
        if (attribute <= scissors.size())
        {
            scissors[attribute] = scissor;
        }
    }
    uint32_t addScissor(VkRect2D scissor)
    {
        scissors.push_back(scissor);
        return (uint32_t)(scissors.size() - 1);
    }


    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    VkPipelineMultisampleStateCreateInfo   multisampleState   = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    VkPipelineDepthStencilStateCreateInfo  depthStencilState  = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    VkPipelineViewportStateCreateInfo      viewportState      = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    VkPipelineDynamicStateCreateInfo       dynamicState       = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    VkPipelineColorBlendStateCreateInfo    colorBlendState    = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    VkPipelineVertexInputStateCreateInfo   vertexInputState   = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

private:
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ makePipelineColorBlendAttachmentState() };
    std::vector<VkDynamicState>                      dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    std::vector<VkViewport> viewports;
    std::vector<VkRect2D>   scissors;


    // Helper to set objects for either C and C++
    template <class T, class U>
    void setValue(T& target, const U& val)
    {
        target = (T)(val);
    }
};


//--------------------------------------------------------------------------------------------------
/**
\struct nvvk::GraphicsPipelineGenerator

The graphics pipeline generator takes a GraphicsPipelineState object and pipeline-specific information such as
the render pass and pipeline layout to generate the final pipeline.

Example of usage :
\code{.cpp}
nvvk::GraphicsPipelineState pipelineState();
...
nvvk::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_renderPass, pipelineState);
pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
\endcode
*/

struct GraphicsPipelineGenerator
{
public:
    // For VK_KHR_dynamic_rendering
    using PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo;

public:
    GraphicsPipelineGenerator(GraphicsPipelineState& pipelineState_);
    GraphicsPipelineGenerator(const GraphicsPipelineGenerator& src);
    GraphicsPipelineGenerator(VkDevice                device_,
                              const VkPipelineLayout& layout,
                              const VkRenderPass&     renderPass,
                              GraphicsPipelineState&  pipelineState_);
    GraphicsPipelineGenerator(VkDevice                           device_,
                              const VkPipelineLayout&            layout,
                              const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                              GraphicsPipelineState&             pipelineState_);
    const GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& src)
    {
        device        = src.device;
        pipelineState = src.pipelineState;
        createInfo    = src.createInfo;
        pipelineCache = src.pipelineCache;

        init();
        return *this;
    }
    ~GraphicsPipelineGenerator() { destroyShaderModules(); }

    void setDevice(VkDevice device_) { device = device_; }

    void setRenderPass(VkRenderPass renderPass)
    {
        createInfo.renderPass = renderPass;
        createInfo.pNext      = nullptr;
    }

    void setPipelineRenderingCreateInfo(const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo);

    void setLayout(VkPipelineLayout layout) { createInfo.layout = layout; }

    VkPipelineShaderStageCreateInfo& addShader(const std::string& code, VkShaderStageFlagBits stage, const char* entryPoint = "main");

    template <typename T>
    VkPipelineShaderStageCreateInfo& addShader(const std::vector<T>& code, VkShaderStageFlagBits stage, const char* entryPoint = "main");
    VkPipelineShaderStageCreateInfo& addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entryPoint = "main");

    void clearShaders()
    {
        shaderStages.clear();
        destroyShaderModules();
    }

    VkShaderModule getShaderModule(size_t index) const
    {
        if (index < shaderStages.size())
        {
            return shaderStages[index].module;
        }
        return VK_NULL_HANDLE;
    }

    VkPipeline createPipeline(const VkPipelineCache& cache)
    {
        update();
        VkPipeline pipeline;
        vkCreateGraphicsPipelines(device, cache, 1, (VkGraphicsPipelineCreateInfo*)&createInfo, nullptr, &pipeline);
        return pipeline;
    }

    VkPipeline createPipeline() { return createPipeline(pipelineCache); }

    void destroyShaderModules()
    {
        for (const auto& shaderModule : temporaryModules)
        {
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
        temporaryModules.clear();
    }

    void update()
    {
        createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        createInfo.pStages    = shaderStages.data();
        pipelineState.update();
    }

private:
    void init();

    // Helper to set objects for either C and C++
    template <class T, class U>
    void setValue(T& target, const U& val)
    {
        target = (T)(val);
    }

public:
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

private:
    VkDevice        device;
    VkPipelineCache pipelineCache{};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule>                  temporaryModules;
    std::vector<VkFormat>                        dynamicRenderingColorFormats;
    GraphicsPipelineState&                       pipelineState;
    PipelineRenderingCreateInfo                  dynamicRenderingInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
};

template <typename T>
inline VkPipelineShaderStageCreateInfo& GraphicsPipelineGenerator::addShader(const std::vector<T>& code,
                                                                             VkShaderStageFlagBits stage,
                                                                             const char*           entryPoint)

{
    VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = sizeof(T) * code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    temporaryModules.push_back(shaderModule);

    return addShader(shaderModule, stage, entryPoint);
}


//  class nvvk::GraphicsPipelineGeneratorCombined
//
//  In some cases the application may have each state associated to a single pipeline. For convenience,
//  nvvk::GraphicsPipelineGeneratorCombined combines both the state and generator into a single object.
//
//  Example of usage :
//  {
//      nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
//      pipelineGenerator.depthStencilState.setDepthTestEnable(true);
//      pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
//      pipelineGenerator.addBindingDescription({0, sizeof(Vertex)});
//      pipelineGenerator.addAttributeDescriptions ({
//          {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
//          {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
//          {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});
//
//      pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
//      pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);
//
//      m_pipeline = pipelineGenerator.createPipeline();
//  }
struct GraphicsPipelineGeneratorCombined : public GraphicsPipelineState,
                                           public GraphicsPipelineGenerator
{
    GraphicsPipelineGeneratorCombined(VkDevice device, const VkPipelineLayout& layout, const VkRenderPass& renderPass)
        : GraphicsPipelineState()
        , GraphicsPipelineGenerator(device, layout, renderPass, *this)
    {}
};

}  // namespace vulkan
