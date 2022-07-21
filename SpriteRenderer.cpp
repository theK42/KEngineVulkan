//
//  SpriteRenderer.cpp
//  KEngineOpenGL
//
//  Created by Kelson Hootman on 7/8/22.
//  Copyright © 2022 Kelson Hootman. All rights reserved.
//

#include "SpriteRenderer.h"
#include "ShaderFactory.h"
#include "VulkanCore.h"
#include <cassert>
#include <stdexcept>


#undef near
#undef far

KEngineVulkan::SpriteGraphic::SpriteGraphic()
{
    mRenderer = nullptr;
    mTransform = nullptr;
    mSprite = nullptr;
}

KEngineVulkan::SpriteGraphic::~SpriteGraphic()
{
    Deinit();
}

void KEngineVulkan::SpriteGraphic::Init(SpriteRenderer* renderer, Sprite const* sprite, KEngine2D::Transform const* transform)
{
    assert(transform != nullptr);
    assert(renderer != nullptr);
    mRenderer = renderer;
    mSprite = sprite;
    mTransform = transform;
    renderer->AddToRenderList(this);

    VkDeviceSize bufferSize = 2 * sizeof(KEngine2D::Matrix); //This is gross hack, improve please
    int maxFramesInFlight = renderer->GetCore()->getMaxFramesInFlight();
    uniformBuffers.resize(maxFramesInFlight);

    for (int i = 0; i < maxFramesInFlight; i++) {
        renderer->GetCore()->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, uniformBuffers[i].first, uniformBuffers[i].second);
    }

    createDescriptorSets(renderer->GetCore(), sprite);
}

void KEngineVulkan::SpriteGraphic::Deinit()
{
    VulkanCore* core = mRenderer->GetCore();

    for (auto& bufferPair : uniformBuffers) {
        vmaDestroyBuffer(core->getAllocator(), bufferPair.first, bufferPair.second);
    }
    vkFreeDescriptorSets(core->getDevice(), core->getDescriptorPool(), descriptorSets.size(), &descriptorSets[0]);
    
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<std::pair<VkBuffer, VmaAllocation>> uniformBuffers;
}

void KEngineVulkan::SpriteGraphic::createDescriptorSets(KEngineVulkan::VulkanCore* core, const KEngineVulkan::Sprite* sprite)
{
    int maxFramesInFlight = core->getMaxFramesInFlight();

    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, sprite->mLayout->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = core->getDescriptorPool();
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();
    descriptorSets.resize(maxFramesInFlight);
    VkResult result = vkAllocateDescriptorSets(core->getDevice(), &allocInfo, descriptorSets.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].first;
        bufferInfo.offset = 0;
        bufferInfo.range = 2 * sizeof(KEngine2D::Matrix); // To do:  read this so it doesn't get out of sync

        std::vector< VkWriteDescriptorSet> descriptorWrites;

        VkWriteDescriptorSet uniformBufferWrite{};
        uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformBufferWrite.dstSet = descriptorSets[i];
        uniformBufferWrite.dstBinding = 0;
        uniformBufferWrite.dstArrayElement = 0;
        uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferWrite.descriptorCount = 1;
        uniformBufferWrite.pBufferInfo = &bufferInfo;

        descriptorWrites.push_back(uniformBufferWrite);

        if (sprite->textureImageView != VK_NULL_HANDLE)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = sprite->textureImageView;
            imageInfo.sampler = sprite->textureSampler;

            VkWriteDescriptorSet samplerWrite{};
            samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWrite.dstSet = descriptorSets[i];
            samplerWrite.dstBinding = 1;
            samplerWrite.dstArrayElement = 0;
            samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerWrite.descriptorCount = 1;
            samplerWrite.pImageInfo = &imageInfo;

            descriptorWrites.push_back(samplerWrite);
        }

        vkUpdateDescriptorSets(core->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), &descriptorWrites[0], 0, nullptr);
    }
}

