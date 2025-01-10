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
#include "Scene/Scene.h"

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/ImGuizmo.h>

#include "Globals.h"
#include "Rendering/Renderer2D.h"
#include "UI/Widgets.h"

namespace wc
{
	struct Editor
	{
	private:
		// Panning variables
		glm::vec2 m_StartPan;
		glm::vec2 m_BeginCameraPosition;
		bool m_Panning = false;


		OrthographicCamera camera;

		RenderData m_RenderData;
		Renderer2D m_Renderer;

		Texture t_Close;
		Texture t_Minimize;
		Texture t_Collapse;

		glm::vec2 WindowPos;
		glm::vec2 RenderSize;

		glm::vec2 ViewPortSize = glm::vec2(1.f);

		bool allowInput = true;

		ImGuizmo::OPERATION m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;

		bool showEditor = true;
		bool showSettings = true;
		bool showEntities = true;
		bool showProperties = true;
		bool showConsole = true;
		bool showFileExplorer = true;

		flecs::entity selected_entity = flecs::entity::null();

		enum class PlayState { Paused, Simulate, Play };

		Scene scene;

		// Settings

		float ZoomSpeed = 2.f;
	public:

		//ECS testing
		flecs::entity ent1, ent2;

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();

			m_Renderer.camera = &camera;
			m_Renderer.Init(m_RenderData);
			m_Renderer.CreateScreen(renderSize);

			// Load Textures
			t_Close.Load("assets/textures/menu/close.png");
			t_Minimize.Load("assets/textures/menu/minimize.png");
			t_Collapse.Load("assets/textures/menu/collapse.png");

			// Entity Loading
			ent1 = scene.AddEntity("Entity 1");

			ent1.set<PositionComponent>({ { 0.0f, 0.0f } });

			ent1.set<ScaleComponent>({ { 1.0f, 1.0f } });
			ent1.set<CircleRendererComponent>({});

			ent2 = scene.AddEntity("Entity 2");

			ent2.set<PositionComponent>({ { -10.0f, 0.0f } });

			ent2.set<ScaleComponent>({ { 1.0f, 1.0f } });
			ent2.set<SpriteRendererComponent>({});
			ent2.child_of(ent1);
		}

