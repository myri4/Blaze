//#define GLM_FORCE_INTRINSICS 

#include <Windows.h>
#include <commdlg.h>

#pragma warning( push )
#pragma warning( disable : 4702) // Disable unreachable code
#define GLFW_INCLUDE_NONE
//#define GLM_FORCE_PURE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS

#define MSDFGEN_PUBLIC // ???

#include "imgui_backend/imgui_impl_glfw.h"
#include "imgui_backend/imgui_impl_vulkan.h"

#include "Editor/Editor.h"

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

EditorInstance editor;

vk::Swapchain swapchain;

void Resize()
{
	auto size = Globals.window.GetFramebufferSize();
	if (size.x == 0 || size.y == 0)
		return;

	VulkanContext::GetLogicalDevice().WaitIdle();
	swapchain.Destroy();
	swapchain.Create(Globals.window);

	// Resize renderers
	editor.Resize(Globals.window.GetSize());
	Globals.window.resized = false;
}

//----------------------------------------------------------------------------------------------------------------------
bool InitApp()
{
	WindowCreateInfo windowInfo =
	{
		.Width = 1280,
		.Height = 720,
		.Name = "Editor",
		.StartMode = WindowMode::Maximized,
		.VSync = false,
		.Resizeable = true,
		.Decorated = false,
	};
	Globals.window.Create(windowInfo);
	Globals.window.SetFramebufferResizeCallback([](GLFWwindow* window, int w, int h)
		{
			reinterpret_cast<wc::Window*>(glfwGetWindowUserPointer(window))->resized = true;
		});
	swapchain.Create(Globals.window);

	vk::SyncContext::Create();

	vk::descriptorAllocator.Create();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "assets/imgui.ini"; // TODO - remove and find alternative
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	/*//OLD FONTS
	Globals.fontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 17.f);
	Globals.fontBig = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 30.f);
	Globals.fontMenu = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 20.f);*/

	// Default font -> Poppins
	Globals.f_Default.Regular = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Regular.ttf", 17.f);
	Globals.f_Default.Bold = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Bold.ttf", 17.f);
	Globals.f_Default.Italic = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Italic.ttf", 17.f);
	Globals.f_Default.Thin = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Thin.ttf", 17.f);
	Globals.f_Default.Big = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Regular.ttf", 30.f);
	Globals.f_Default.Small = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Regular.ttf", 14.f);
	Globals.f_Default.Menu = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Regular.ttf", 20.f);

	// Display font -> Neptune
	Globals.f_Display.Regular = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Regular.ttf", 17.f);
	Globals.f_Display.Bold = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Bold.ttf", 17.f);
	Globals.f_Display.Thin = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Thin.ttf", 17.f);
	Globals.f_Display.Big = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Regular.ttf", 30.f);
	Globals.f_Display.Small = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Regular.ttf", 14.f);
	Globals.f_Display.Menu = io.Fonts->AddFontFromFileTTF("assets/fonts/SeedSans/SeedSans-Regular.ttf", 20.f);

	io.FontDefault = Globals.f_Default.Regular;

	ImGui_ImplGlfw_InitForVulkan(Globals.window, false);

	ImGui_ImplVulkan_Init(swapchain.RenderPass);
	ImGui_ImplVulkan_CreateFontsTexture();

	ImGuiStyle& style = ImGui::GetStyle();
	style = ui::SoDark(0.0f);
	editor.Create();

	return true;
}

