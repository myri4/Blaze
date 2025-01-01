#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include <wc/vk/SyncContext.h>

#include <wc/Utils/Time.h>
#include <wc/Utils/List.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/YAML.h>

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../Globals.h"
#include "../Rendering/Renderer2D.h"
#include "UI/Widgets.h"

namespace wc
{
	struct GameInstance
	{
	protected:

		OrthographicCamera camera;

		RenderData m_RenderData;
		Renderer2D m_Renderer;
		Font font;
	public:	

		std::string text;

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();
			font.Load("assets/fonts/ST-SimpleSquare.ttf", m_RenderData);

			m_Renderer.bloom.Init();
			m_Renderer.composite.Init();
			m_Renderer.crt.Init();
			m_Renderer.camera = &camera;
			m_Renderer.Init(m_RenderData);
			m_Renderer.CreateScreen(renderSize);

			for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
			{
				vk::SyncContext::GraphicsCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Renderer.m_Cmd[i]);
				vk::SyncContext::ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Renderer.m_ComputeCmd[i]);
			}

			text = "myri4\nmyri4";
		}
		
		void InputGame()
		{
			if (Key::GetKey(Key::F) != GLFW_RELEASE)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize(Globals.window.GetSize());
			}
		}

		glm::vec2 p0 = glm::vec2(0.f, 0.f);
		glm::vec2 p1 = glm::vec2(2.5f, 2.5f);
		glm::vec2 p2 = glm::vec2(2.7f, -2.5f);
		glm::vec2 p3 = glm::vec2(5.f, 0.f);

		
		void RenderGame()
		{
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();

			//m_RenderData.DrawQuad({ 0.f, 0.f, 0.f }, { 1.f, 1.f }, 0u, { 1.f, 1.f, 1.f, 1.f });
			auto color = glm::vec4(0.f, 1.f, 0.5f, 1.f) * 2.5f;
			//m_RenderData.DrawLine(p0, p1, color);
			//m_RenderData.DrawLine(p2, p3, color);
			//m_RenderData.DrawCircle({ p0.x, p0.y, 0.f }, 0.1f);
			//m_RenderData.DrawCircle({ p1.x, p1.y, 0.f }, 0.1f);
			//m_RenderData.DrawCircle({ p2.x, p2.y, 0.f }, 0.1f);
			//m_RenderData.DrawCircle({ p3.x, p3.y, 0.f }, 0.1f);

			//m_RenderData.DrawBezierCurve(p0, p1, p2, p3, color);
			m_RenderData.DrawString(std::format("{}", 1.f / Globals.deltaTime), font, {0.f, 0.f});

			m_Renderer.Flush(m_RenderData);


			m_RenderData.Reset();
		}

		void Update()
		{
			

			RenderGame();
		}

		void UI()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.140f, 0.140f, 0.140f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			
			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

			// Main Menu Bar
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Exit"))
					{
						Globals.window.Close();
					}
					ImGui::EndMenu();
				}

				// Buttons
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.0f));

				float buttonWidth = 45.0f;
				float spacing = 5.0f;

				ImGui::SameLine(ImGui::GetContentRegionMax().x - 3 * (buttonWidth + spacing));

				if (ImGui::Button("_", { buttonWidth, ImGui::GetContentRegionMax().y }))
				{
					// Collapse
				}

				ImGui::SameLine();
				if (ImGui::Button("[]", { buttonWidth, ImGui::GetContentRegionMax().y }))
				{
					// Maximize / Minimize
				}

				ImGui::SameLine();
				if (ImGui::Button("X", { buttonWidth, ImGui::GetContentRegionMax().y }))
				{
					Globals.window.Close();
				}

				ImGui::PopStyleVar();



				ImGui::EndMainMenuBar();
			}

			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetImguiImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
			
			ImGui::End();

			ImGui::PopStyleVar(3);
		}


		void Resize(glm::vec2 size)
		{
			m_Renderer.DestroyScreen();
			m_Renderer.CreateScreen(size);
		}

		void Destroy()
		{
			m_Renderer.Deinit();

			m_RenderData.Destroy();
		}
	};
}