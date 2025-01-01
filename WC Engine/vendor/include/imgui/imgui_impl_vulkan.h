#pragma once
#ifndef IMGUI_DISABLE
#include "imgui.h"      // IMGUI_IMPL_API

// Vulkan includes
#include <wc/Memory/Buffer.h>

#include <wc/vk/Buffer.h>
#include <wc/vk/Commands.h>
#include <wc/vk/Image.h>
#include <wc/vk/SyncContext.h>
#include <wc/Descriptors.h>
#include <wc/Shader.h>
#include <wc/Swapchain.h>

// Called by user code
bool         ImGui_ImplVulkan_Init(VkRenderPass rp);
void         ImGui_ImplVulkan_Shutdown();
void         ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer cmd, VkRenderPassBeginInfo rpInfo);
bool         ImGui_ImplVulkan_CreateFontsTexture();
void         ImGui_ImplVulkan_DestroyFontsTexture();

//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_FrameSemaphores
{
    vk::Semaphore         ImageAcquiredSemaphore;
	vk::Semaphore         RenderCompleteSemaphore;
};

struct ImGui_ImplVulkanH_Frame
{
    VkCommandBuffer     CommandBuffer;
    vk::Fence           Fence;
};

struct ImGui_ImplVulkanH_Window
{
    uint32_t            Width = 0;
    uint32_t            Height = 0;
    bool                ClearEnable = true;
    vk::Swapchain       Swapchain;
    VkClearValue        ClearValue = {};
    uint32_t            FrameIndex = 0;
    uint32_t            SemaphoreIndex = 0;
    uint32_t            swapchainImageIndex = 0;
    wc::FPtr<ImGui_ImplVulkanH_Frame> Frames;
    wc::FPtr<ImGui_ImplVulkanH_FrameSemaphores> FrameSemaphores;

    void CreateOrResize(uint32_t width, uint32_t height)
    {
        if (Swapchain)
		{
			VulkanContext::GetLogicalDevice().WaitIdle();
			FreeSyncs();
		    Swapchain.DestroyFramebuffers();
        }
		// Create Swapchain
		Width = width;
		Height = height;

		Swapchain.surface.Query();
		Swapchain.Create(VkExtent2D{ width, height }, { width, height }, false, ClearEnable);
		AllocSyncs();
    }

    void AllocSyncs()
    {
		Frames.Allocate(Swapchain.images.Count);
		FrameSemaphores.Allocate(Swapchain.images.Count + 1);
		memset(Frames.Data, 0, sizeof(Frames[0]) * Frames.Count);
		memset(FrameSemaphores.Data, 0, sizeof(FrameSemaphores[0]) * FrameSemaphores.Count);

		// Create Command Buffers
		for (uint32_t i = 0; i < Frames.Count; i++)
		{
			ImGui_ImplVulkanH_Frame* fd = &Frames[i];
			vk::SyncContext::GraphicsCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, fd->CommandBuffer);

			fd->Fence.Create(VK_FENCE_CREATE_SIGNALED_BIT);
		}

		for (auto& fsd : FrameSemaphores)
		{
			fsd.ImageAcquiredSemaphore.Create();
			fsd.RenderCompleteSemaphore.Create();
		}
    }

    void FreeSyncs()
    {
		for (uint32_t i = 0; i < Frames.Count; i++)
		{
			Frames[i].Fence.Destroy();
			vk::SyncContext::GraphicsCommandPool.Free(Frames[i].CommandBuffer);
			Frames[i].CommandBuffer = VK_NULL_HANDLE;
		}

		for (auto& fsd : FrameSemaphores)
		{
			fsd.ImageAcquiredSemaphore.Destroy();
			fsd.RenderCompleteSemaphore.Destroy();
		}
		Frames.Free();
		FrameSemaphores.Free();
    }

    void Destroy(bool destroySurface = true)
	{
        VulkanContext::GetLogicalDevice().WaitIdle();
        FreeSyncs();
        
		Swapchain.DestroyFramebuffers();
		Swapchain.DestroyRenderPass();
		Swapchain.DestroySwapchain();
		if (destroySurface) Swapchain.surface.Destroy();

        auto surface = Swapchain.surface;
        *this = ImGui_ImplVulkanH_Window();
        Swapchain.surface = surface;
    }
};

struct ImGui_ImplVulkan_FrameRenderBuffers
{
	vk::Buffer VertexBuffer;
	vk::Buffer IndexBuffer;
};

struct ImGui_ImplVulkan_WindowRenderBuffers
{
	uint32_t            Index = 0;
	wc::FPtr<ImGui_ImplVulkan_FrameRenderBuffers> FrameRenderBuffers;

	void Destroy()
	{
		for (auto& frb : FrameRenderBuffers)
		{
			frb.VertexBuffer.Free();
			frb.IndexBuffer.Free();
		}
		FrameRenderBuffers.Free();
		Index = 0;
	}
};

// For multi-viewport support:
// Helper structure we store in the void* RendererUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplVulkan_ViewportData
{
    bool                                    WindowOwned = false;
    ImGui_ImplVulkanH_Window                Window;

    ImGui_ImplVulkan_WindowRenderBuffers RenderBuffers;	
};

// Vulkan data
struct ImGui_ImplVulkan_Data
{
    VkRenderPass                RenderPass = VK_NULL_HANDLE;
    wc::Shader                  Shader;

    // Font data
    vk::Sampler                 FontSampler;
    vk::Image                   FontImage;
    vk::ImageView               FontView;
    VkDescriptorSet             FontDescriptorSet = VK_NULL_HANDLE;

    // Render buffers for main window
    ImGui_ImplVulkan_WindowRenderBuffers MainWindowRenderBuffers;
};

static ImGui_ImplVulkan_Data* ImGui_ImplVulkan_GetBackendData() { return ImGui::GetCurrentContext() ? (ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData : nullptr; }

inline VkDescriptorSet MakeImGuiDescriptor(VkDescriptorSet dSet, const VkDescriptorImageInfo& imageInfo)
{
    if (dSet == VK_NULL_HANDLE) vk::descriptorAllocator.Allocate(dSet, ImGui_ImplVulkan_GetBackendData()->Shader.GetDescriptorLayout());

    vk::DescriptorWriter writer(dSet);
    writer.BindImage(0, imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    return dSet;
}

#endif // #ifndef IMGUI_DISABLE
