//
//  ShaderFactory.h
//  KEngineVulkan
//
//  Created by Kelson Hootman on 7/12/22.
//  Copyright © 2022 Kelson Hootman. All rights reserved.
//

#pragma once

#include "StringHash.h"
#include <map>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

namespace KEngineVulkan {

    class VulkanCore;

    class DataLayout
    {
    public:
        enum class DataType
        {
            ScalarFloat,
            Pad1 = ScalarFloat,
            Vec2Float,
            Pad2 = Vec2Float,
            Vec3Float,
            Vec4Float,
            Mat4Float
        };
        

        struct AttributeBindingLayout
        {
            struct AttributeLayout
            {
                KEngineCore::StringHash name;
                DataType type;
                int location;
            };
            std::vector<AttributeLayout> attributes;
        };

        struct UniformBindingLayout
        {
            bool isVertex;
            bool isFragment;
            bool isSampler;
            bool repeatSampler;
            struct UniformBufferFieldLayout
            {
                KEngineCore::StringHash name;
                DataType type;
            };
            std::vector<UniformBufferFieldLayout> bufferFields;
        };
              

        void Init(KEngineVulkan::VulkanCore * core, const std::vector<AttributeBindingLayout>& attributeBindings, const std::vector<UniformBindingLayout> & uniformBindings);        
        const std::vector<VkVertexInputBindingDescription>& getAttributeBindingDescriptions() const;
        const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;
        const VkDescriptorSetLayout& getDescriptorSetLayout() const;
        VkPipelineLayout getPipelineLayout() const;
    private: 

#ifndef NDEBUG
        bool mDescriptionsGenerated{ false };
#endif
        std::vector<VkVertexInputBindingDescription> mBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> mAttributeDescriptions;
        VkDescriptorSetLayout mDescriptorSetLayout;

        VkPipelineLayout pipelineLayout;
        std::vector<VkSampler> textureSamplers;  //Owned by core
    };


    class ShaderFactory
    {
    public:
        ~ShaderFactory() { Deinit(); }
        void Init(VulkanCore * core);
        void CreatePipeline(KEngineCore::StringHash name, const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename, const DataLayout& dataLayout, bool transparent);
        VkPipeline GetGraphicsPipeline(KEngineCore::StringHash name);
        void ClearModules();
        void Deinit();

        VulkanCore* GetCore() const;


    private:
        VkShaderModule CompileShader(const std::string& shaderFilename);

        std::map<KEngineCore::StringHash, VkShaderModule> mShaderModules;
        std::map<KEngineCore::StringHash, VkPipeline> mGraphicsPipelines;
        VulkanCore * mCore;
    };
}
