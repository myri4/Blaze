#pragma once

#include "vk/VulkanContext.h"
#include <array>
#include <deque>

namespace vk 
{
	struct DescriptorAllocator 
	{
		void Create() 
		{
			m_CurrentPool = GrabPool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		void ResetPools() 
		{
			for (auto& p : m_UsedPools)
				vkResetDescriptorPool(VulkanContext::GetLogicalDevice(), p, 0);

			m_FreePools = m_UsedPools;
			m_UsedPools.clear();
			m_CurrentPool = VK_NULL_HANDLE;
		}

		bool Allocate(VkDescriptorSet& set, const VkDescriptorSetLayout& layout) { return Allocate(set, layout, nullptr, 1); }

		bool Allocate(VkDescriptorSet& set, const VkDescriptorSetLayout& layout, const void* pNext, uint32_t descriptorCount) 
		{
			if (m_CurrentPool == VK_NULL_HANDLE)
			{
				m_CurrentPool = GrabPool();
				m_UsedPools.push_back(m_CurrentPool);
			}

			VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocInfo.descriptorPool = m_CurrentPool;
			allocInfo.descriptorSetCount = descriptorCount;
			allocInfo.pSetLayouts = &layout;
			allocInfo.pNext = pNext;

			switch (vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, &set))
			{
			case VK_SUCCESS: return true;
			case VK_ERROR_FRAGMENTED_POOL:
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				//allocate a new pool and retry
				m_CurrentPool = GrabPool();
				m_UsedPools.push_back(m_CurrentPool);
				
				//if it still fails then we have big issues
				if (vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, &set) == VK_SUCCESS) return true;
			}

			return false;
		}

		void Destroy() 
		{
			for (auto& p : m_FreePools)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), p, VulkanContext::GetAllocator());
			
			for (auto& p : m_UsedPools)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), p, VulkanContext::GetAllocator());
		}

		auto GetCurrentPool() { return m_CurrentPool; }

	private:
		VkDescriptorPool GrabPool() 
		{
			if (m_FreePools.size() > 0)
			{
				VkDescriptorPool pool = m_FreePools.back();
				m_FreePools.pop_back();
				return pool;
			}

			uint32_t count = 500;

			std::pair<VkDescriptorType, float> dSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1.f },
			};


			std::array<VkDescriptorPoolSize, std::size(dSizes)> sizes;
			for (int i = 0; i < std::size(dSizes); i++)
				sizes[i] = { dSizes[i].first, uint32_t(dSizes[i].second * count) };

			VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			pool_info.flags = 0;
			pool_info.maxSets = count;
			pool_info.poolSizeCount = (uint32_t)sizes.size();
			pool_info.pPoolSizes = sizes.data();

			VkDescriptorPool descriptorPool;
			vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &pool_info, VulkanContext::GetAllocator(), &descriptorPool);

			return descriptorPool;
		}

		VkDescriptorPool m_CurrentPool;
		std::vector<VkDescriptorPool> m_UsedPools;
		std::vector<VkDescriptorPool> m_FreePools;
	}inline descriptorAllocator;

	struct DescriptorWriter 
	{
	private:
		std::deque<VkDescriptorImageInfo> m_ImageInfos;
		std::deque<VkDescriptorBufferInfo> m_BufferInfos;
		bool m_Updated = false;
	public:
		std::vector<VkWriteDescriptorSet> writes;
		VkDescriptorSet dstSet = VK_NULL_HANDLE;

		DescriptorWriter(VkDescriptorSet dSet) { dstSet = dSet; }
		~DescriptorWriter() { Update(); }

		DescriptorWriter& BindBuffer(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) 
		{
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pBufferInfo = &m_BufferInfos.emplace_back(bufferInfo);

			writes.push_back(write);
			return *this;
		}

		DescriptorWriter& BindBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) { return BindBuffer(binding, { buffer, offset, size }, type); }

		DescriptorWriter& BindImage(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType type) 
		{
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pImageInfo = &m_ImageInfos.emplace_back(imageInfo);

			writes.push_back(write);
			return *this;
		}

		DescriptorWriter& BindImage(uint32_t binding, VkSampler sampler, VkImageView image, VkImageLayout layout, VkDescriptorType type) {	return BindImage(binding, { sampler, image, layout }, type); }

		DescriptorWriter& BindImages(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfo, VkDescriptorType type)
		{
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = (uint32_t)imageInfo.size();
			write.descriptorType = type;
			write.pImageInfo = imageInfo.data();

			writes.push_back(write);
			return *this;
		}

		void Clear()
		{
			m_ImageInfos.clear();
			m_BufferInfos.clear();
			writes.clear();
		}

		void Update() 
		{	
			if (!m_Updated)
			{
				vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
				m_Updated = true;
			}
		}
	};
}