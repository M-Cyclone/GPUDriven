#include "graphics/vulkan_helper/pipeline_helper.h"

namespace vulkan
{

// Initialize the state to common values: triangle list topology, depth test enabled,
// dynamic viewport and scissor, one render target, blending disabled
GraphicsPipelineState::GraphicsPipelineState()
{
    rasterizationState.flags                   = {};
    rasterizationState.depthClampEnable        = {};
    rasterizationState.rasterizerDiscardEnable = {};
    setValue(rasterizationState.polygonMode, VK_POLYGON_MODE_FILL);
    setValue(rasterizationState.cullMode, VK_CULL_MODE_BACK_BIT);
    setValue(rasterizationState.frontFace, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    rasterizationState.depthBiasEnable         = {};
    rasterizationState.depthBiasConstantFactor = {};
    rasterizationState.depthBiasClamp          = {};
    rasterizationState.depthBiasSlopeFactor    = {};
    rasterizationState.lineWidth               = 1.f;

    inputAssemblyState.flags = {};
    setValue(inputAssemblyState.topology, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    inputAssemblyState.primitiveRestartEnable = {};


    colorBlendState.flags         = {};
    colorBlendState.logicOpEnable = {};
    setValue(colorBlendState.logicOp, VK_LOGIC_OP_CLEAR);
    colorBlendState.attachmentCount = {};
    colorBlendState.pAttachments    = {};
    for (int i = 0; i < 4; i++)
    {
        colorBlendState.blendConstants[i] = 0.f;
    }


    dynamicState.flags             = {};
    dynamicState.dynamicStateCount = {};
    dynamicState.pDynamicStates    = {};


    vertexInputState.flags                           = {};
    vertexInputState.vertexBindingDescriptionCount   = {};
    vertexInputState.pVertexBindingDescriptions      = {};
    vertexInputState.vertexAttributeDescriptionCount = {};
    vertexInputState.pVertexAttributeDescriptions    = {};


    viewportState.flags         = {};
    viewportState.viewportCount = {};
    viewportState.pViewports    = {};
    viewportState.scissorCount  = {};
    viewportState.pScissors     = {};


    depthStencilState.flags            = {};
    depthStencilState.depthTestEnable  = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    setValue(depthStencilState.depthCompareOp, VK_COMPARE_OP_LESS_OR_EQUAL);
    depthStencilState.depthBoundsTestEnable = {};
    depthStencilState.stencilTestEnable     = {};
    setValue(depthStencilState.front, VkStencilOpState());
    setValue(depthStencilState.back, VkStencilOpState());
    depthStencilState.minDepthBounds = {};
    depthStencilState.maxDepthBounds = {};

    setValue(multisampleState.rasterizationSamples, VK_SAMPLE_COUNT_1_BIT);
}

// Attach the pointer values of the structures to the internal arrays
void GraphicsPipelineState::update()
{
    colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
    dynamicState.pDynamicStates    = dynamicStateEnables.data();

    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputState.pVertexBindingDescriptions      = bindingDescriptions.data();
    vertexInputState.pVertexAttributeDescriptions    = attributeDescriptions.data();

    if (viewports.empty())
    {
        viewportState.viewportCount = 1;
        viewportState.pViewports    = nullptr;
    }
    else
    {
        viewportState.viewportCount = (uint32_t)viewports.size();
        viewportState.pViewports    = viewports.data();
    }

    if (scissors.empty())
    {
        viewportState.scissorCount = 1;
        viewportState.pScissors    = nullptr;
    }
    else
    {
        viewportState.scissorCount = (uint32_t)scissors.size();
        viewportState.pScissors    = scissors.data();
    }
}

VkPipelineColorBlendAttachmentState GraphicsPipelineState::makePipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask_,
                                                                                                 VkBool32              blendEnable_,
                                                                                                 VkBlendFactor         srcColorBlendFactor_,
                                                                                                 VkBlendFactor         dstColorBlendFactor_,
                                                                                                 VkBlendOp             colorBlendOp_,
                                                                                                 VkBlendFactor         srcAlphaBlendFactor_,
                                                                                                 VkBlendFactor         dstAlphaBlendFactor_,
                                                                                                 VkBlendOp             alphaBlendOp_)
{
    VkPipelineColorBlendAttachmentState res;

    res.blendEnable         = blendEnable_;
    res.srcColorBlendFactor = srcColorBlendFactor_;
    res.dstColorBlendFactor = dstColorBlendFactor_;
    res.colorBlendOp        = colorBlendOp_;
    res.srcAlphaBlendFactor = srcAlphaBlendFactor_;
    res.dstAlphaBlendFactor = dstAlphaBlendFactor_;
    res.alphaBlendOp        = alphaBlendOp_;
    res.colorWriteMask      = colorWriteMask_;
    return res;
}

VkVertexInputBindingDescription GraphicsPipelineState::makeVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate)
{
    VkVertexInputBindingDescription vertexBinding;
    vertexBinding.binding   = binding;
    vertexBinding.inputRate = rate;
    vertexBinding.stride    = stride;
    return vertexBinding;
}

VkVertexInputAttributeDescription GraphicsPipelineState::makeVertexInputAttribute(uint32_t location,
                                                                                  uint32_t binding,
                                                                                  VkFormat format,
                                                                                  uint32_t offset)
{
    VkVertexInputAttributeDescription attrib;
    attrib.binding  = binding;
    attrib.location = location;
    attrib.format   = format;
    attrib.offset   = offset;
    return attrib;
}

GraphicsPipelineGenerator::GraphicsPipelineGenerator(GraphicsPipelineState& pipelineState_)
    : pipelineState(pipelineState_)
{
    init();
}

GraphicsPipelineGenerator::GraphicsPipelineGenerator(const GraphicsPipelineGenerator& src)
    : createInfo(src.createInfo)
    , device(src.device)
    , pipelineCache(src.pipelineCache)
    , pipelineState(src.pipelineState)
{
    init();
}

GraphicsPipelineGenerator::GraphicsPipelineGenerator(VkDevice                device_,
                                                     const VkPipelineLayout& layout,
                                                     const VkRenderPass&     renderPass,
                                                     GraphicsPipelineState&  pipelineState_)
    : device(device_)
    , pipelineState(pipelineState_)
{
    createInfo.layout     = layout;
    createInfo.renderPass = renderPass;
    init();
}

GraphicsPipelineGenerator::GraphicsPipelineGenerator(VkDevice                           device_,
                                                     const VkPipelineLayout&            layout,
                                                     const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                                                     GraphicsPipelineState&             pipelineState_)
    : device(device_)
    , pipelineState(pipelineState_)
{
    createInfo.layout = layout;
    setPipelineRenderingCreateInfo(pipelineRenderingCreateInfo);
    init();
}

void GraphicsPipelineGenerator::setPipelineRenderingCreateInfo(const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo)
{
    // Deep copy
    assert(pipelineRenderingCreateInfo.pNext == nullptr);  // Update deep copy if needed.
    dynamicRenderingInfo = pipelineRenderingCreateInfo;
    if (dynamicRenderingInfo.colorAttachmentCount != 0)
    {
        dynamicRenderingColorFormats.assign(dynamicRenderingInfo.pColorAttachmentFormats,
                                            dynamicRenderingInfo.pColorAttachmentFormats + dynamicRenderingInfo.colorAttachmentCount);
        dynamicRenderingInfo.pColorAttachmentFormats = dynamicRenderingColorFormats.data();
    }

    // Set VkGraphicsPipelineCreateInfo::pNext to point to deep copy of extension struct.
    // NB: Will have to change if more than 1 extension struct needs to be supported.
    createInfo.pNext = &dynamicRenderingInfo;
}

VkPipelineShaderStageCreateInfo& GraphicsPipelineGenerator::addShader(const std::string&    code,
                                                                      VkShaderStageFlagBits stage,
                                                                      const char*           entryPoint)
{
    std::vector<char> v;
    std::copy(code.begin(), code.end(), std::back_inserter(v));
    return addShader(v, stage, entryPoint);
}

VkPipelineShaderStageCreateInfo& GraphicsPipelineGenerator::addShader(VkShaderModule        shaderModule,
                                                                      VkShaderStageFlagBits stage,
                                                                      const char*           entryPoint)
{
    VkPipelineShaderStageCreateInfo shaderStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStage.stage  = (VkShaderStageFlagBits)stage;
    shaderStage.module = shaderModule;
    shaderStage.pName  = entryPoint;

    shaderStages.push_back(shaderStage);
    return shaderStages.back();
}

void GraphicsPipelineGenerator::init()
{
    createInfo.pRasterizationState = &pipelineState.rasterizationState;
    createInfo.pInputAssemblyState = &pipelineState.inputAssemblyState;
    createInfo.pColorBlendState    = &pipelineState.colorBlendState;
    createInfo.pMultisampleState   = &pipelineState.multisampleState;
    createInfo.pViewportState      = &pipelineState.viewportState;
    createInfo.pDepthStencilState  = &pipelineState.depthStencilState;
    createInfo.pDynamicState       = &pipelineState.dynamicState;
    createInfo.pVertexInputState   = &pipelineState.vertexInputState;
}

}  // namespace vulkan
