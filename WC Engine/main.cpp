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

namespace gui = ImGui;

namespace wc
{
	Editor editor;

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
			.Decorated = false,
		};
		Globals.window.Create(windowInfo);
		Globals.window.SetResizeCallback([](GLFWwindow* window, int w, int h)
			{
				reinterpret_cast<wc::Window*>(window)->resized = true;
				Resize(); // @TODO: Probably should remove this in the future
			});
		swapchain.Create(Globals.window);

		vk::SyncContext::Create();

		vk::descriptorAllocator.Create();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.IniFilename = "assets/imgui.ini"; // TODO - remove and find alternative
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		Globals.fontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 17.f);
		Globals.fontBig = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 30.f);
	    Globals.fontMenu = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Regular.ttf", 20.f);
		io.FontDefault = Globals.fontDefault;

		ImGui_ImplGlfw_Init(Globals.window, false);

		ImGui_ImplVulkan_Init(swapchain.RenderPass);
		ImGui_ImplVulkan_CreateFontsTexture();

		ImGuiStyle& style = ImGui::GetStyle();
		style = ui::SoDark(0.0f);
		editor.Create();

		return true;
	}
	//----------------------------------------------------------------------------------------------------------------------
	void OnUpdate()
	{
		auto r = vk::SyncContext::GetRenderFence().Wait();

		//WC_CORE_INFO("Acquire result: {}, {}", magic_enum::enum_name(r), (int)r);

		Globals.UpdateTime();
		editor.Update();

		auto extent = Globals.window.GetExtent();

		if (extent.width > 0 && extent.height > 0)
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
				.renderArea.extent = extent,
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
				WC_CORE_ERROR("Presentation result: {}", magic_enum::enum_name(presentationResult));
			}

			vk::SyncContext::UpdateFrame();
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	void OnFrameUpdate()
	{
		while (Globals.window.IsOpen())
		{
			Globals.window.PoolEvents();

		    // Resize window
			{
			    // one struct for the operations
                struct ResizeState {
                    bool active = false;
                    bool edges[4] = {}; // [left, right, top, bottom]
                    glm::ivec2 initialPos;
                    glm::ivec2 initialSize;
                    glm::ivec2 initialMouseGlobal;
                };
                static ResizeState resize;

			    // get window variables
                auto& window = Globals.window;
                const auto mousePos = window.GetCursorPos();
                const auto windowSize = window.GetSize();
                const auto windowPos = window.GetPosition();
                constexpr int borderSize = 5;

			    // calculate global mouse position
                const glm::ivec2 globalMousePos = windowPos + mousePos;

			    //calculate edges
                enum Edges { LEFT = 0, RIGHT, TOP, BOTTOM };
                bool edgeHover[4] = {
                    (mousePos.x >= -borderSize) && (mousePos.x <= borderSize),
                    (mousePos.x >= windowSize.x - borderSize) && (mousePos.x <= windowSize.x + borderSize),
                    (mousePos.y >= -borderSize) && (mousePos.y <= borderSize),
                    (mousePos.y >= windowSize.y - borderSize) && (mousePos.y <= windowSize.y + borderSize)
                };

                // check edges when window has focus and not resizing
                const bool canResize = window.HasFocus() && !resize.active;
                for (auto& edge : edgeHover) edge &= canResize;

                // resize
                if (!Globals.window.IsMaximized() && !resize.active && ImGui::IsMouseClicked(0)) {
                    for (int i = 0; i < 4; i++) {
                        if (edgeHover[i]) {
                            resize.active = true;
                            resize.edges[i] = true;
                            resize.initialPos = windowPos;
                            resize.initialSize = windowSize;
                            resize.initialMouseGlobal = globalMousePos;
                            break;
                        }
                    }
                }

                // ongoing resize
                if (resize.active) {
                    if (ImGui::IsMouseDragging(0)) {
                        const glm::ivec2 delta = globalMousePos - resize.initialMouseGlobal;

                        // Horizontal
                        if (resize.edges[LEFT]) {
                            window.SetPosition({ resize.initialPos.x + delta.x, resize.initialPos.y });
                            window.SetSize({ resize.initialSize.x - delta.x, windowSize.y });
                        }
                        else if (resize.edges[RIGHT]) {
                            window.SetSize({ resize.initialSize.x + delta.x, windowSize.y });
                        }

                        // Vertical
                        if (resize.edges[TOP]) {
                            window.SetPosition({ resize.initialPos.x, resize.initialPos.y + delta.y });
                            window.SetSize({ windowSize.x, resize.initialSize.y - delta.y });
                        }
                        else if (resize.edges[BOTTOM]) {
                            window.SetSize({ windowSize.x, resize.initialSize.y + delta.y });
                        }
                    }

                    if (ImGui::IsMouseReleased(0)) {
                        memset(&resize, 0, sizeof(resize));
                    }
                }

                // Update cursor
                ImGuiMouseCursor cursor = ImGuiMouseCursor_Arrow;
                if (!Globals.window.IsMaximized() && (resize.active || canResize)) {
                    const bool horizontal = edgeHover[LEFT] || edgeHover[RIGHT] || resize.edges[LEFT] || resize.edges[RIGHT];
                    const bool vertical = edgeHover[TOP] || edgeHover[BOTTOM] || resize.edges[TOP] || resize.edges[BOTTOM];

                    if (horizontal) cursor = ImGuiMouseCursor_ResizeEW;
                    else if (vertical) cursor = ImGuiMouseCursor_ResizeNS;
                }
                if (!gui::IsAnyItemHovered())gui::SetMouseCursor(cursor);
            }

			if (Globals.window.HasFocus()) editor.Input();

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