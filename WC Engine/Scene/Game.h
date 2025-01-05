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

			auto color = glm::vec4(0.f, 1.f, 0.5f, 1.f) * 2.5f;

			// Draw horizontal lines across the whole screen
			float windowHeight = Globals.window.GetSize().y;
			float windowWidth = Globals.window.GetSize().x;
			m_RenderData.DrawQuad({ 0.f, 0.f, 0.f }, { windowWidth, windowHeight }, 0u, { 1.f, 1.f, 1.f, 1.f });

			for (float y = windowHeight / 2.0f; y >= -windowHeight / 2.0f; y -= 1.0f)
			{
				glm::vec3 p0 = { -windowWidth / 2.0f, y, 0.0f };
				glm::vec3 p1 = { windowWidth / 2.0f, y, 0.0f };
				m_RenderData.DrawLine(p0, p1, color);
			}

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
			//scene.GetWorld().each([](PositionComponent& p) {
			//	WC_CORE_INFO("penis");
			//});

			RenderGame();
		}

		void UI_Scene()
		{
			// TODO - ADD MENU BAR!
			ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoBackground);

			// Get the position and size of the Scene window
			ImVec2 windowPos = ImGui::GetWindowPos();
			ImVec2 windowSize = ImGui::GetWindowSize();

			// Add padding and account for the tab height
			const float padding = 10.0f; // Adjust padding as needed
			float tabHeight = ImGui::GetFrameHeight(); // Height of the window tab bar
			ImVec2 availableSize = ImVec2(windowSize.x - 2 * padding, windowSize.y - tabHeight - 2 * padding);

			// Get the aspect ratio of the image
			float imageAspectRatio = m_Renderer.GetAspectRatio();

			// Calculate the maximum size while maintaining aspect ratio
			ImVec2 drawSize;
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
			ImVec2 drawPos = ImVec2(
				windowPos.x + padding + (availableSize.x - drawSize.x) * 0.5f,
				windowPos.y + tabHeight + padding + (availableSize.y - drawSize.y) * 0.5f
			);

			// Draw the image
			ImGui::GetWindowDrawList()->AddImage(
				m_Renderer.GetImguiImageID(),
				drawPos,
				ImVec2(drawPos.x + drawSize.x, drawPos.y + drawSize.y)
			);

			ImGui::End();
		}

		void UI_Editor()
		{
			ImGui::Begin("Editor");

			ImGui::Separator();

			ImGui::End();
		}

		void UI_Settings()
		{
			ImGui::Begin("Settings");

			ImGui::Separator();

			ImGui::End();
		}

		flecs::entity ShowEntityTree(flecs::entity e, flecs::entity parent) 
		{
			static int selection_mask = 0;  // selected entity
			static flecs::entity selected_entity = flecs::entity::null();  // Track the selected entity

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

			if(keep_open)
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
					flecs::entity child_selected = ShowEntityTree(child, e);  // Pass the current entity as the parent for the child
					if (child_selected != flecs::entity::null()) {
						selected_entity = child_selected;  // Update the selected entity if a child is selected
					}
				}
				if (!children.empty()) {
					ImGui::TreePop();  // Close node
				}
			}

			return selected_entity;
		}

		flecs::entity selected_entity = flecs::entity::null(); 
		void UI_Entities()
		{
			ImGui::Begin("Entities", NULL);
	
			scene.GetWorld().each([this](flecs::entity e, LookupTag p) {
				// Only call ShowEntityTree for root entities (entities with no parent)
				if (e.parent() == flecs::entity::null()) {
					flecs::entity root_selected = this->ShowEntityTree(e, flecs::entity::null());
					if (root_selected != flecs::entity::null()) {
						selected_entity = root_selected;  // Update the selected entity if selected
					}
				}
				});

			// Get the window size
			ImVec2 windowSize = ImGui::GetWindowSize();

			// Set the cursor position to the bottom-right corner
			ImGui::SetCursorPos(ImVec2(windowSize.x - 40, windowSize.y - 40));

			// Create the button
			if (ImGui::Button("+", { 30, 30 }))
			{
				ImGui::OpenPopup("AddEntityModal");
			}

			// Declare a name buffer for InputText
			static char name[128] = "";  // Use a char array for InputText compatibility

			// Begin the modal popup
			if (ImGui::BeginPopupModal("AddEntityModal", NULL))
			{
				ImGui::InputText("Name", name, sizeof(name));
				if (ImGui::Button("Create") && name != "")
				{
					// Add logic to create the entity with the given name
					scene.AddEntity(std::string(name));  // Convert char array to std::string
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::End();
		}

		void UI_Properties()
		{
			ImGui::Begin("Properties");
			
			if (selected_entity != flecs::entity::null()) 
			{
				static char nameBuffer[256]; // Buffer to hold the entity's name

				// Copy the entity's name into the buffer (ensure the buffer is large enough)
				strncpy(nameBuffer, selected_entity.name().c_str(), sizeof(nameBuffer) - 1);
				nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // Null-terminate the string

				// Allow the user to edit the name in the buffer
				if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
					// Update the entity's name if it has changed
					selected_entity.set_name(nameBuffer);
				}

				// Display the entity's ID
				//ImGui::Text("ID: %u", selected_entity.id());

				if (ImGui::Button("Delete")) {
					// Delete the entity
					scene.KillEntity(selected_entity);
					selected_entity = flecs::entity::null(); // Clear the selected entity
				}

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

		void UI_Console()
		{
			ImGui::Begin("Console");

			ImGui::End();
		}

		void UI_FileExplorer()
		{
			ImGui::Begin("File Explorer");

			ImGui::Separator();

			ImGui::End();
		}

			//auto windowPos = (glm::vec2)Globals.window.GetPos();
			//ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetImguiImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
		glm::vec2 mousePos = { 0.f, 0.f };

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

					//// TODO - Window dragging - FIX THIS FUCKING SHIT
					//if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					//{
					//	// Get the mouse drag delta
					//	ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
					//
					//	// Get the current window position
					//	glm::vec2 currentPos = Globals.window.GetPos();
					//
					//	// Calculate the new window position
					//	glm::vec2 newWindowPos = { currentPos.x + dragDelta.x, currentPos.y + dragDelta.y };
					//
					//	// Set the GLFW window position to the new position
					//	Globals.window.SetPosition(newWindowPos);
					//
					//	// Reset the mouse drag delta to avoid cumulative effect
					//	ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
					//}



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

			UI_Scene();
			UI_Editor();
			UI_Settings();
			UI_Entities();
			UI_Properties();
			UI_Console();
			UI_FileExplorer();

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