#pragma once

#include "Buffer.h"
#include <glm/glm.hpp>
#undef min

namespace vk 
{
	inline glm::ivec2 GetMipSize(uint32_t level, glm::ivec2 size) { return { size.x >> level, size.y >> level }; }

    inline uint32_t GetMipLevelCount(glm::vec2 size) { return (uint32_t)glm::floor(glm::log2(glm::min(size.x, size.y))); }

	inline auto GenerateImageMemoryBarier(
        VkImage image,
        VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange)
    {
		VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barier.oldLayout = oldImageLayout;
		barier.newLayout = newImageLayout;
		barier.image = image;
		barier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			barier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			barier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			barier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			barier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			barier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			barier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			barier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			barier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			barier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			barier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			barier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (barier.srcAccessMask == 0)
				barier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

			barier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}
        return barier;
    }

    struct ImageSpecification
    {
        VkFormat                 format = VK_FORMAT_UNDEFINED;
        uint32_t                 width = 1;
        uint32_t                 height = 1;
        uint32_t                 mipLevels = 1;
        VkImageUsageFlags        usage = 0;

		bool operator==(const ImageSpecification& other) const
        {
			return format == other.format &&
				width == other.width &&
				height == other.height &&
				mipLevels == other.mipLevels &&
				usage == other.usage;
		}
    };

    class Image : public VkObject<VkImage> 
    {
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
    public:
        uint32_t width = 0, height = 0;
        uint32_t mipLevels = 1;
        uint32_t layers = 1;
        VkFormat format = VK_FORMAT_UNDEFINED;

        Image() = default;
		Image(VkImage img, VmaAllocation alloc = VK_NULL_HANDLE) 
        {
            m_Handle = img; 
            m_Allocation = alloc;
		}

        VkResult Create(const VkImageCreateInfo& dimg_info, VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY, VkMemoryPropertyFlags requiredFlags = 0)
        {
            VmaAllocationCreateInfo dimg_allocinfo = {
                .usage = usage,
                .requiredFlags = requiredFlags,
            };

            width = dimg_info.extent.width;
            height = dimg_info.extent.height;
            mipLevels = dimg_info.mipLevels;
            layers = dimg_info.arrayLayers;
            format = dimg_info.format;

            return vmaCreateImage(VulkanContext::GetMemoryAllocator(), &dimg_info, &dimg_allocinfo, &m_Handle, &m_Allocation, nullptr);
        }

        VkResult Create(const ImageSpecification& imageSpec, VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY, VkMemoryPropertyFlags requiredFlags = 0)
        {
			return Create({
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,

				.imageType = VK_IMAGE_TYPE_2D,

				.format = imageSpec.format,

				.extent = {
					.width = imageSpec.width,
					.height = imageSpec.height,
					.depth = 1,
				},

				.mipLevels = imageSpec.mipLevels,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = imageSpec.usage
				}, usage, requiredFlags);
        }

        void Destroy() 
        {
            vmaDestroyImage(VulkanContext::GetMemoryAllocator(), m_Handle, m_Allocation);
            m_Handle = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }

        glm::ivec2 GetMipSize(uint32_t level) const { return { width >> level, height >> level }; }

        glm::ivec2 GetSize() const { return { width, height }; }
        float GetAspectRatio() const { return (float)width / (float)height; }

        uint32_t GetMipLevelCount() { return (uint32_t)glm::floor(glm::log2((float)glm::min(width, height))); }

        void InsertMemoryBarrier(
            VkCommandBuffer cmd,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkImageSubresourceRange subresourceRange)
        {
            VkImageMemoryBarrier imageMemoryBarrier = { 
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .srcAccessMask = srcAccessMask,
                .dstAccessMask = dstAccessMask,
                .oldLayout = oldImageLayout,
                .newLayout = newImageLayout,
                .image = m_Handle,
                .subresourceRange = subresourceRange,
            };

            vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        void InsertMemoryBarrier(
            VkCommandBuffer cmd,
            VkImageAspectFlags aspectMask,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask)
        {
			InsertMemoryBarrier(cmd, srcAccessMask, dstAccessMask, oldImageLayout, newImageLayout, srcStageMask, dstStageMask, {
				.aspectMask = aspectMask,
				.levelCount = mipLevels,
				.layerCount = layers,
			});
        }

        void SetLayout(
            VkCommandBuffer cmd,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkImageSubresourceRange subresourceRange,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            VkImageMemoryBarrier imageMemoryBarrier = GenerateImageMemoryBarier(m_Handle, oldImageLayout, newImageLayout, subresourceRange);

            vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        // Fixed sub resource on first mip level and layer
        void SetLayout(
            const VkCommandBuffer& cmd,
            const VkImageAspectFlags& aspectMask,
            const VkImageLayout& oldImageLayout,
            const VkImageLayout& newImageLayout,
            const VkPipelineStageFlags& srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            const VkPipelineStageFlags& dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            VkImageSubresourceRange subresourceRange = {
                .aspectMask = aspectMask,
                .levelCount = mipLevels,
                .layerCount = layers,
            };
			SetLayout(cmd, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
        }

        VkSubresourceLayout SubresourceLayout(const VkImageAspectFlagBits& aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) 
        {
            VkImageSubresource subResource{};
            subResource.aspectMask = aspectMask;
            VkSubresourceLayout subResourceLayout;

            vkGetImageSubresourceLayout(VulkanContext::GetLogicalDevice(), m_Handle, &subResource, &subResourceLayout);
            return subResourceLayout;
        }

        bool HasDepth() const
        {
			VkFormat formats[] = 
            {
		        VK_FORMAT_D16_UNORM,
		        VK_FORMAT_X8_D24_UNORM_PACK32,
		        VK_FORMAT_D32_SFLOAT,
		        VK_FORMAT_D16_UNORM_S8_UINT,
		        VK_FORMAT_D24_UNORM_S8_UINT,
		        VK_FORMAT_D32_SFLOAT_S8_UINT
			};
			return std::find(std::begin(formats), std::end(formats), format) != std::end(formats);
        }

        bool HasStencil() const
        {
            VkFormat formats[] =
            {
                VK_FORMAT_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
            };
            return std::find(std::begin(formats), std::end(formats), format) != std::end(formats);
        }

        bool IsDepthStencil() const { return(HasDepth() || HasStencil()); }
    };

    struct ImageView : public VkObject<VkImageView> 
    {
        ImageView() = default;
        ImageView(VkImageView view) { m_Handle = view; }
        VkResult Create(const VkImageViewCreateInfo& createInfo) { return vkCreateImageView(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_Handle); }

        VkResult Create(const Image& image)
        {
			return Create({
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,

				.image = image,
				.viewType = image.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				.format = image.format,
				.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
				.subresourceRange = {
					.aspectMask = VkImageAspectFlags(image.HasDepth() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
					.levelCount = image.mipLevels,
					.layerCount = image.layers,
				},
			});
        }

        void Destroy() 
        {
            vkDestroyImageView(VulkanContext::GetLogicalDevice(), m_Handle, VulkanContext::GetAllocator());
            m_Handle = VK_NULL_HANDLE;
        }
    };


	struct TrackedImageView : public ImageView
	{
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

		uint32_t baseMipLevel = 0;
		uint32_t levelCount = 1;
		uint32_t baseArrayLayer = 0;
		uint32_t layerCount = 1;
	};

	struct TrackedImage : public Image
	{
		std::vector<TrackedImageView> views;
	};

    enum class Filter 
    {
        NEAREST = 0,
        LINEAR = 1,
    };

    enum class SamplerMipmapMode 
    {
        NEAREST = 0,
        LINEAR = 1,
    };

    enum SamplerAddressMode 
    {
        REPEAT = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE = 2,
        CLAMP_TO_BORDER = 3
    };

    struct SamplerSpecification
    {
        Filter                magFilter = Filter::NEAREST;
        Filter                minFilter = Filter::NEAREST;
        SamplerMipmapMode     mipmapMode = SamplerMipmapMode::NEAREST;
        SamplerAddressMode    addressModeU = SamplerAddressMode::REPEAT;
        SamplerAddressMode    addressModeV = SamplerAddressMode::REPEAT;
        SamplerAddressMode    addressModeW = SamplerAddressMode::REPEAT;
        float                 mipLodBias = 0.f;
        bool                  anisotropyEnable = false;
        float                 maxAnisotropy = 1.f;
        float                 minLod = 0.f;
        float                 maxLod = 1.f;
    };

    struct Sampler : public VkObject<VkSampler> 
    {
        Sampler() = default;
        Sampler(const VkSampler& sampler) { m_Handle = sampler; }

        VkResult Create(const VkSamplerCreateInfo& create_info) 
        { return vkCreateSampler(VulkanContext::GetLogicalDevice(), &create_info, VulkanContext::GetAllocator(), &m_Handle); }

        VkResult Create(const SamplerSpecification& spec)
        {
			return Create({
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,

				.magFilter = (VkFilter)spec.magFilter,
				.minFilter = (VkFilter)spec.minFilter,
				.mipmapMode = (VkSamplerMipmapMode)spec.mipmapMode,
				.addressModeU = (VkSamplerAddressMode)spec.addressModeU,
				.addressModeV = (VkSamplerAddressMode)spec.addressModeV,
				.addressModeW = (VkSamplerAddressMode)spec.addressModeW,
				.mipLodBias = spec.mipLodBias,
				.anisotropyEnable = spec.anisotropyEnable,
				.maxAnisotropy = spec.maxAnisotropy,
				.compareEnable = false,
				.compareOp = VK_COMPARE_OP_NEVER,
				.minLod = spec.minLod,
				.maxLod = spec.maxLod,
				.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
				.unnormalizedCoordinates = false
			});
        }

        void Destroy() 
        {
            vkDestroySampler(VulkanContext::GetLogicalDevice(), m_Handle, VulkanContext::GetAllocator());
            m_Handle = VK_NULL_HANDLE;
        }
    };
}