		void Input()
		{
			if (allowInput)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;
				if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GuizmoOp = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Y;

				float scroll = Mouse::GetMouseScroll().y;
				if (scroll != 0.f)
				{
					camera.Zoom += -scroll * ZoomSpeed;
					camera.Zoom = glm::max(camera.Zoom, 0.05f);
					camera.Update(m_Renderer.GetHalfSize());
				}

				glm::vec2 mousePos = (glm::vec2)(Globals.window.GetCursorPos() + Globals.window.GetPos()) - WindowPos;
				glm::vec2 mouseFinal = m_BeginCameraPosition + m_Renderer.ScreenToWorld(mousePos);

				if (Mouse::GetMouse(Mouse::RIGHT))
				{
					if (!m_Panning)
					{
						m_StartPan = mouseFinal;
						m_BeginCameraPosition = camera.Position;
					}

					camera.Position = glm::vec3(m_BeginCameraPosition + (m_StartPan - mouseFinal), camera.Position.z);
					m_Panning = true;
				}
				else
				{
					m_StartPan = glm::vec2(0.f);
					m_BeginCameraPosition = camera.Position;
					m_Panning = false;
				}
			}
		}

		void Render()
		{
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();

			scene.GetWorld().each([&](flecs::entity entt, PositionComponent& p) {
				glm::vec2 scale = glm::vec2(1.f);
				float rotation = 0.f;

				if (entt.has<ScaleComponent>()) scale = entt.get<ScaleComponent>()->scale;
				if (entt.has<RotationComponent>()) rotation = entt.get<RotationComponent>()->rotation;
				
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(p.position, 0.f)) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { scale.x, scale.y, 1.f });
				
				if (entt.has<SpriteRendererComponent>())
				{
					auto& data = *entt.get<SpriteRendererComponent>();

					m_RenderData.DrawQuad(transform, data.Texture, data.Color);
				}
				else if (entt.has<CircleRendererComponent>())
				{
					auto& data = *entt.get<CircleRendererComponent>();
					m_RenderData.DrawCircle(transform, data.Thickness, data.Fade, data.Color);
				}
				else if (entt.has<TextRendererComponent>())
				{
					auto& data = *entt.get<TextRendererComponent>();

					m_RenderData.DrawString(data.Text, data.Font, transform, data.Color);
				}
			});

			m_Renderer.Flush(m_RenderData);

			m_RenderData.Reset();
		}

		void Update()
		{
			Render();
		}

		void UI_Editor()
		{
			// TODO - ADD MENU BAR!
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
			ImGui::Begin("Editor", &showEditor);

			allowInput = ImGui::IsWindowFocused();

			ImVec2 viewPortSize = ImGui::GetContentRegionAvail();
			if (ViewPortSize != *((glm::vec2*)&viewPortSize))
			{
				ViewPortSize = { viewPortSize.x, viewPortSize.y };

				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize(ViewPortSize);
			}

			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();
			ImVec2 viewportBounds[2];
			viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			WindowPos = *((glm::vec2*)&viewportBounds[0]);
			RenderSize = *((glm::vec2*)&viewportBounds[1]) - WindowPos;

			auto image = m_Renderer.GetImguiImageID();
			ImGui::Image(image, viewPortSize);

			glm::mat4 projection = camera.GetProjectionMatrix();
			projection[1][1] *= -1;

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(WindowPos.x, WindowPos.y, RenderSize.x, RenderSize.y);

			if (selected_entity != flecs::entity::null() && selected_entity.has<PositionComponent>())
			{
				auto position = selected_entity.get<PositionComponent>()->position;
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(position, 0.f));

				ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()), glm::value_ptr(projection), m_GuizmoOp, ImGuizmo::MODE::WORLD, glm::value_ptr(transform));

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					DecomposeTransform(transform, translation, rotation, scale);
					selected_entity.set<PositionComponent>({ glm::vec2(translation) });
				}
			}

			ImGui::PopStyleVar();
			ImGui::End();
		}

		void UI_Settings()
		{
			ImGui::Begin("Settings", &showSettings);

			ImGui::Separator();

			ImGui::End();
		}

		flecs::entity ShowEntityTree(flecs::entity e, flecs::entity parent) 
		{
			static int selection_mask = 0;  // selected entity

			// Check if the mouse is clicked outside the tree nodes
			if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsWindowFocused()) 
			{
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
			for (const auto& child : children) 
				if ((selection_mask & (1 << child.id())) != 0) 
				{
					keep_open = true;
					break;
				}
			
			if (keep_open)
				ImGui::SetNextItemOpen(true); // set node open

			ImGuiTreeNodeFlags node_flags = (children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);

			if (is_selected) 
				node_flags |= ImGuiTreeNodeFlags_Selected;
			

			bool is_open = ImGui::TreeNodeEx(e.name().c_str(), node_flags);
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) 
			{
				selection_mask = (1 << e.id());  // Select the current node
				selected_entity = e;  // Update the selected entity
			}

			if (is_open) 
			{
				// If the node is open, recursively show its children
				for (const auto& child : children) 
					ShowEntityTree(child, e);  // Pass the current entity as the parent for the child
				
				if (!children.empty()) 
					ImGui::TreePop();  // Close node				
			}

			return selected_entity;
		}

		void UI_Entities()
		{
			ImGui::Begin("Entities", &showEntities);

			scene.GetWorld().each([this](flecs::entity e, EntityTag p)
				{
					// Only call ShowEntityTree for root entities (entities with no parent)
					if (e.parent() == flecs::entity::null()) {
						ShowEntityTree(e, flecs::entity::null());
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

				if (ImGui::Button("Create")) 
				{
					if (!name.empty()) 
					{
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
				if (ImGui::Button("Cancel")) 
				{
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
						auto& position = const_cast<glm::vec2&>(p.position);

						Widgets::PositionUI(position);

					}
				}

				if (selected_entity.has<ScaleComponent>())
				{
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Scale"))
					{
						auto& s = *selected_entity.get<ScaleComponent>();
						auto& scale = const_cast<glm::vec2&>(s.scale);

						Widgets::ScaleUI(scale);
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
						//Globals.window.Maximize();
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

			if (showEditor) UI_Editor();
			if (showSettings) UI_Settings();
			if (showEntities) UI_Entities();
			if (showProperties) UI_Properties();
			if (showConsole) UI_Console();
			if (showFileExplorer) UI_FileExplorer();

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
			t_Close.Destroy();
			t_Collapse.Destroy();
			t_Minimize.Destroy();
			m_Renderer.Deinit();

			m_RenderData.Destroy();
		}
	};
}