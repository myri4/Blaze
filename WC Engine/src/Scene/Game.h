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

//ECS
#include "Scene.h"
#include "Components.h"

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../Globals.h"
#include "../Rendering/Renderer2D.h"
#include "../UI/Widgets.h"

namespace wc
{
	struct GameInstance
	{
	protected:

		OrthographicCamera camera;

		RenderData m_RenderData;
		Renderer2D m_Renderer;
		Font font;

		Texture t_Close;
		Texture t_Minimize;
		Texture t_Collapse;

	public:	

		std::string text;

		//ECS testing
		Scene scene;
		flecs::entity ent1, ent2;

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

			// Load Textures
			t_Close.Load("assets/textures/menu/close.png");
			t_Minimize.Load("assets/textures/menu/minimize.png");
			t_Collapse.Load("assets/textures/menu/collapse.png");

			// Entity Loading
			ent1 = scene.AddEntity();

			ent1.add<PositionComponent>();
			ent1.set<PositionComponent>({ { 0.0f, 0.0f } });

			ent1.add<VelocityComponent>();
			ent1.set<VelocityComponent>({ { 0.0f, 0.0f } });

			ent1.add<ScaleComponent>();
			ent1.set<ScaleComponent>({ { 1.0f, 1.0f } });

			ent2 = scene.AddEntity();

			ent2.add<PositionComponent>();
			ent2.set<PositionComponent>({ { -10.0f, 0.0f } });

			ent2.add<VelocityComponent>();
			ent2.set<VelocityComponent>({ { 0.0f, 0.0f } });

			ent2.add<ScaleComponent>();
			ent2.set<ScaleComponent>({ { 1.0f, 1.0f } });
		}
		
		void InputGame()
		{
			if (Key::GetKey(Key::F) != GLFW_RELEASE)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize(Globals.window.GetSize());
			}

			if (Key::GetKey(Key::Up) != GLFW_RELEASE)
			{
				ent1.set<VelocityComponent>({ {0.0f, 0.005f} });
				ent2.set<VelocityComponent>({ {0.0f, 0.005f} });
			}

			if (Key::GetKey(Key::Left) != GLFW_RELEASE)
			{
				ent1.set<VelocityComponent>({ {-0.01f, 0.0f} });
				ent2.set<VelocityComponent>({ {-0.01f, 0.0f} });
			}

			if (Key::GetKey(Key::Down) != GLFW_RELEASE)
			{
				ent1.set<VelocityComponent>({ {0.0f, -0.005f} });
				ent2.set<VelocityComponent>({ {0.0f, -0.005f} });
			}

			if (Key::GetKey(Key::Right) != GLFW_RELEASE)
			{
				ent1.set<VelocityComponent>({ {0.01f, 0.0f} });
				ent2.set<VelocityComponent>({ {0.01f, 0.0f} });
			}

			if (Key::GetKey(Key::Space) != GLFW_RELEASE)
			{
				ent1.set<VelocityComponent>({ {0.0f, 0.0f} });
				ent2.set<VelocityComponent>({ {0.0f, 0.0f} });
			}

			if (Key::GetKey(Key::W) != GLFW_RELEASE)
			{
				ent1.set<ScaleComponent>({ ent1.get<ScaleComponent>()->scale + glm::vec2{0.0f, 0.002f} });
				ent2.set<ScaleComponent>({ ent2.get<ScaleComponent>()->scale + glm::vec2{0.0f, 0.002f} });
			}

			if (Key::GetKey(Key::A) != GLFW_RELEASE)
			{
				ent1.set<ScaleComponent>({ ent1.get<ScaleComponent>()->scale + glm::vec2{-0.002f, 0.0f} });
				ent2.set<ScaleComponent>({ ent2.get<ScaleComponent>()->scale + glm::vec2{-0.002f, 0.0f} });
			}

			if (Key::GetKey(Key::S) != GLFW_RELEASE)
			{
				ent1.set<ScaleComponent>({ ent1.get<ScaleComponent>()->scale + glm::vec2{0.0f, -0.002f} });
				ent2.set<ScaleComponent>({ ent2.get<ScaleComponent>()->scale + glm::vec2{0.0f, -0.002f} });
			}

			if (Key::GetKey(Key::D) != GLFW_RELEASE)
			{
				ent1.set<ScaleComponent>({ ent1.get<ScaleComponent>()->scale + glm::vec2{0.002f, 0.0f} });
				ent2.set<ScaleComponent>({ ent2.get<ScaleComponent>()->scale + glm::vec2{0.002f, 0.0f} });
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

			//m_RenderData.DrawString(std::format("{}", 1.f / Globals.deltaTime), font, {0.0f, 0.0f});

			scene.GetWorld().each([](PositionComponent& p, VelocityComponent& v) {
				p.position += v.velocity;
			});

			m_RenderData.DrawString(std::format("{}", 1.f / Globals.deltaTime), font, ent1.get<PositionComponent>()->position, ent1.get<ScaleComponent>()->scale, 0.0f, color);
			m_RenderData.DrawString(std::format("{}", 1.f / Globals.deltaTime), font, ent2.get<PositionComponent>()->position, ent2.get<ScaleComponent>()->scale, 0.0f, color);

			m_Renderer.Flush(m_RenderData);


			m_RenderData.Reset();
		}

		void Update()
		{
			

			RenderGame();
		}

		void UI_Scene()
		{
		}
		void UI_Editor()
		{
		}
		void UI_Settings()
		{
		}
		void UI_Entities()
		{
		}
		void UI_Properties()
		{
		}

			//auto windowPos = (glm::vec2)Globals.window.GetPos();
			//ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetImguiImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
		void UI()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

			ImGui::Begin("Screen Render", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

			// Main Menu Bar
			{
				//ImGui::SetWindowFontScale(2.f);
				if (ImGui::BeginMenuBar())
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
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.0f));
					float buttonSize = ImGui::GetFrameHeightWithSpacing();
					float spacing = 5.0f;

					ImGui::SameLine(ImGui::GetContentRegionMax().x - 3 * (buttonSize + spacing));
					if (ImGui::ImageButton(t_Collapse, { buttonSize, buttonSize }))
					{
						//Globals.window.Collapse();
					}

					ImGui::SameLine();
					if (ImGui::ImageButton(t_Minimize, { buttonSize, buttonSize }))
					{
						Globals.HandleWindowState();
					}

					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92, 0.25f, 0.2f, 1.f));
					ImGui::SameLine();
					if (ImGui::ImageButton(t_Close, { buttonSize, buttonSize }))
					{
						Globals.window.Close();
					}
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));

					ImGui::PopStyleVar(2);

					ImGui::EndMenuBar();
				}
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