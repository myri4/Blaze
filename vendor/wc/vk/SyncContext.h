#pragma once

#include "VulkanContext.h"
#include "Commands.h"
#include "Synchronization.h"

constexpr uint32_t FRAME_OVERLAP = 1;
constexpr uint64_t MAX_PASSES = 200;
inline uint8_t CURRENT_FRAME = 0;

namespace vk::SyncContext
{
	inline Semaphore PresentSemaphores[FRAME_OVERLAP], ImageAvaibleSemaphores[FRAME_OVERLAP];
	inline TimelineSemaphore m_TimelineSemaphore;
	inline uint64_t m_TimelineValue = 0;

	inline Fence RenderFences[FRAME_OVERLAP];
	inline Fence ImmediateFence;

	inline VkCommandBuffer MainCommandBuffers[FRAME_OVERLAP];
	inline VkCommandBuffer ComputeCommandBuffers[FRAME_OVERLAP];
	inline VkCommandBuffer UploadCommandBuffer;

	inline CommandPool GraphicsCommandPool;
	inline CommandPool ComputeCommandPool;
	inline CommandPool UploadCommandPool;

	inline auto& GetPresentSemaphore() { return PresentSemaphores[CURRENT_FRAME]; }
	inline auto& GetImageAvaibleSemaphore() { return ImageAvaibleSemaphores[CURRENT_FRAME]; }

	inline auto& GetRenderFence() { return RenderFences[CURRENT_FRAME]; }

	inline auto& GetMainCommandBuffer() { return MainCommandBuffers[CURRENT_FRAME]; }
	inline auto& GetComputeCommandBuffer() { return ComputeCommandBuffers[CURRENT_FRAME]; }

	inline const Queue GetGraphicsQueue() { return GraphicsQueue; }
	inline const Queue GetComputeQueue() { return ComputeQueue; }
	inline const Queue GetTransferQueue() { return GraphicsQueue; }
	inline const Queue GetPresentQueue() { return /*presentQueue*/GraphicsQueue; }

	inline void Create()
	{
		GraphicsCommandPool.Create(GetGraphicsQueue().queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		ComputeCommandPool.Create(GetComputeQueue().queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		UploadCommandPool.Create(GetTransferQueue().queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		UploadCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, UploadCommandBuffer);
		ImmediateFence.Create();

		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			PresentSemaphores[i].Create(std::format("PresentSemaphore[{}]", i));
			ImageAvaibleSemaphores[i].Create(std::format("ImageAvaibleSemaphore[{}]", i));

			GraphicsCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, MainCommandBuffers[i]);
			ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, ComputeCommandBuffers[i]);

			VulkanContext::SetObjectName(MainCommandBuffers[i], std::format("MainCommandBuffer[{}]", i));
			VulkanContext::SetObjectName(ComputeCommandBuffers[i], std::format("ComputeCommandBuffer[{}]", i));

			RenderFences[i].Create(VK_FENCE_CREATE_SIGNALED_BIT);
		}

		m_TimelineSemaphore.Create("SyncContext::m_TimelineSemaphore");
	}	

	inline void UpdateFrame()
	{
		CURRENT_FRAME = (CURRENT_FRAME + 1) % FRAME_OVERLAP;
		if (m_TimelineValue >= UINT64_MAX - MAX_PASSES)
		{
			m_TimelineSemaphore.Destroy();
			m_TimelineSemaphore.Create("SyncContext::m_TimelineSemaphore");
			m_TimelineValue = 0;
		}
	}

	inline void Submit(VkCommandBuffer cmd, const Queue& queue, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, Fence fence = VK_NULL_HANDLE)
	{
		const uint64_t waitValue = m_TimelineValue;
		const uint64_t signalValue = ++m_TimelineValue;

		VkTimelineSemaphoreSubmitInfo timelineInfo = { 
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount = 1,
			.pWaitSemaphoreValues = &waitValue,
			.signalSemaphoreValueCount = 1,
			.pSignalSemaphoreValues = &signalValue,
		};

		VkSubmitInfo submitInfo = { 
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_TimelineSemaphore,
			.pWaitDstStageMask = &waitStage,

			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_TimelineSemaphore,
		};

		queue.Submit(submitInfo, fence);
	}

	inline void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function) // @TODO: revisit if this is suitable for a inline
	{
		VkCommandBufferBeginInfo begInfo = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		vkBeginCommandBuffer(UploadCommandBuffer, &begInfo);

		function(UploadCommandBuffer);

		vkEndCommandBuffer(UploadCommandBuffer);

		VkSubmitInfo submit = { 
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &UploadCommandBuffer,
		};

		GetGraphicsQueue().Submit(submit, ImmediateFence);

		ImmediateFence.Wait();
		ImmediateFence.Reset();

		vkResetCommandBuffer(UploadCommandBuffer, 0);
	}	

	inline void Destroy()
	{
		GraphicsCommandPool.Destroy();
		ComputeCommandPool.Destroy();

		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			PresentSemaphores[i].Destroy();
			ImageAvaibleSemaphores[i].Destroy();

			RenderFences[i].Destroy();
		}

		m_TimelineSemaphore.Destroy();

		UploadCommandPool.Destroy();
		ImmediateFence.Destroy();
	}
}