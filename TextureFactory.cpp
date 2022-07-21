#include "TextureFactory.h"
#include "VulkanCore.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>


void KEngineVulkan::TextureFactory::Init(VulkanCore* core)
{
	mCore = core;
	mTextures.clear();
}

void KEngineVulkan::TextureFactory::Deinit()
{
    for (auto & texturePair : mTextures) {
        auto & textureStruct = texturePair.second;
        vkDestroyImageView(mCore->getDevice(), textureStruct.textureImageView, nullptr);
        vmaDestroyImage(mCore->getAllocator(), textureStruct.textureImage, textureStruct.textureImageAllocation);
    }
    mTextures.clear();
}

void KEngineVulkan::TextureFactory::CreateTexture(KEngineCore::StringHash name, const std::string& textureFilename)
{
    Texture texture;
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(textureFilename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = (int64_t)texWidth * (int64_t)texHeight * 4ll;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    mCore->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

    void* data;

    vmaMapMemory(mCore->getAllocator(), stagingBufferAllocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(mCore->getAllocator(), stagingBufferAllocation);

    stbi_image_free(pixels);

    mCore->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture.textureImage, texture.textureImageAllocation);

    //These three functions should combine their command buffers in practice
    mCore->transitionImageLayout(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    mCore->copyBufferToImage(stagingBuffer, texture.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    mCore->transitionImageLayout(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vmaDestroyBuffer(mCore->getAllocator(), stagingBuffer, stagingBufferAllocation);
  
    texture.textureImageView = mCore->createImageView(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB);


    mTextures[name] = texture;
}

VkImageView KEngineVulkan::TextureFactory::GetTexture(KEngineCore::StringHash name) const
{
    return mTextures.find(name)->second.textureImageView;
}