void KEngineVulkan::SpriteGraphic::updateUniformBuffer(int currentFrame, const KEngine2D::Matrix & projectionMatrix)
{
    struct Ubo {
        KEngine2D::Matrix model;
        KEngine2D::Matrix projection;
    };
    Ubo ubo{ mTransform->GetAsMatrix(), projectionMatrix };
    
    void* data;
    VmaAllocator allocator = mRenderer->GetCore()->getAllocator();
    vmaMapMemory(allocator, uniformBuffers[currentFrame].second, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(allocator, uniformBuffers[currentFrame].second);
}

KEngineVulkan::Sprite const* KEngineVulkan::SpriteGraphic::GetSprite() const
{
    return mSprite;
}

void KEngineVulkan::SpriteGraphic::SetSprite(const KEngineVulkan::Sprite* sprite)
{
    assert(mTransform != nullptr); /// Initialized
    mSprite = sprite;
}

KEngine2D::Transform const* KEngineVulkan::SpriteGraphic::GetTransform() const
{
    assert(mTransform != nullptr);
    return mTransform;
}

VkDescriptorSet KEngineVulkan::SpriteGraphic::GetDescriptorSet(int currentFrame) const
{
    return descriptorSets[currentFrame];
}

KEngineVulkan::SpriteRenderer::SpriteRenderer()
{
    mInitialized = false;
}

KEngineVulkan::SpriteRenderer::~SpriteRenderer()
{
    Deinit();
}

void KEngineVulkan::SpriteRenderer::Init(VulkanCore* core, int width, int height)
{
    assert(!mInitialized);
    assert(core != nullptr);
    
    mCore = core;
    mWidth = width;
    mHeight = height;

    float left = 0.0f;
    float right = (float)width;
    float top = 0.0f;
    float bottom = (float)height;
    float near = -1.0f;
    float far = 1.0f;

    mProjection.data[0][0] = 2.0f / (right - left);
    mProjection.data[0][1] = 0.0f;
    mProjection.data[0][2] = 0.0f;
    mProjection.data[0][3] = 0.0f;

    mProjection.data[1][0] = 0.0f;
    mProjection.data[1][1] = -2.0f / (top - bottom);
    mProjection.data[1][2] = 0.0f;
    mProjection.data[1][3] = 0.0f;

    mProjection.data[2][0] = 0.0f;
    mProjection.data[2][1] = 0.0f;
    mProjection.data[2][2] = -2.0f / (far - near);
    mProjection.data[2][3] = 0.0f;

    mProjection.data[3][0] = -(right + left) / (right - left);
    mProjection.data[3][1] = (top + bottom) / (top - bottom);
    mProjection.data[3][2] = -(far + near) / (far - near);
    mProjection.data[3][3] = 1.0f;


    mInitialized = true;
}

void KEngineVulkan::SpriteRenderer::Deinit()
{
    mInitialized = false;
    mRenderList.clear();
}

void KEngineVulkan::SpriteRenderer::Render() const
{
    assert(mInitialized);

    bool selfStarter = !mCore->inRenderPass();
    if (selfStarter)
    {
        mCore->startFrame();
    }

    int currentFrame = mCore->getCurrentFrame();
    assert(currentFrame >= 0);
    VkCommandBuffer commandBuffer = mCore->getCommandBuffer();
   
    for (SpriteGraphic* graphic : mRenderList)
    {

        const Sprite* sprite = graphic->GetSprite();
        graphic->updateUniformBuffer(currentFrame, mProjection);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,sprite->graphicsPipeline);
        VkBuffer vertexBuffers[] = { sprite->vertexBuffer.first };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, sprite->indexBuffer.first, 0, VK_INDEX_TYPE_UINT16);
        VkDescriptorSet descriptorSet = graphic->GetDescriptorSet(currentFrame);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sprite->mLayout->getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sprite->indexCount), 1, 0, 0, 0);
    }

    if (selfStarter)
    {
        mCore->endFrame();
    }
}

void KEngineVulkan::SpriteRenderer::AddToRenderList(SpriteGraphic* spriteGraphic)
{
    assert(mInitialized);
    mRenderList.push_back(spriteGraphic);
}

void KEngineVulkan::SpriteRenderer::RemoveFromRenderList(SpriteGraphic* spriteGraphic)
{
    assert(mInitialized);
    mRenderList.remove(spriteGraphic);
}

int KEngineVulkan::SpriteRenderer::GetWidth() const {
    return mWidth;
}

int KEngineVulkan::SpriteRenderer::GetHeight() const {
    return mHeight;
}

KEngineVulkan::VulkanCore* KEngineVulkan::SpriteRenderer::GetCore() const {
    return mCore;
}