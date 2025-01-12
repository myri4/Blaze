#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <ranges>
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
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imguizmo/ImGuizmo.h>

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
		bool showSceneProperties = true;
		bool showEntities = true;
		bool showProperties = true;
		bool showConsole = true;
		bool showFileExplorer = true;

		flecs::entity selected_entity = flecs::entity::null();

		enum class PlayState { Paused, Simulate, Play };
		PlayState m_PlayState = PlayState::Paused;

		Scene scene;

		// Settings

		float ZoomSpeed = 2.f;
	public:

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
		}

		void Input()
		{
			if (allowInput)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_G)) m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;
				else if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GuizmoOp = ImGuizmo::OPERATION::ROTATE_Z;
				else if (ImGui::IsKeyPressed(ImGuiKey_S)) m_GuizmoOp = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Y;

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

		void RenderEntity(flecs::entity entt, glm::mat4& transform) 
		{
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

				//m_RenderData.DrawString(data.Text, data.Font, transform, data.Color);
			}
			scene.GetWorld().query_builder<TransformComponent, EntityTag>()
				.with(flecs::ChildOf, entt)
				.each([&](flecs::entity child, TransformComponent childTransform, EntityTag)
					{
						transform = transform * childTransform.GetTransform();
						RenderEntity(child, transform);
					});
		}

		void Render()
		{
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();

			scene.GetWorld().each([&](flecs::entity entt, TransformComponent& p) {
				if (entt.parent() != 0) return;

				glm::mat4 transform = p.GetTransform();
				RenderEntity(entt, transform);				
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

			allowInput = ImGui::IsWindowFocused() && ImGui::IsWindowHovered();

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
			ImGui::GetWindowDrawList()->AddImage(image, ImVec2(WindowPos.x, WindowPos.y), ImVec2(WindowPos.x + RenderSize.x, WindowPos.y + RenderSize.y));


			float buttonsWidth = ImGui::CalcTextSize("Play").x + ImGui::CalcTextSize("Stop").x + ImGui::CalcTextSize("Simulate").x + ImGui::GetStyle().FramePadding.x * 6.0f;
			float totalWidth = buttonsWidth + ImGui::GetStyle().ItemSpacing.x * 2;
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);
			bool isPlayingOrSimulating = (m_PlayState == PlayState::Play || m_PlayState == PlayState::Simulate);
			bool isPaused = (m_PlayState == PlayState::Paused);
			if (isPlayingOrSimulating)
				ImGui::BeginDisabled();
			if (ImGui::Button("Play") && isPaused)
			{
				m_PlayState = PlayState::Play;
				WC_INFO("Play");
			}
			ImGui::SameLine();
			if (ImGui::Button("Simulate") && isPaused)
			{
				m_PlayState = PlayState::Simulate;
				WC_INFO("Simulate");
			}
			if (isPlayingOrSimulating)
				ImGui::EndDisabled();
			ImGui::SameLine();
			if (isPaused)
				ImGui::BeginDisabled();
			if (ImGui::Button("Stop"))
			{
				m_PlayState = PlayState::Paused;
				WC_INFO("Stop");
			}
			if (isPaused)
				ImGui::EndDisabled();


			glm::mat4 projection = camera.GetProjectionMatrix();
			projection[1][1] *= -1;

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(WindowPos.x, WindowPos.y, RenderSize.x, RenderSize.y);

			if (selected_entity != flecs::entity::null() && selected_entity.has<TransformComponent>())
			{
				glm::mat4 local_transform = selected_entity.get<TransformComponent>()->GetTransform();
				glm::mat4 world_transform = local_transform;
				glm::mat4 deltaMatrix;

				// Build the world transform by accumulating parent transforms
				auto parent = selected_entity.parent();
				while (parent != flecs::entity::null() && parent.has<TransformComponent>())
				{
					glm::mat4 parent_transform = parent.get<TransformComponent>()->GetTransform();
					world_transform = parent_transform * world_transform;
					parent = parent.parent();
				}

				ImGuizmo::Manipulate(
					glm::value_ptr(camera.GetViewMatrix()),
					glm::value_ptr(projection),
					m_GuizmoOp,
					ImGuizmo::MODE::LOCAL,
					glm::value_ptr(world_transform),
					glm::value_ptr(deltaMatrix)
				);

				if (ImGuizmo::IsUsing())
				{
					// Convert world transform back to local space
					glm::mat4 parent_world_transform(1.0f);
					parent = selected_entity.parent();
					if (parent != flecs::entity::null() && parent.has<TransformComponent>())
					{
						// Build parent's world transform
						auto current = parent;
						std::vector<flecs::entity> parent_transforms;

						while (current != flecs::entity::null() && current.has<TransformComponent>())
						{
							parent_transforms.push_back(current);
							current = current.parent();
						}

						// Multiply transforms from root to immediate parent
						for (auto it = parent_transforms.rbegin(); it != parent_transforms.rend(); ++it)
							parent_world_transform = parent_world_transform * (*it).get<TransformComponent>()->GetTransform();
					}

					// Convert world transform to local space
					glm::mat4 local_matrix = glm::inverse(parent_world_transform) * world_transform;

					// Decompose the local transform
					glm::vec3 translation, rotation, scale;
					DecomposeTransform(local_matrix, translation, rotation, scale);

					// Update the entity's local transform
					selected_entity.set<TransformComponent>({
						glm::vec2(translation),
						glm::vec2(scale),
						glm::degrees(rotation.z)
						});
				}
			}

			ImGui::PopStyleVar();
			ImGui::End();
		}

		void UI_SceneProperties()
		{
			if (ImGui::Begin("Scene Properties", &showSceneProperties))
			{
				UI::Separator("Physics world");

			}
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

			// Check for right-click and open popup
			if (ImGui::IsWindowHovered())
			{
				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup(std::to_string(e.id()).c_str());
				}
				else
				{
					if (selected_entity != flecs::entity::null() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup(std::to_string(selected_entity.id()).c_str());
				}
			}

			// Display the popup
			if (ImGui::BeginPopup(std::to_string(e.id()).c_str()))
			{
				ImGui::Text("% s", e.name().c_str());
				ImGui::Separator();

				if (ImGui::MenuItem("Clone"))
				{
					WC_CORE_INFO("Implement Clone");
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Export"))
				{
					WC_CORE_INFO("Implement Export");
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.92, 0.25f, 0.2f, 1.f));
				if (ImGui::MenuItem("Delete"))
				{
					scene.KillEntity(e);
					selected_entity = flecs::entity::null();
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopStyleColor();



				ImGui::EndPopup();
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

			if (ImGui::BeginPopupModal("Add Entity", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) 
			{
				static std::string name = "";
				ImGui::InputText("Name", &name);

				if (ImGui::Button("Create") || ImGui::IsKeyPressed(ImGuiKey_Enter))
				{
					if (!name.empty()) 
					{
						scene.AddEntity(name);
						name.clear();
						ImGui::CloseCurrentPopup();
					}
					else 
					{
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

			ImGui::End();
		}

		void UI_Properties()
		{
			ImGui::Begin("Properties", &showProperties);

			if (selected_entity != flecs::entity::null())
			{
				std::string nameBuffer = selected_entity.name().c_str(); // Buffer to hold the entity's name

				// Input name
				if (ImGui::InputText("Name", &nameBuffer)) selected_entity.set_name(nameBuffer.c_str());

				// Display the entity's ID
				//ImGui::Text("ID: %u", selected_entity.id());

				//NOTE: for every new component, a new if statement is needed
				ImGui::SeparatorText("Components");

				if (selected_entity.has<TransformComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Transform##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto& p = *selected_entity.get<TransformComponent>();
						auto& position = const_cast<glm::vec2&>(p.Translation);
						auto& scale = const_cast<glm::vec2&>(p.Scale);
						auto& rotation = const_cast<float&>(p.Rotation);

						// Draw position UI
						UI::DragButton2("Position", position);
						UI::DragButton2("Scale", scale);
						ImGui::SliderFloat("Rotation", &rotation, 0.0f, 360.0f);
						ImGui::Separator();

					}

					if (!visible) selected_entity.remove<TransformComponent>(); // add modal popup
				}

				if (selected_entity.has<SpriteRendererComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Sprite Renderer##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto& s = *selected_entity.get<SpriteRendererComponent>();
						auto& color = const_cast<glm::vec4&>(s.Color);
						ImGui::ColorEdit4("color", glm::value_ptr(color));
						ImGui::Separator();
					}

					if (!visible) selected_entity.remove<SpriteRendererComponent>();
				}

				if (selected_entity.has<CircleRendererComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Circle Renderer##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto& c = *selected_entity.get<CircleRendererComponent>();
						auto& thickness = const_cast<float&>(c.Thickness);
						auto& fade = const_cast<float&>(c.Fade);
						auto& color = const_cast<glm::vec4&>(c.Color);

						// Draw circle renderer UI
						ImGui::SliderFloat("Thickness", &thickness, 0.0f, 1.0f);
						ImGui::SliderFloat("Fade", &fade, 0.0f, 1.0f);
						ImGui::ColorEdit4("Color", glm::value_ptr(color));
						ImGui::Separator();
					}

					if (!visible) selected_entity.remove<CircleRendererComponent>(); // add modal popup
				}

				if (selected_entity.has<TextRendererComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Text Renderer##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto& t = *selected_entity.get<TextRendererComponent>();
						auto& text = const_cast<std::string&>(t.Text);
						//auto& font = const_cast<std::string&>(t.Font);
						auto& color = const_cast<glm::vec4&>(t.Color);
						// Draw text renderer UI
						ImGui::InputText("Text", &text);
						//ImGui::InputText("Font", &font);
						ImGui::ColorEdit4("Color", glm::value_ptr(color));
						ImGui::Separator();
					}
					if (!visible) selected_entity.remove<TextRendererComponent>(); // add modal popup
				}

				auto UI_PhysicsMaterial = [&](PhysicsMaterial& material)
					{
						UI::Separator("Material");
						UI::Drag("Density", material.Density);
						UI::Drag("Friction", material.Friction);
						UI::Drag("Restitution", material.Restitution);
						UI::Drag("Rolling Resistance", material.RollingResistance);

						UI::Separator();
						UI::Drag("Allowed Clip Fraction", material.AllowedClipFraction);
						ImGui::ColorEdit4("Debug Color", glm::value_ptr(material.DebugColor));

						UI::Separator();
						UI::Checkbox("Enable Sensor Events", material.EnableSensorEvents);
						UI::Checkbox("Enable Contact Events", material.EnableContactEvents);
						UI::Checkbox("Enable Hit Events", material.EnableHitEvents);
						UI::Checkbox("Enable Pre-Solve Events", material.EnablePreSolveEvents);
						UI::Checkbox("Invoke Contact Creation", material.InvokeContactCreation);
						UI::Checkbox("Update Body Mass", material.UpdateBodyMass);
					};

				if (selected_entity.has<RigidBodyComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Rigid Body Component##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto p = selected_entity.get_ref<RigidBodyComponent>();

						const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic"};
						const char* currentBodyTypeString = bodyTypeStrings[(int)p->Type];

						if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
						{
							for (int i = 0; i < 3; i++)
							{
								bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
								if (ImGui::Selectable(bodyTypeStrings[i], &isSelected))
								{
									currentBodyTypeString = bodyTypeStrings[i];
									p->Type = BodyType(i);
								}

								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						UI::Drag("Gravity Scale", p->GravityScale);
						UI::Drag("Linear Damping", p->LinearDamping);
						UI::Drag("Angular Damping", p->AngularDamping);
						UI::Checkbox("Fixed Rotation", p->FixedRotation);
						UI::Checkbox("Bullet", p->Bullet);
						UI::Checkbox("Fast Rotation", p->FastRotation);

						ImGui::Separator();
					}

					if (!visible) selected_entity.remove<BoxCollider2DComponent>(); // add modal popup
				}

				if (selected_entity.has<BoxCollider2DComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Box Collider##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto p = selected_entity.get_ref<BoxCollider2DComponent>();
						auto& offset = const_cast<glm::vec2&>(p->Offset);
						auto& size = const_cast<glm::vec2&>(p->Size);

						UI::DragButton2("Offset", offset);
						UI::DragButton2("Size", size);

						UI_PhysicsMaterial(p->Material);

						ImGui::Separator();

					}

					if (!visible) selected_entity.remove<BoxCollider2DComponent>(); // add modal popup
				}

				if (selected_entity.has<CircleCollider2DComponent>())
				{
					bool visible = true;
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::CollapsingHeader("Circle Collider##header", &visible, ImGuiTreeNodeFlags_None))
					{
						auto p = selected_entity.get_ref<CircleCollider2DComponent>();
						auto& offset = const_cast<glm::vec2&>(p->Offset);
						auto& radius = const_cast<float&>(p->Radius);

						UI::DragButton2("Offset", offset);
						UI::Drag("Radius", radius);

						UI_PhysicsMaterial(p->Material);

						ImGui::Separator();

					}

					if (!visible) selected_entity.remove<BoxCollider2DComponent>(); // add modal popup
				}

				static bool showAddComponent = false;
				if (ImGui::Button("Add Component")) showAddComponent = true;

				if (showAddComponent)
				{
					// Calculate clamp
					{
						// Calculate the desired position for the popup
						ImVec2 popupPos = ImGui::GetItemRectMin();
						popupPos.y = ImGui::GetItemRectMax().y + 5;
						ImVec2 popupSize = { 200, 200 }; // Desired size of the popup

						// Get the view port's boundaries
						const ImGuiViewport* viewport = ImGui::GetWindowViewport();
						ImVec2 viewportMin = viewport->Pos;
						ImVec2 viewportMax = { viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y };

						// Adjust the position to ensure the popup doesn't go outside the viewport
						popupPos.x = std::clamp(popupPos.x, viewportMin.x, viewportMax.x - popupSize.x);
						popupPos.y = std::clamp(popupPos.y, viewportMin.y, viewportMax.y - popupSize.y);

						// Set the position and size of the popup
						ImGui::SetNextWindowPos(popupPos);
						ImGui::SetNextWindowSize(popupSize, ImGuiCond_Once);
					}

					// Begin the popup window
					if (ImGui::Begin("Components", &showAddComponent, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
					{
						if (!ImGui::IsWindowFocused()) showAddComponent = false;

						if (!selected_entity.has<TransformComponent>())
						{
							if (ImGui::Button("Transform Component"))
								selected_entity.add<TransformComponent>();
						}

						if (!selected_entity.has<SpriteRendererComponent>() &&
							!selected_entity.has<CircleRendererComponent>() &&
							!selected_entity.has<TextRendererComponent>())
						if (ImGui::CollapsingHeader("Render"))
						{
							if (ImGui::Button("Sprite Renderer Component")) selected_entity.add<SpriteRendererComponent>();
							if (ImGui::Button("Circle Renderer Component")) selected_entity.add<CircleRendererComponent>();
							if (ImGui::Button("Text Renderer Component")) selected_entity.add<TextRendererComponent>();
						}

						if (ImGui::CollapsingHeader("Rigid Body Component##header"))
						{
							if (!selected_entity.has<RigidBodyComponent>())
							{
								if (ImGui::Button("Rigid Body Component"))
									selected_entity.add<RigidBodyComponent>();
							}

							if (!selected_entity.has<BoxCollider2DComponent>() &&
								!selected_entity.has<CircleCollider2DComponent>())
							{
								if (ImGui::Button("Box Collider Component")) selected_entity.add<BoxCollider2DComponent>();
								if (ImGui::Button("Circle Collider Component")) selected_entity.add<CircleCollider2DComponent>();
							}

						}

					}
					ImGui::End();
				}

			}

			ImGui::End();
		}

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
			}
			ImGui::EndChild();

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
							scene.Load("testScene.scene");
						}

						if (ImGui::MenuItem("Save"))
						{
							scene.Save("testScene.scene");
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
						ImGui::MenuItem("Scene properties", NULL, &showSceneProperties);
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
					if (ImGui::ImageButton("collapse", t_Collapse, {buttonSize, buttonSize}))
					{
						//Globals.window.Collapse();
					}

					ImGui::SameLine();
					if (ImGui::ImageButton("minimize", t_Minimize, { buttonSize, buttonSize }))
					{
						//Globals.window.Maximize();
					}

					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.25f, 0.2f, 1.f));
					ImGui::SameLine();
					if (ImGui::ImageButton("close", t_Close, { buttonSize, buttonSize }))
					{
						Globals.window.Close();
					}

					ImGui::PopStyleVar();
					ImGui::PopStyleColor(4);

					ImGui::EndMenuBar();
				}
			}

			if (showEditor) UI_Editor();
			if (showSceneProperties) UI_SceneProperties();
			if (showEntities) UI_Entities();
			if (showProperties) UI_Properties();
			if (showConsole) UI_Console();
			if (showFileExplorer) UI_FileExplorer();

			ImGui::End();
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