//----------------------------------------------------------------------------------------------------------------------
void UpdateApp()
{
	auto r = vk::SyncContext::GetRenderFence().Wait();

	//WC_CORE_INFO("Acquire result: {}, {}", magic_enum::enum_name(r), (int)r);

	Globals.UpdateTime();
	editor.Update();

	auto extent = Globals.window.GetExtent();

	//if (extent.width > 0 && extent.height > 0)
	{
		uint32_t swapchainImageIndex = 0;

		VkResult result = swapchain.AcquireNextImage(swapchainImageIndex, vk::SyncContext::GetImageAvaibleSemaphore());

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Resize();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			WC_CORE_ERROR("Acquire result: {}, {}", magic_enum::enum_name(result), (int)result);
		}
		vk::SyncContext::GetRenderFence().Reset(); // deadlock fix


		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		editor.UI();

		ImGui::Render();

		auto& cmd = vk::SyncContext::GetMainCommandBuffer();
		vkResetCommandBuffer(cmd, 0);

		editor.Render();

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}


		VkClearValue clearValue = {
			.color = { 0.f, 0.f, 0.f, 0.f },
		};

		VkRenderPassBeginInfo rpInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,

			.renderPass = swapchain.RenderPass,
			.framebuffer = swapchain.Framebuffers[swapchainImageIndex],
			.renderArea.extent = swapchain.extent,
			.clearValueCount = 1,
			.pClearValues = &clearValue,
		};

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, rpInfo);

		VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
		vk::Semaphore waitSemaphores[] = { vk::SyncContext::GetImageAvaibleSemaphore(), vk::SyncContext::m_TimelineSemaphore, };
		uint64_t waitValues[] = { 0, vk::SyncContext::m_TimelineValue }; // @NOTE: Apparently the timeline semaphore should be waiting last?
		VkTimelineSemaphoreSubmitInfo timelineInfo = {
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount = std::size(waitValues),
			.pWaitSemaphoreValues = waitValues,
		};

		VkSubmitInfo submit = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,

			.pWaitDstStageMask = waitStage,

			.waitSemaphoreCount = std::size(waitSemaphores),
			.pWaitSemaphores = (VkSemaphore*)waitSemaphores,

			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &vk::SyncContext::GetPresentSemaphore(),
		};

		vk::SyncContext::GetGraphicsQueue().Submit(submit, vk::SyncContext::GetRenderFence());

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &vk::SyncContext::GetPresentSemaphore(),

			.swapchainCount = 1,
			.pSwapchains = &swapchain,

			.pImageIndices = &swapchainImageIndex,
		};

		VkResult presentationResult = vkQueuePresentKHR(vk::SyncContext::GetPresentQueue(), &presentInfo);

		if (presentationResult == VK_ERROR_OUT_OF_DATE_KHR || presentationResult == VK_SUBOPTIMAL_KHR || Globals.window.resized)
		{
			Resize();
		}
		else if (presentationResult != VK_SUCCESS)
		{
			WC_CORE_ERROR("Presentation result: {}, code: {}", magic_enum::enum_name(presentationResult), (int)presentationResult);
		}

		vk::SyncContext::UpdateFrame();
	}
}

