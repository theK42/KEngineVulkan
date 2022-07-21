//
//  TextureFactory.h
//  KEngineVulkan
//
//  Created by Kelson Hootman on 7/14/22.
//  Copyright © 2022 Kelson Hootman. All rights reserved.
//


#pragma once
#include <string>
#include <map>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "StringHash.h"

namespace KEngineVulkan {
	class VulkanCore;

	class TextureFactory
	{
	public:
		~TextureFactory() { Deinit(); }
		void Init(VulkanCore * core);
		void CreateTexture(KEngineCore::StringHash name, const std::string& textureFilename);
		VkImageView GetTexture(KEngineCore::StringHash name) const;
		void Deinit();
	private:

		struct Texture
		{
			VkImage textureImage;
			VkImageView textureImageView;
			VmaAllocation textureImageAllocation;
		};

		VulkanCore* mCore;
		std::map<KEngineCore::StringHash, Texture> mTextures;
	};
}
