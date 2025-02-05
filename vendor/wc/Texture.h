#pragma once

#include <filesystem>
#include "imgui_backend/imgui_impl_vulkan.h"
#include <stb_image/stb_image.h>
#include <wc/vk/Image.h>
#include <wc/vk/SyncContext.h>

namespace wc 
{
    struct TextureSpecification
    {
        // Image
		VkFormat                 format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32_t                 width = 1;
		uint32_t                 height = 1;
        bool                     mipMapping = false;
		VkImageUsageFlags        usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        // Sampler
		vk::Filter                magFilter = vk::Filter::NEAREST;
		vk::Filter                minFilter = vk::Filter::NEAREST;
		vk::SamplerMipmapMode     mipmapMode = vk::SamplerMipmapMode::NEAREST;
		vk::SamplerAddressMode    addressModeU = vk::SamplerAddressMode::REPEAT;
		vk::SamplerAddressMode    addressModeV = vk::SamplerAddressMode::REPEAT;
		vk::SamplerAddressMode    addressModeW = vk::SamplerAddressMode::REPEAT;
    };

    class Texture 
    {
        vk::Image image;
        vk::ImageView view;
        vk::Sampler sampler;
        VkDescriptorSet imageID = VK_NULL_HANDLE;
    public:

		void Allocate(const TextureSpecification& specification)
		{
			vk::ImageSpecification imageSpecification = 
            {
			    .format = specification.format,

			    .width = specification.width,
			    .height = specification.height,

			    .mipLevels = specification.mipMapping ? vk::GetMipLevelCount(glm::vec2(specification.width, specification.height)) : 1,
			    .usage = specification.usage,
            };

			image.Create(imageSpecification);

			view.Create(image);

			vk::SamplerSpecification samplerSpec = 
            {
			    .addressModeU = specification.addressModeU,
                .addressModeV = specification.addressModeV,
                .addressModeW = specification.addressModeW,
			    .maxLod = (float)image.mipLevels,
			    .magFilter = specification.magFilter,
			    .minFilter = specification.minFilter,
			    .mipmapMode = specification.mipmapMode,
            };

			if (specification.mipMapping && VulkanContext::GetPhysicalDevice().GetFeatures().samplerAnisotropy)
			{
				samplerSpec.anisotropyEnable = true;
				samplerSpec.maxAnisotropy = VulkanContext::GetPhysicalDevice().GetLimits().maxSamplerAnisotropy;
			}

			sampler.Create(samplerSpec);
			imageID = MakeImGuiDescriptor(imageID, { .sampler = sampler, .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		}

        void Allocate(uint32_t width, uint32_t height, bool mipMapping = false)
        {
            TextureSpecification texSpec = {
                .width = width,
                .height = height,
                .mipMapping = mipMapping,
            };
            
            if (width <= 128 || height <= 128) 
            {
                texSpec.magFilter = vk::Filter::NEAREST;
                texSpec.minFilter = vk::Filter::NEAREST;
                texSpec.mipmapMode = vk::SamplerMipmapMode::NEAREST;
            }
            else 
            {
                texSpec.magFilter = vk::Filter::LINEAR;
                texSpec.minFilter = vk::Filter::LINEAR;
                texSpec.mipmapMode = vk::SamplerMipmapMode::LINEAR;
            }
            Allocate(texSpec);
        }

		void Load(const void* data, uint32_t width, uint32_t height, bool mipMapping = false)
		{
			Allocate(width, height, mipMapping);
			SetData(data, width, height, 0, 0, mipMapping);
		}

        void Load(const std::string& filepath, bool mipMapping = false)
        {
            int32_t width = 0, height = 0, fnrComponents;
            auto data = stbi_load(filepath.c_str(), &width, &height, &fnrComponents, 4);
            
            if (data)
            {
                Load(data, width, height, mipMapping);
                SetName(filepath);
            }
            else
                WC_CORE_ERROR("Could not find file at location {}", filepath);

            stbi_image_free(data);
        }        

        void SetData(const void* data, uint32_t width, uint32_t height, uint32_t offsetX = 0, uint32_t offsetY = 0, bool mipMapping = false)
        {
            VkDeviceSize imageSize = width * height * 4;

            vk::StagingBuffer stagingBuffer;
            stagingBuffer.Allocate(imageSize);
            stagingBuffer.SetData(data, imageSize);

            VkImageSubresourceRange subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            };

            vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
                image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkBufferImageCopy copyRegion = {
                    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .imageSubresource.layerCount = 1,
                    .imageExtent = {
                        .width = width,
                        .height = height,
                        .depth = 1
                    },
                    .imageOffset =
                    {
                        .x = (int32_t)offsetX,
                        .y = (int32_t)offsetY,
                        .z = 0,
                    },
                };

                vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                if (mipMapping)
                {
                    image.InsertMemoryBarrier(
                        cmd,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        subresourceRange);

                    // Copy down mips from n-1 to n
                    for (uint32_t i = 1; i < image.mipLevels; i++)
                    {
                        VkImageBlit imageBlit{};

                        // Source
                        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageBlit.srcSubresource.layerCount = 1;
                        imageBlit.srcSubresource.mipLevel = i - 1;
                        imageBlit.srcSubresource.baseArrayLayer = 0;
                        imageBlit.srcOffsets[1].x = int32_t(image.width >> (i - 1));
                        imageBlit.srcOffsets[1].y = int32_t(image.height >> (i - 1));
                        imageBlit.srcOffsets[1].z = 1;

                        // Destination
                        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageBlit.dstSubresource.layerCount = 1;
                        imageBlit.dstSubresource.mipLevel = i;
                        imageBlit.dstSubresource.baseArrayLayer = 0;
                        imageBlit.dstOffsets[1].x = int32_t(image.width >> i);
                        imageBlit.dstOffsets[1].y = int32_t(image.height >> i);
                        imageBlit.dstOffsets[1].z = 1;

                        VkImageSubresourceRange mipSubRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = i,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        };

                        // Prepare current mip level as image blit destination
                        image.InsertMemoryBarrier(
                            cmd, 0,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            mipSubRange);

                        // Blit from previous level
                        vkCmdBlitImage(
                            cmd,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1,
                            &imageBlit,
                            VK_FILTER_LINEAR);

                        // Prepare current mip level as image blit source for next level
                        image.InsertMemoryBarrier(
                            cmd,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_TRANSFER_READ_BIT,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            mipSubRange);
                    }

                    // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
                    subresourceRange.levelCount = image.mipLevels;
                    image.InsertMemoryBarrier(
                        cmd,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        subresourceRange);
                }
                else
                    image.SetLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                });

            stagingBuffer.Free();
        }

        void MakeRenderable()
        {
            vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
                    image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                });
        }

        void Destroy() 
        {
            image.Destroy();
            view.Destroy();
            sampler.Destroy();
        }

        glm::ivec2 GetSize() { return { image.width, image.height }; }

        vk::ImageView GetView() const { return view; }
        vk::Sampler GetSampler() const { return sampler; }
        vk::Image GetImage() const { return image; }
        auto GetImageID() const { return imageID; }

        void SetName(const std::string& name) 
        {
            view.SetName(name + "_view");
            sampler.SetName(name + "_sampler");
            image.SetName(name + "_image");
        }

        operator ImTextureID () const { return (ImTextureID)imageID; }
    };
}