//----------------------------------------------------------------------------------------------------------------------
void UpdateAppFrame()
{
	while (Globals.window.IsOpen())
	{
		Globals.window.PoolEvents();

		// Resize window
		{
			constexpr glm::ivec2 minWindowSize{ 720, 480 };
			struct ResizeState {
				bool active = false;
				bool edges[4] = {};
				glm::ivec2 initialPos;
				glm::ivec2 initialSize;
				glm::ivec2 initialMouseGlobal;
			}static resize;

			auto& window = Globals.window;
			const auto mousePos = window.GetCursorPos();
			const auto windowSize = window.GetSize();
			const auto windowPos = window.GetPosition();
			constexpr int borderSize = 5;

			const glm::ivec2 globalMousePos = windowPos + mousePos;

			enum Edges { LEFT = 0, RIGHT, TOP, BOTTOM };
			bool edgeHover[4] = {
				(mousePos.x >= -borderSize) && (mousePos.x <= borderSize),
				(mousePos.x >= windowSize.x - borderSize) && (mousePos.x <= windowSize.x + borderSize),
				(mousePos.y >= -borderSize) && (mousePos.y <= borderSize),
				(mousePos.y >= windowSize.y - borderSize) && (mousePos.y <= windowSize.y + borderSize)
			};

			const bool canResize = window.HasFocus() && !resize.active;
			for (auto& edge : edgeHover) 
				edge &= canResize;

			// resize initiation
			if (!Globals.window.IsMaximized() && !resize.active && ImGui::IsMouseClicked(0)) 
			{
				bool anyEdge = false;
				for (int i = 0; i < 4; i++) 
				{
					if (edgeHover[i]) 
					{
						resize.edges[i] = true;
						anyEdge = true;
					}
				}
				if (anyEdge) 
				{
					resize.active = true;
					resize.initialPos = windowPos;
					resize.initialSize = windowSize;
					resize.initialMouseGlobal = globalMousePos;
				}
			}

			// ongoing resize
			if (resize.active) 
			{
				if (ImGui::IsMouseDragging(0)) 
				{
					const glm::ivec2 delta = globalMousePos - resize.initialMouseGlobal;

					// Horizontal
					if (resize.edges[LEFT])
					{
						window.SetPosition({ resize.initialPos.x + delta.x, window.GetPosition().y });
						window.SetSize({ std::max(resize.initialSize.x - delta.x, minWindowSize.x), window.GetSize().y });
					}
					else if (resize.edges[RIGHT]) 
						window.SetSize({ std::max(resize.initialSize.x + delta.x, minWindowSize.x), window.GetSize().y });

					// Vertical
					if (resize.edges[TOP]) 
					{
						window.SetPosition({ window.GetPosition().x, resize.initialPos.y + delta.y });
						window.SetSize({ window.GetSize().x, std::max(resize.initialSize.y - delta.y, minWindowSize.y) });
					}
					else if (resize.edges[BOTTOM]) 
						window.SetSize({ window.GetSize().x, std::max(resize.initialSize.y + delta.y, minWindowSize.y) });
				}

				if (ImGui::IsMouseReleased(0)) 
					memset(&resize, 0, sizeof(resize));
			}

			// Update cursor
			ImGuiMouseCursor cursor = ImGuiMouseCursor_Arrow;
			if (!Globals.window.IsMaximized() && (resize.active || canResize)) 
			{
				bool horizontalHover = edgeHover[LEFT] || edgeHover[RIGHT];
				bool verticalHover = edgeHover[TOP] || edgeHover[BOTTOM];
				bool horizontalActive = resize.edges[LEFT] || resize.edges[RIGHT];
				bool verticalActive = resize.edges[TOP] || resize.edges[BOTTOM];

				bool isCorner = false;
				ImGuiMouseCursor cornerCursor = ImGuiMouseCursor_Arrow;

				if (resize.active) 
				{
					if ((resize.edges[LEFT] && resize.edges[TOP]) || (resize.edges[RIGHT] && resize.edges[BOTTOM])) 
					{
						isCorner = true;
						cornerCursor = ImGuiMouseCursor_ResizeNWSE;
					}
					else if ((resize.edges[RIGHT] && resize.edges[TOP]) || (resize.edges[LEFT] && resize.edges[BOTTOM])) 
					{
						isCorner = true;
						cornerCursor = ImGuiMouseCursor_ResizeNESW;
					}
				}
				else 
				{
					if ((edgeHover[LEFT] && edgeHover[TOP]) || (edgeHover[RIGHT] && edgeHover[BOTTOM])) 
					{
						isCorner = true;
						cornerCursor = ImGuiMouseCursor_ResizeNWSE;
					}
					else if ((edgeHover[RIGHT] && edgeHover[TOP]) || (edgeHover[LEFT] && edgeHover[BOTTOM])) 
					{
						isCorner = true;
						cornerCursor = ImGuiMouseCursor_ResizeNESW;
					}
				}

				if (isCorner)								  cursor = cornerCursor;
				else if (horizontalHover || horizontalActive) cursor = ImGuiMouseCursor_ResizeEW;
				else if (verticalHover || verticalActive)     cursor = ImGuiMouseCursor_ResizeNS;
			}

			if (!gui::IsAnyItemHovered()) 
				gui::SetMouseCursor(cursor);
		}

		if (Globals.window.HasFocus()) editor.Input();

		UpdateApp();
	}
}

//----------------------------------------------------------------------------------------------------------------------
void DeinitApp()
{
	VulkanContext::GetLogicalDevice().WaitIdle();
	glfwWaitEvents();

	Globals.window.Destroy();
	swapchain.Destroy();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vk::descriptorAllocator.Destroy();
	editor.Destroy();

	vk::SyncContext::Destroy();
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


		if (InitApp())
			UpdateAppFrame();

		DeinitApp();


		VulkanContext::Destroy();
	}

	glfwTerminate();

	return 0;
}