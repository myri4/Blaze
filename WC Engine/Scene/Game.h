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

#include <glm/gtc/type_ptr.hpp>

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
#include <imgui/ImGuizmo.h>

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

		glm::vec2 WindowPos;
		glm::vec2 RenderSize;

		bool allowInput = true;

		ImGuizmo::OPERATION m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;

		bool showEditor = true;
		bool showSettings = true;
		bool showEntities = true;
		bool showProperties = true;
		bool showConsole = true;
		bool showFileExplorer = true;

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
			ent1 = scene.AddEntity("Entity 1");

			ent1.add<PositionComponent>();
			ent1.set<PositionComponent>({ { 0.0f, 0.0f } });

			ent1.add<VelocityComponent>();
			ent1.set<VelocityComponent>({ { 0.0f, 0.0f } });

			ent1.add<ScaleComponent>();
			ent1.set<ScaleComponent>({ { 1.0f, 1.0f } });

			ent2 = scene.AddEntity("Entity 2");

			ent2.add<PositionComponent>();
			ent2.set<PositionComponent>({ { -10.0f, 0.0f } });

			ent2.add<VelocityComponent>();
			ent2.set<VelocityComponent>({ { 0.0f, 0.0f } });

			ent2.add<ScaleComponent>();
			ent2.set<ScaleComponent>({ { 1.0f, 1.0f } });
			ent2.child_of(ent1);

		}

		void InputGame()
		{
			if (allowInput)
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

				if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;
				//if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GuizmoOp = ImGuizmo::OPERATION::ROTATE_Z;
			}
		}

		void RenderGame()
		{
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();

			auto color = glm::vec4(0.f, 1.f, 0.5f, 1.f) * 2.5f;

			//m_RenderData.DrawQuad({ 0.f, 0.f, 0.f }, { 50, 50 }, 0u, { 1.f, 1.f, 1.f, 1.f });

			//for (float y = windowHeight / 2.0f; y >= -windowHeight / 2.0f; y -= 1.0f)
			//{
			//	glm::vec3 p0 = { -windowWidth / 2.0f, y, 0.0f };
			//	glm::vec3 p1 = { windowWidth / 2.0f, y, 0.0f };
			//	m_RenderData.DrawLine(p0, p1, color);
			//}

			scene.GetWorld().each([](PositionComponent& p, VelocityComponent& v) {
				p.position += v.velocity;
				});

			m_RenderData.DrawString("Entity 1", font, ent1.get<PositionComponent>()->position, ent1.get<ScaleComponent>()->scale, 0.0f, color);
			m_RenderData.DrawString("Entity 2", font, ent2.get<PositionComponent>()->position, ent2.get<ScaleComponent>()->scale, 0.0f, color);

			// fps = std::format("{}", 1.f / Globals.deltaTime


			m_Renderer.Flush(m_RenderData);

			m_RenderData.Reset();
		}

		void Update()
		{
			//scene.GetWorld().each([](flecs::entity e, LookupTag t) {
			//	WC_CORE_INFO(e.name());
			//});

			RenderGame();
		}

		flecs::entity selected_entity = flecs::entity::null();
		void UI_Editor()
		{
			// TODO - ADD MENU BAR!
			ImGui::Begin("Editor", &showEditor);

			allowInput = ImGui::IsWindowFocused();

			ImVec2 drawPos, drawSize;
			// Render Game
			{
				// Add padding and account for the tab height
				const float padding = 10.0f; // Adjust padding as needed
				float tabHeight = ImGui::GetFrameHeight(); // Height of the window tab bar
				ImVec2 availableSize = ImVec2(ImGui::GetWindowSize().x - 2 * padding, ImGui::GetWindowSize().y - tabHeight - 2 * padding);

				// Get the aspect ratio of the image
				float imageAspectRatio = m_Renderer.GetAspectRatio();

				// Calculate the maximum size while maintaining aspect ratio
				float availableAspectRatio = availableSize.x / availableSize.y;

				if (availableAspectRatio > imageAspectRatio)
				{
					// Available area is wider than the image aspect ratio, fit to height
					drawSize.y = availableSize.y;
					drawSize.x = drawSize.y * imageAspectRatio;
				}
				else
				{
					// Available area is taller than the image aspect ratio, fit to width
					drawSize.x = availableSize.x;
					drawSize.y = drawSize.x / imageAspectRatio;
				}

				// Center the image within the available space
				drawPos = ImVec2(
					ImGui::GetWindowPos().x + padding + (availableSize.x - drawSize.x) * 0.5f,
					ImGui::GetWindowPos().y + tabHeight + padding + (availableSize.y - drawSize.y) * 0.5f
				);

				// Draw the image
				ImGui::GetWindowDrawList()->AddImage(
					m_Renderer.GetImguiImageID(),
					drawPos,
					ImVec2(drawPos.x + drawSize.x, drawPos.y + drawSize.y)
				);
			}

			glm::mat4 projection = camera.GetProjectionMatrix();
			projection[1][1] *= -1;

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(drawPos.x, drawPos.y, drawSize.x, drawSize.y);

			if (selected_entity != flecs::entity::null())
			{
				auto position = selected_entity.get<PositionComponent>()->position;
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(position, 0.f));

				ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()), glm::value_ptr(projection), m_GuizmoOp, ImGuizmo::MODE::LOCAL, glm::value_ptr(transform));

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					DecomposeTransform(transform, translation, rotation, scale);
					selected_entity.set<PositionComponent>({ glm::vec2(translation) });
				}
			}

			ImGui::End();
		}

		void UI_Settings()
		{
			ImGui::Begin("Settings", &showSettings);

			ImGui::Separator();

			ImGui::End();
		}

		flecs::entity ShowEntityTree(flecs::entity e, flecs::entity parent, flecs::entity& selected_entity) {
			static int selection_mask = 0;  // selected entity

			// Check if the mouse is clicked outside the tree nodes
			if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsWindowFocused()) {
				selection_mask = 0;
				selected_entity = flecs::entity::null();
			}

			if (parent != flecs::entity::null() && !e.is_a(flecs::ChildOf))
				return flecs::entity::null(); // Skip if entity is not a child 

			// Get children
			std::vector<flecs::entity> children;
			e.children([&children](flecs::entity child) {
				children.push_back(child);
				});

			bool is_selected = (selection_mask & (1 << e.id())) != 0;

			// check if a child is selected - keep parent node open
			bool keep_open = false;
			for (const auto& child : children) {
				if ((selection_mask & (1 << child.id())) != 0) {
					keep_open = true;
					break;
				}
			}

			if (keep_open)
				ImGui::SetNextItemOpen(true); // set node open

			ImGuiTreeNodeFlags node_flags = (children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);

			if (is_selected) {
				node_flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool is_open = ImGui::TreeNodeEx(e.name().c_str(), node_flags);
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				selection_mask = (1 << e.id());  // Select the current node
				selected_entity = e;  // Update the selected entity
			}

			if (is_open) {
				// If the node is open, recursively show its children
				for (const auto& child : children) {
					ShowEntityTree(child, e, selected_entity);  // Pass the current entity as the parent for the child
				}
				if (!children.empty()) {
					ImGui::TreePop();  // Close node
				}
			}

			return selected_entity;
		}

		void UI_Entities()
		{
			ImGui::Begin("Entities", &showEntities);

			scene.GetWorld().each([this](flecs::entity e, LookupTag p)
				{
					// Only call ShowEntityTree for root entities (entities with no parent)
					if (e.parent() == flecs::entity::null()) {
						ShowEntityTree(e, flecs::entity::null(), selected_entity);
					}
				});

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 40, ImGui::GetWindowSize().y - 40));
			if (ImGui::Button("+", { 30, 30 }))
			{
				ImGui::OpenPopup("Add Entity");
			}
			ImGui::SetItemTooltip("Add Entity");
			ImGui::PopStyleVar();

			if (ImGui::BeginPopupModal("Add Entity", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
			{
				static std::string name = "";
				ImGui::InputText("Name", &name);

				if (ImGui::Button("Create")) {
					if (!name.empty()) {
						// create entity with name
						scene.AddEntity(name);
						name.clear();
						ImGui::CloseCurrentPopup();
					}
					else {
						ImGui::SetNextWindowPos(ImGui::GetMousePos());
						ImGui::OpenPopup("WarnEmptyName");
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					//ImGui::ClosePopupsOverWindow(ImGui::FindWindowByName("Entities"), true); 
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::BeginPopupModal("WarnEmptyName", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
				{
					ImGui::Text("Name cannot be empty!");

					if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}

				ImGui::EndPopup();
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered() && selected_entity != flecs::entity::null())
			{
				ImGui::OpenPopup("RightClickPopup");
			}

			if (ImGui::BeginPopup("RightClickPopup"))
			{
				ImGui::Text("Selected Entity");

				if (ImGui::Button("Add Child"))
				{
					//scene.AddEntity("Child Entity", selected_entity); TODO
				}

				if (ImGui::Button("Add Component"))
				{
					//ImGui::OpenPopup("AddComponent"); TODO
				}

				if (ImGui::Button("Copy"))
				{
					// copies components but not children + asks to rename TODO

				}

				if (ImGui::Button("Delete"))
				{
					scene.KillEntity(selected_entity);
					selected_entity = flecs::entity::null();
				}

				ImGui::EndPopup();
			}

			ImGui::End();
		}

		void UI_Properties()
		{
			ImGui::Begin("Properties", &showProperties);

			if (selected_entity != flecs::entity::null())
			{
				std::string nameBuffer = selected_entity.name().c_str(); // Buffer to hold the entity's name

				// Allow the user to edit the name in the buffer
				if (ImGui::InputText("Name", &nameBuffer)) {
					// Update the entity's name if it has changed
					selected_entity.set_name(nameBuffer.c_str());
				}


				// Display the entity's ID
				//ImGui::Text("ID: %u", selected_entity.id());

				//NOTE: for every new component, a new if is needed
				ImGui::SeparatorText("Components");

				if (selected_entity.has<PositionComponent>())
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Position"))
					{
						auto& p = *selected_entity.get<PositionComponent>();
						float position[2] = { p.position.x, p.position.y };

						if (ImGui::InputFloat2("Position", position))
						{
							// Use const_cast to modify the original position
							const_cast<glm::vec2&>(p.position) = glm::vec2(position[0], position[1]);
						}

					}
				}

				if (selected_entity.has<VelocityComponent>())
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Velocity"))
					{
						auto& v = *selected_entity.get<VelocityComponent>();
						float velocity[2] = { v.velocity.x, v.velocity.y };

						if (ImGui::InputFloat2("Velocity", velocity))
						{
							// Use const_cast to modify the original velocity
							const_cast<glm::vec2&>(v.velocity) = glm::vec2(velocity[0], velocity[1]);
						}
					}
				}

				if (selected_entity.has<ScaleComponent>())
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Scale"))
					{
						auto& s = *selected_entity.get<ScaleComponent>();
						float scale[2] = { s.scale.x, s.scale.y };

						if (ImGui::InputFloat2("Scale", scale))
						{
							// Use const_cast to modify the original scale
							const_cast<glm::vec2&>(s.scale) = glm::vec2(scale[0], scale[1]);
						}
					}
				}
			}

			ImGui::End();
		}

		// @TODO fix this
		void UI_Console()
		{
			ImGui::Begin("Console", &showConsole);

			if (ImGui::Button("Clear"))
			{
				//Globals.console.Clear();
				Log::GetConsoleSink()->messages.clear();
				
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Copy"))
			{
				WC_INFO("TODO - IMPLEMENT COPY");
			}

			ImGui::SameLine();
			ImGui::InputText("Input", const_cast<char*>(""), 0);

			ImGui::Separator();

			const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
			{
				static const std::unordered_map<spdlog::level::level_enum, std::pair<glm::vec4, std::string>> level_colors = {
					{spdlog::level::debug, {glm::vec4(58.f, 150.f, 221.f, 255.f) / 255.f, "[debug] "}},
					{spdlog::level::info, {glm::vec4(19.f, 161.f, 14.f, 255.f) / 255.f, "[info] "}},
					{spdlog::level::warn, {glm::vec4(249.f, 241.f, 165.f, 255.f) / 255.f, "[warn!] "}},
					{spdlog::level::err, {glm::vec4(231.f, 72.f, 86.f, 255.f) / 255.f, "[-ERROR-] "}},
					{spdlog::level::critical, {glm::vec4(139.f, 0.f, 0.f, 255.f) / 255.f, "[!CRITICAL!] "}}
				};

				for (auto& msg : Log::GetConsoleSink()->messages)
				{
					auto it = level_colors.find(msg.level);
					if (it != level_colors.end())
					{
						const auto& [color, prefix] = it->second;
						ImGui::PushStyleColor(ImGuiCol_Text, { color.r, color.g, color.b, color.a });
						UI::Text(prefix + msg.payload);
						ImGui::PopStyleColor();
					}
					else
					{
						UI::Text(msg.payload);
					}
				}

				if (/*ScrollToBottom ||*/
					(/*m_ConsoleAutoScroll && */ ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
					ImGui::SetScrollHereY(1.f);

				ImGui::EndChild();
			}

			ImGui::End();
		}

		void UI_FileExplorer()
		{
			ImGui::Begin("File Explorer", &showFileExplorer);

			ImGui::Separator();

			ImGui::End();
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

			ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
				| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);

			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f));
			}

			ImGui::PopStyleVar(3);

			// Main Menu Bar
			{
				if (ImGui::BeginMenuBar())
				{

					// TODO - add Dragging and Turn of GLFW tab bar -> custom
					if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
					{
						//WC_CORE_INFO("Empty space on main menu bar is hovered)"
					}



					if (ImGui::BeginMenu("File"))
					{
						if (ImGui::MenuItem("New"))
						{
							WC_CORE_INFO("New");
						}

						if (ImGui::MenuItem("Open"))
						{
							WC_CORE_INFO("Open");
						}

						if (ImGui::MenuItem("Save"))
						{
							WC_CORE_INFO("Save");
						}

						if (ImGui::MenuItem("Save As"))
						{
							WC_CORE_INFO("Save As");
						}

						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("View"))
					{
						ImGui::MenuItem("Editor", NULL, &showEditor);
						ImGui::MenuItem("Settings", NULL, &showSettings);
						ImGui::MenuItem("Entities", NULL, &showEntities);
						ImGui::MenuItem("Properties", NULL, &showProperties);
						ImGui::MenuItem("Console", NULL, &showConsole);
						ImGui::MenuItem("File Explorer", NULL, &showFileExplorer);

						ImGui::EndMenu();
					}

					// Buttons
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 1.f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

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

					ImGui::PopStyleVar(1);
					ImGui::PopStyleColor(5);

					ImGui::EndMenuBar();
				}
			}

			if (showEditor)UI_Editor();
			if (showSettings)UI_Settings();
			if (showEntities)UI_Entities();
			if (showProperties)UI_Properties();
			if (showConsole)UI_Console();
			if (showFileExplorer)UI_FileExplorer();

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