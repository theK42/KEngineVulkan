//
//  ShaderFactory.cpp
//  KEngineVulkan
//
//  Created by Kelson Hootman on 7/12/22.
//  Copyright © 2022 Kelson Hootman. All rights reserved.
//



#include "ShaderFactory.h"
#include "BinaryFile.h"
#include "VulkanCore.h"

#include <assert.h>
#include <string>
#include <array>
#include <stdexcept>

void KEngineVulkan::ShaderFactory::Init(VulkanCore * vulkanCore)
{
    mCore = vulkanCore;
    mShaderModules.clear();
    mGraphicsPipelines.clear();
}

void KEngineVulkan::ShaderFactory::CreatePipeline(KEngineCore::StringHash name, const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename, const DataLayout& dataLayout, bool transparent)
{
    VkPipeline pipeline;

    KEngineCore::StringHash vertexHash(vertexShaderFilename.c_str());
    KEngineCore::StringHash fragmentHash(fragmentShaderFilename.c_str());

    if (mShaderModules.find(vertexHash) == mShaderModules.end()) {
        mShaderModules[vertexHash] = CompileShader(vertexShaderFilename);
    }

    if (mShaderModules.find(fragmentHash) == mShaderModules.end()) {
        mShaderModules[fragmentHash] = CompileShader(fragmentShaderFilename);
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = mShaderModules[vertexHash];
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = mShaderModules[fragmentHash];
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescriptions = dataLayout.getAttributeBindingDescriptions();
    auto attributeDescriptions = dataLayout.getAttributeDescriptions();
 
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.size() > 0 ? &bindingDescriptions[0] : nullptr;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.size() > 0 ? &attributeDescriptions[0] : nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    const bool dynamicViewport = false;
    if (dynamicViewport) {
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
    }

    const VkExtent2D& framebufferExtent = mCore->getFramebufferExtent();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)framebufferExtent.width;
    viewport.height = (float)framebufferExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = framebufferExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f; // check documentation if you want to increase this, requires wideLines

    rasterizer.cullMode = VK_CULL_MODE_NONE;  //VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; //hacky thing because of the projection matrix hack

    rasterizer.depthBiasEnable = VK_FALSE;  //Probably not useful except for shadow mapping?
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional 

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = transparent ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = transparent ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = transparent ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = dataLayout.getPipelineLayout();
    pipelineInfo.renderPass = mCore->getRenderPass();
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional  //Actually don't use this
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(mCore->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    mGraphicsPipelines[name] = pipeline;
}

VkPipeline KEngineVulkan::ShaderFactory::GetGraphicsPipeline(KEngineCore::StringHash name)
{
    assert(mGraphicsPipelines.find(name) != mGraphicsPipelines.end());
    return mGraphicsPipelines[name];
}

void KEngineVulkan::ShaderFactory::ClearModules()
{
    for (auto & modulePair : mShaderModules)
    {
        auto module = modulePair.second;
        vkDestroyShaderModule(mCore->getDevice(), module, nullptr);
    }
}

void KEngineVulkan::ShaderFactory::Deinit()
{
    ClearModules();
    for (auto pipelinePair : mGraphicsPipelines)
    {
        auto& pipeline = pipelinePair.second;
        vkDestroyPipeline(mCore->getDevice(), pipeline, nullptr);

    }
}

KEngineVulkan::VulkanCore* KEngineVulkan::ShaderFactory::GetCore() const {
    return mCore;
}

VkShaderModule KEngineVulkan::ShaderFactory::CompileShader(const std::string& shaderFilename)
{
    KEngineCore::BinaryFile file;
    file.LoadFromFile(shaderFilename, ".spv");
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = file.GetSize();
    createInfo.pCode = file.GetContents<uint32_t>();
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mCore->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

inline const std::vector<VkVertexInputAttributeDescription>& KEngineVulkan::DataLayout::getAttributeDescriptions() const
{
    assert(mDescriptionsGenerated);
    return mAttributeDescriptions;
}

const VkDescriptorSetLayout& KEngineVulkan::DataLayout::getDescriptorSetLayout() const
{
    assert(mDescriptionsGenerated);
    return mDescriptorSetLayout;
}

VkPipelineLayout KEngineVulkan::DataLayout::getPipelineLayout() const
{
    return pipelineLayout;
}

const std::vector<VkVertexInputBindingDescription>& KEngineVulkan::DataLayout::getAttributeBindingDescriptions() const
{
    assert(mDescriptionsGenerated);
    return mBindingDescriptions;
}

void KEngineVulkan::DataLayout::Init(KEngineVulkan::VulkanCore * core, const std::vector<AttributeBindingLayout>& attributeBindings, const std::vector<UniformBindingLayout> & uniformBindings)
{
    mAttributeDescriptions.clear();
    int attributeBindingCount = 0;
    for (auto binding : attributeBindings)
    {
        int offset = 0;
        for (auto & attribute : binding.attributes)
        {
            VkVertexInputAttributeDescription attributeDescription;
            attributeDescription.binding = attributeBindingCount;
            attributeDescription.location = attribute.location;
            attributeDescription.offset = offset * sizeof(float);

            switch (attribute.type)
            {
            case DataType::ScalarFloat:
                attributeDescription.format = VK_FORMAT_R32_SFLOAT;
                offset += 1;
                break;
            case DataType::Vec2Float:
                assert(offset % 2 == 0);
                attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
                offset += 2;
                break;
            case DataType::Vec3Float:
                assert(offset % 4 == 0);
                attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
                offset += 3;
                break;
            case DataType::Vec4Float:
                assert(offset % 4 == 0);
                attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                offset += 4;
                break;
            default:
                assert(false);
            }
            mAttributeDescriptions.push_back(attributeDescription);

        }

        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = attributeBindingCount++;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = offset * sizeof(float);

        mBindingDescriptions.push_back(bindingDescription);

    }

    int uniformBindingCount = 0;
    std::vector<VkDescriptorSetLayoutBinding> uniformBindingDescriptors;
    for (auto uniformBinding : uniformBindings)
    {
        VkDescriptorSetLayoutBinding uniformBindingDescriptor{};
        uniformBindingDescriptor.binding = uniformBindingCount++;
        uniformBindingDescriptor.descriptorCount = 1; // Currently not supporting arrays
        if (uniformBinding.isSampler)
        {
            uniformBindingDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureSamplers.push_back(core->getSampler(uniformBinding.repeatSampler));
        }
        else {
            uniformBindingDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        uniformBindingDescriptor.pImmutableSamplers = nullptr;
        uniformBindingDescriptor.stageFlags = 
            (uniformBinding.isVertex ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
            (uniformBinding.isFragment ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
#ifndef NDEBUG
        int offset = 0;
        for (auto & uniformBufferField : uniformBinding.bufferFields)
        {
            switch (uniformBufferField.type)
            {
            case DataType::ScalarFloat:
                offset += 1;
                break;
            case DataType::Vec2Float:
                assert(offset % 2 == 0);
                offset += 2;
                break;
            case DataType::Vec3Float:
                assert(offset % 4 == 0);
                offset += 3;
                break;
            case DataType::Vec4Float:
                assert(offset % 4 == 0);
                offset += 4;
                break;
            case DataType::Mat4Float:
                assert(offset % 4 == 0);
                offset += 16;
                break;
            }
        }
#endif
        uniformBindingDescriptors.push_back(uniformBindingDescriptor);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(uniformBindingDescriptors.size());
    layoutInfo.pBindings = &uniformBindingDescriptors[0];

    if (vkCreateDescriptorSetLayout(core->getDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(core->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

#ifndef NDEBUG
    mDescriptionsGenerated = true;
#endif
}
