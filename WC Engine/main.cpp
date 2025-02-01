//#define GLM_FORCE_INTRINSICS 
#pragma comment(lib, "spirv-cross-core")

#include <Windows.h>
#include <commdlg.h>

#pragma warning( push )
#pragma warning( disable : 4702) // Disable unreachable code
#define GLFW_INCLUDE_NONE
//#define GLM_FORCE_PURE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS
#define flecs_STATIC

#define MSDFGEN_PUBLIC // ???

#include "wc/imgui_backend/imgui_impl_glfw.h"
#include "wc/imgui_backend/imgui_impl_vulkan.h"

#include "Editor.h"
#include "UI/Widgets.h"

#include <wc/Swapchain.h>

//DANGEROUS!
#pragma warning(push, 0)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_write.h>

#define VOLK_IMPLEMENTATION 
#include <Volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#pragma warning(pop)

namespace wc
{
	Editor editor;

	vk::Swapchain swapchain;
	bool SwapChainOk = true;

	void Resize()
	{
		auto size = Globals.window.GetFramebufferSize();
		while (size.x == 0 || size.y == 0)
		{
			size = Globals.window.GetFramebufferSize();
			glfwWaitEvents();
		}

		VulkanContext::GetLogicalDevice().WaitIdle();
		swapchain.Destroy();
		swapchain.Create(Globals.window);

		// Resize renderers
		editor.Resize(Globals.window.GetSize());
		Globals.window.resized = false;
		SwapChainOk = true;
	}

	//----------------------------------------------------------------------------------------------------------------------
	bool OnCreate()
	{
		WindowCreateInfo windowInfo =
		{
			.Width = 1280,
			.Height = 720,
			.Name = "Editor",
			.StartMode = WindowMode::Maximized,
			.VSync = false,
			.Resizeable = true,
			//	.Decorated = false;
		};
		Globals.window.Create(windowInfo);
		Globals.window.SetResizeCallback([](GLFWwindow* window, int w, int h)
			{
				Resize();
			});
		swapchain.Create(Globals.window);

		vk::SyncContext::Create();

		vk::descriptorAllocator.Create();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.IniFilename = "assets/imgui.ini"; // TODO - remove and find alternative
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	    Globals.fontDeffault = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 17.f);
	    Globals.fontBig = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 30.f);
		io.FontDefault = Globals.fontDeffault;

		ImGui_ImplGlfw_Init(Globals.window, false);

		ImGui_ImplVulkan_Init(swapchain.RenderPass);
		ImGui_ImplVulkan_CreateFontsTexture();


		ImGuiStyle& style = ImGui::GetStyle();
		style = UI::SoDark(0.0f);
		editor.Create();

		return true;
	}
	//----------------------------------------------------------------------------------------------------------------------		

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
		ImGuizmo::BeginFrame();
		editor.UI();
		//ImGui::ShowDemoWindow();
		ImGui::EndFrame();
		//ImGuizmo::EndFrame();
		ImGui::Render();

		vkResetCommandBuffer(vk::SyncContext::GetMainCommandBuffer(), 0);
		editor.Update();
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
	void OnFrameUpdate()
	{
		while (Globals.window.IsOpen())
		{
			Globals.window.PoolEvents();

			if (Globals.window.HasFocus())
				editor.Input();

			OnUpdate();
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	void OnDelete()
	{
		VulkanContext::GetLogicalDevice().WaitIdle();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		vk::descriptorAllocator.Destroy();
		editor.Destroy();

		vk::SyncContext::Destroy();
		swapchain.Destroy();
		Globals.window.Destroy();
	}

	int main()
	{
		Log::Init();

#ifdef MSVC  // Visual Studio
		std::filesystem::current_path("../../../../WC Engine/workdir");
#elif defined(CLION)  // CLion
		std::filesystem::current_path("../../WC Engine/workdir");
#else  // Default or other IDEs
		std::filesystem::current_path("../../../../WC Engine/workdir");
#endif

		glfwSetErrorCallback([](int err, const char* description) { WC_CORE_ERROR(description); /*WC_DEBUGBREAK();*/ });
		//glfwSetMonitorCallback([](GLFWmonitor* monitor, int event)
		//	{
		//		if (event == GLFW_CONNECTED)
		//		else if (event == GLFW_DISCONNECTED)
		//	});
		if (!glfwInit())
			return 1;

		if (VulkanContext::Create())
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);


			if (OnCreate())
				OnFrameUpdate();

			OnDelete();


			VulkanContext::Destroy();
		}

		glfwTerminate();

		return 0;
	}
}

int main()
{
	return wc::main();
}