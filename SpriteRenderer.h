//
//  SpriteRenderer.h
//  KEngineVulkan
//
//  Created by Kelson Hootman on 7/8/22.
//  Copyright © 2022 Kelson Hootman. All rights reserved.
//

#pragma once

#include "StringHash.h"
#include "Transform2D.h"
#include "Renderer2D.h"
#include "ShaderFactory.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <list>


namespace KEngineVulkan
{
    class VulkanCore;
    class DataLayout;
    struct Sprite
    {
        int width;
        int height;
        VkPipeline graphicsPipeline; // Owned by ShaderFactory
        const DataLayout * mLayout;

        std::pair<VkBuffer, VmaAllocation> vertexBuffer;
        std::pair<VkBuffer, VmaAllocation> indexBuffer;
        int indexCount;

        VkImageView textureImageView{ VK_NULL_HANDLE }; // Owned by TextureFactory
        VkSampler   textureSampler{ VK_NULL_HANDLE }; // Owned by VulkanCore
    };

    class SpriteRenderer;

    class SpriteGraphic
    {
    public:
        SpriteGraphic();
        ~SpriteGraphic();
        void Init(SpriteRenderer* renderer, Sprite const* sprite, KEngine2D::Transform const* transform);
        void Deinit();
        void createDescriptorSets(KEngineVulkan::VulkanCore* core, const KEngineVulkan::Sprite* sprite);
        void updateUniformBuffer(int currentFrame, const KEngine2D::Matrix & projectionMatrix);
        Sprite const* GetSprite() const;
        void SetSprite(Sprite const* sprite);
        KEngine2D::Transform const* GetTransform() const;
        VkDescriptorSet GetDescriptorSet(int currentFrame) const;

    protected:
        Sprite const* mSprite;
        KEngine2D::Transform const* mTransform;
        SpriteRenderer* mRenderer;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<std::pair<VkBuffer, VmaAllocation>> uniformBuffers;
    };

    class SpriteRenderer : public KEngine2D::Renderer
    {
    public:
        SpriteRenderer();
        ~SpriteRenderer();
        void Init(VulkanCore * core, int width, int height);
        void Deinit();
        void Render() const;
        void AddToRenderList(SpriteGraphic* cursesGraphic);
        void RemoveFromRenderList(SpriteGraphic* cursesGraphic);
        int GetWidth() const;
        int GetHeight() const;
        VulkanCore * GetCore() const;
    protected:

        VulkanCore*                   mCore;
        std::list<SpriteGraphic*>     mRenderList;
        bool                          mInitialized;
        int                           mWidth;
        int                           mHeight;
        KEngine2D::Matrix             mProjection;

    };
}
