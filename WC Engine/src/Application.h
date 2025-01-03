#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/ImGuizmo.h>

#include "Scene/Game.h"

#include <wc/Swapchain.h>

namespace wc
{
		GameInstance game;
		
		vk::Swapchain swapchain;
		bool SwapChainOk = true;

		void Resize()
		{
			VulkanContext::GetLogicalDevice().WaitIdle();
			swapchain.Destroy();
			swapchain.Create(Globals.window);

			// Resize renderers
			game.Resize(Globals.window.GetSize());
			Globals.window.resized = false;
			SwapChainOk = true;
		}
	class Application 
	{
		//----------------------------------------------------------------------------------------------------------------------
		bool OnCreate() 
		{
			WindowCreateInfo windowInfo;
			windowInfo.Width = 1280;
			windowInfo.Height = 720;
			windowInfo.VSync = false;
			windowInfo.Resizeable = true;
			windowInfo.AppName = "WC Engine";
			windowInfo.StartMode = WindowMode::Normal;
			windowInfo.Decorated = true;
			Globals.window.Create(windowInfo);
			Globals.window.SetResizeCallback([](GLFWwindow* window, int w, int h) 
			{ 
				if (w > 0 && h > 0) 
					Resize();
			});
			swapchain.Create(Globals.window);

			vk::SyncContext::Create();

			vk::descriptorAllocator.Create();

			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.IniFilename = nullptr;
			io.IniFilename = "assets/imgui.ini"; // TODO - remove and find alternative
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/ST-SimpleSquare.ttf", 25.f);

			ImGui_ImplGlfw_Init(Globals.window, false);

			ImGui_ImplVulkan_Init(swapchain.RenderPass);
			ImGui_ImplVulkan_CreateFontsTexture();

			auto& style = ImGui::GetStyle();
			style.WindowMenuButtonPosition = ImGuiDir_None;
			game.Create(Globals.window.GetSize());

			return true;
		}
		//----------------------------------------------------------------------------------------------------------------------
		
		//----------------------------------------------------------------------------------------------------------------------
		void OnFrameUpdate() 
		{
			while (Globals.window.IsOpen())
			{
				if (SwapChainOk)
				{
					Globals.window.PoolEvents();

					if (Globals.window.HasFocus())
						game.InputGame();
				}
				else
				{
					glfwWaitEvents(); // If minimized, block until unminimized
					continue;
				}

				OnUpdate();
			}			
		}

		void OnUpdate()
		{
			vk::SyncContext::GetRenderFence().Wait();

			Globals.UpdateTime();

			uint32_t swapchainImageIndex = 0;
			if (SwapChainOk)
			{
				VkResult result = swapchain.AcquireNextImage(swapchainImageIndex, vk::SyncContext::GetImageAvaibleSemaphore());

				if (result == VK_ERROR_OUT_OF_DATE_KHR)
				{
					Resize();
					SwapChainOk = false;
				}
				vk::SyncContext::GetRenderFence().Reset(); // deadlock fix
			}

			if (!SwapChainOk)
				return;


			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			game.UI();
			//ImGui::ShowDemoWindow();
			ImGui::EndFrame();
			ImGui::Render();			

			vkResetCommandBuffer(vk::SyncContext::GetMainCommandBuffer(), 0);
			game.Update();
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			
			auto extent = Globals.window.GetExtent();

			if (extent.width != 0 && extent.height != 0)
			{
				VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

				rpInfo.renderPass = swapchain.RenderPass;
				rpInfo.framebuffer = swapchain.Framebuffers[swapchainImageIndex];
				rpInfo.renderArea.extent = extent;
				rpInfo.clearValueCount = 1;
				VkClearValue clearValue = {};
				clearValue.color = { 0.f, 0.f, 0.f, 0.f };
				rpInfo.pClearValues = &clearValue;

				auto& cmd = vk::SyncContext::GetMainCommandBuffer();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, rpInfo);

				VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

				VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
				timelineInfo.waitSemaphoreValueCount = 2;
				uint64_t waitValues[] = { 0, vk::SyncContext::m_TimelineValue }; // @NOTE: Apparently the timeline semaphore should be waiting last?
				timelineInfo.pWaitSemaphoreValues = waitValues;
				submit.pNext = &timelineInfo;
				submit.commandBufferCount = 1;
				submit.pCommandBuffers = &cmd;

				VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

				submit.pWaitDstStageMask = waitStage;

				vk::Semaphore waitSemaphores[] = { vk::SyncContext::GetImageAvaibleSemaphore(), vk::SyncContext::m_TimelineSemaphore, };

				submit.waitSemaphoreCount = std::size(waitSemaphores);
				submit.pWaitSemaphores = (VkSemaphore*)waitSemaphores;

				submit.signalSemaphoreCount = 1;
				submit.pSignalSemaphores = &vk::SyncContext::GetPresentSemaphore();

				vk::SyncContext::GetGraphicsQueue().Submit(submit, vk::SyncContext::GetRenderFence());

				VkResult presentationResult = swapchain.Present(swapchainImageIndex, vk::SyncContext::GetPresentSemaphore());

				if (presentationResult == VK_ERROR_OUT_OF_DATE_KHR) 
					SwapChainOk = false;

				//if (presentationResult == VK_ERROR_OUT_OF_DATE_KHR || presentationResult == VK_SUBOPTIMAL_KHR || Globals.window.resized)
				//{
				//	Resize();
				//}
			}

			vk::SyncContext::UpdateFrame();
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnDelete()
		{
			VulkanContext::GetLogicalDevice().WaitIdle();
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();

			vk::descriptorAllocator.Destroy();
			game.Destroy();
			
			vk::SyncContext::Destroy();
			swapchain.Destroy();
			Globals.window.Destroy();
		}
		//----------------------------------------------------------------------------------------------------------------------
	public:

		void Start()
		{
			if (!OnCreate()) return;

			OnFrameUpdate();

			OnDelete();
		}
	};
}