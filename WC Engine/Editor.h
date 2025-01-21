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
	glm::vec4 decompress(uint32_t num)
	{ // Remember! Convert from 0-255 to 0-1!
		glm::vec4 Output;
		Output.r = float((num & uint32_t(0x000000ff)));
		Output.g = float((num & uint32_t(0x0000ff00)) >> 8);
		Output.b = float((num & uint32_t(0x00ff0000)) >> 16);
		Output.a = float((num & uint32_t(0xff000000)) >> 24);
		return Output / 255.f;
	}

	void DrawTransformFcn(b2Transform transform, void* context)
	{
		auto t = glm::translate(glm::mat4(1.f), glm::vec3(transform.p.x, transform.p.y, 0.f)) * glm::rotate(glm::mat4(1.f), glm::radians(360.f) - b2Rot_GetAngle(transform.q), { 0.f, 0.f, 1.f });
	    static_cast<RenderData*>(context)->DrawLineQuad(t);
	}

	void DrawCircleFcn(b2Vec2 center, float radius, b2HexColor color, void* context)
	{
		static_cast<RenderData*>(context)->DrawCircle({ center.x, center.y, 0.f }, radius, 0.2f, 0.05f, decompress((int)color));
	}

	void DrawSolidCircleFcn(b2Transform transform, float radius, b2HexColor color, void* context)
	{
		static_cast<RenderData*>(context)->DrawCircle({ transform.p.x, transform.p.y, 0.f }, radius, 1.f, 0.05f, decompress((int)color));
	}

	void DrawSegmentFcn(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
	{
		static_cast<RenderData*>(context)->DrawLine({ p1.x, p1.y }, { p2.x, p2.y }, decompress((int)color));
	}

	void DrawPolygonFcn(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
	{
		//static_cast<Draw*>(context)->DrawPolygon(vertices, vertexCount, color);

		for (int i = 0; i < vertexCount; i++)
			static_cast<RenderData*>(context)->DrawLine({ vertices[i].x, vertices[i].y }, { vertices[(i + 1) % vertexCount].x, vertices[(i + 1) % vertexCount].y }, decompress((int)color));
	}

	void DrawSolidPolygonFcn(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context)
    {
		auto t = glm::translate(glm::mat4(1.f), glm::vec3(transform.p.x, transform.p.y, 0.f)) * glm::rotate(glm::mat4(1.f), glm::radians(360.f) - b2Rot_GetAngle(transform.q), { 0.f, 0.f, 1.f });

		for (int i = 0; i < vertexCount; i++)
		{
			auto v0 = glm::vec4(vertices[i].x, vertices[i].y, 0.f, 1.f) * t;
			auto v1 = glm::vec4(vertices[(i + 1) % vertexCount].x, vertices[(i + 1) % vertexCount].y, 0.f, 1.f) * t;
			static_cast<RenderData*>(context)->DrawLine(glm::vec2(v0), glm::vec2(v1), decompress((int)color));
		}

		for (int i = 0; i < vertexCount; i += 3)
		{
			auto v2 = glm::vec4(vertices[i + 2].x, vertices[i + 2].y, 0.f, 1.f) * t;

			//static_cast<RenderData*>(context)->DrawTriangle({ v2.x, v2.y }, { v0.x, v0.y }, { v1.x, v1.y }, 0, decompress((int)color));
		}
	}

	void DrawSolidCapsuleFcn(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
	{
		//static_cast<Draw*>(context)->DrawSolidCapsule(p1, p2, radius, color);
	}

	void DrawPointFcn(b2Vec2 p, float size, b2HexColor color, void* context)
	{
		//static_cast<Draw*>(context)->DrawPoint(p, size, color);
		static_cast<RenderData*>(context)->DrawCircle({ p.x, p.y, 0.f }, size, 1.f, 0.05f, decompress((int)color));
	}

	void DrawStringFcn(b2Vec2 p, const char* s, b2HexColor color, void* context)
	{
		//static_cast<Draw*>(context)->DrawString(p, s);
	}

	struct Editor
	{
	private:
		// Panning variables
		glm::vec2 m_StartPan;
		glm::vec2 m_BeginCameraPosition;
		bool m_Panning = false;

	    // Debug stats
	    float m_DebugTimer = 0.f;

	    uint32_t m_PrevMaxFPS = 0;
	    uint32_t m_MaxFPS = 0;

	    uint32_t m_PrevMinFPS = 0;
	    uint32_t m_MinFPS = 0;

	    uint32_t m_FrameCount = 0;
	    uint32_t m_FrameCounter = 0;
	    uint32_t m_PrevFrameCounter = 0;

		OrthographicCamera camera;

	    RenderData m_RenderData[FRAME_OVERLAP];
		Renderer2D m_Renderer;

		Texture t_Close;
		Texture t_Minimize;
		Texture t_Collapse;
		Texture t_FolderOpen;
		Texture t_FolderClosed;
		Texture t_File;
		Texture t_FolderEmpty;
		Texture t_Folder;

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
		bool showAssets = true;
	    bool showDebugStats = true;

		flecs::entity m_SelectedEntity = flecs::entity::null();

		enum class SceneState { Edit, Simulate, Play };
		SceneState m_SceneState = SceneState::Edit;

		Scene m_Scene;
		Scene m_TempScene;

		// Settings
		float ZoomSpeed = 2.f;
	    b2DebugDraw m_PhysicsDebugDraw;

	public:
	    AssetManager assetManager;

	    void Create()
	    {
	        assetManager.Init();

	        for (int i = 0; i < FRAME_OVERLAP; i++)
	            m_RenderData[i].Allocate();

	        m_Renderer.camera = &camera;
	        m_Renderer.Init(assetManager);

	        // Load Textures
	        t_Close.Load("assets/textures/menu/close.png");
	        t_Minimize.Load("assets/textures/menu/minimize.png");
	        t_Collapse.Load("assets/textures/menu/collapse.png");
	        t_FolderOpen.Load("assets/textures/menu/folder-open.png");
	        t_FolderClosed.Load("assets/textures/menu/folder-closed.png");
	        t_File.Load("assets/textures/menu/file.png");
	        t_FolderEmpty.Load("assets/textures/menu/folder-empty.png");
	        t_Folder.Load("assets/textures/menu/folder-fill.png");

	        b2AABB bounds = { { -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX } };
	        m_PhysicsDebugDraw = {
	            DrawPolygonFcn,
                DrawSolidPolygonFcn,
                DrawCircleFcn,
                DrawSolidCircleFcn,
                DrawSolidCapsuleFcn,
                DrawSegmentFcn,
                DrawTransformFcn,
                DrawPointFcn,
                DrawStringFcn,
                bounds,
                true, // drawUsingBounds
                true, // shapes
                true, // joints
                true, // joint extras
                true, // aabbs
                true, // mass
                true, // contacts
                true, // colors
                true, // normals
                true, // impulse
                true, // friction
                &m_RenderData };
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
		    auto& renderData = m_RenderData[CURRENT_FRAME];
		    if (entt.has<SpriteRendererComponent>())
		    {
		        auto& data = *entt.get<SpriteRendererComponent>();

		        renderData.DrawQuad(transform, data.Texture, data.Color);
		    }
		    else if (entt.has<CircleRendererComponent>())
		    {
		        auto& data = *entt.get<CircleRendererComponent>();
		        renderData.DrawCircle(transform, data.Thickness, data.Fade, data.Color);
		    }
		    else if (entt.has<TextRendererComponent>())
		    {
		        auto& data = *entt.get<TextRendererComponent>();

		        //renderData.DrawString(data.Text, data.Font, transform, data.Color);
		    }
		    //m_Scene.GetWorld().query_builder<TransformComponent, EntityTag>()
		    //	.with(flecs::ChildOf, entt)
		    //	.each([&](flecs::entity child, TransformComponent childTransform, EntityTag)
		    //		{
		    //			transform = transform * childTransform.GetTransform();
		    //			RenderEntity(child, transform);
		    //		});
		}

	    void Render()
		{
		    auto& renderData = m_RenderData[CURRENT_FRAME];

		    m_Scene.GetWorld().each([&](flecs::entity entt, TransformComponent& p) {
                //if (entt.parent() != 0) return;

                glm::mat4 transform = p.GetTransform();
                RenderEntity(entt, transform);
            });

		    if (m_SceneState != SceneState::Edit)
		    {
		        m_PhysicsDebugDraw.context = &renderData;
		        m_Scene.GetPhysicsWorld().Draw(&m_PhysicsDebugDraw);
		    }

		    m_Renderer.Flush(renderData);

		    renderData.Reset();
		}

		void Update()
		{
			if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate)
				m_Scene.UpdatePhysics();

			Render();
		}

		void ChangeSceneState(SceneState newState)
		{
			std::string selectedEntityName;
			if (m_SelectedEntity != flecs::entity::null()) selectedEntityName = m_SelectedEntity.name().c_str();
			//WC_INFO("Changing scene state. Selected Entity: {0}", selectedEntityName);

			m_SceneState = newState;

			if (newState == SceneState::Play || newState == SceneState::Simulate)
			{
				m_Scene.CreatePhysicsWorld();
				m_TempScene.Copy(m_Scene);
			}
			else if (newState == SceneState::Edit)
			{
				m_Scene.DestroyPhysicsWorld();
				m_Scene.Copy(m_TempScene);
			}

			if(m_SelectedEntity != flecs::entity::null()) m_SelectedEntity = m_Scene.GetWorld().lookup(selectedEntityName.c_str());
			//WC_INFO("Scene state changed. Selected Entity: {0}", m_SelectedEntity.name().c_str());
		}

		void UI_Editor()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
			if (ImGui::Begin("Editor", &showEditor))
			{
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
				glm::vec2 RenderSize = *((glm::vec2*)&viewportBounds[1]) - WindowPos;

				auto image = m_Renderer.GetImguiImageID();
				ImGui::GetWindowDrawList()->AddImage(image, ImVec2(WindowPos.x, WindowPos.y), ImVec2(WindowPos.x + RenderSize.x, WindowPos.y + RenderSize.y));

				float buttonsWidth = ImGui::CalcTextSize("Play").x + ImGui::CalcTextSize("Stop").x + ImGui::CalcTextSize("Simulate").x + ImGui::GetStyle().FramePadding.x * 6.0f;
				float totalWidth = buttonsWidth + ImGui::GetStyle().ItemSpacing.x * 2;
				ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);
				bool isPlayingOrSimulating = (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);
				bool isPaused = (m_SceneState == SceneState::Edit);

				if (isPlayingOrSimulating)
					ImGui::BeginDisabled();
				if (ImGui::Button("Play") && isPaused) ChangeSceneState(SceneState::Play);
				ImGui::SameLine();
				if (ImGui::Button("Simulate") && isPaused) ChangeSceneState(SceneState::Simulate);
				if (isPlayingOrSimulating)
					ImGui::EndDisabled();
				ImGui::SameLine();
				if (isPaused)
					ImGui::BeginDisabled();
				if (ImGui::Button("Stop")) ChangeSceneState(SceneState::Edit); 
				if (isPaused)
					ImGui::EndDisabled();

				glm::mat4 projection = camera.GetProjectionMatrix();
				projection[1][1] *= -1;

				ImGuizmo::SetOrthographic(true);
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(WindowPos.x, WindowPos.y, RenderSize.x, RenderSize.y);

				if (m_SelectedEntity != flecs::entity::null() && m_SelectedEntity.has<TransformComponent>())
				{
					glm::mat4 local_transform = m_SelectedEntity.get<TransformComponent>()->GetTransform();
					glm::mat4 world_transform = local_transform;
					glm::mat4 deltaMatrix;

					// Build the world transform by accumulating parent transforms
					auto parent = m_SelectedEntity.parent();
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
						ImGuizmo::MODE::WORLD,
						glm::value_ptr(world_transform),
						glm::value_ptr(deltaMatrix)
					);

					if (ImGuizmo::IsUsing())
					{
						// Convert world transform back to local space
						glm::mat4 parent_world_transform(1.f);
						parent = m_SelectedEntity.parent();
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
						m_SelectedEntity.set<TransformComponent>({
							glm::vec2(translation),
							glm::vec2(scale),
							rotation.z
							});

						if (m_SelectedEntity.has<RigidBodyComponent>())
						{
							auto body = m_SelectedEntity.get_ref<RigidBodyComponent>()->body;
							if (body.IsValid())
							{
								body.SetLinearVelocity({ 0.f, 0.f });
								body.SetAngularVelocity(0.f);
								body.SetTransform(translation, b2MakeRot(rotation.z));
							}
						}
					}
				}
			}
			ImGui::PopStyleVar();
			ImGui::End();
		}

		void UI_SceneProperties()
		{
			if (ImGui::Begin("Scene Properties", &showSceneProperties))
			{
				auto& worldData = m_Scene.GetPhysicsWorldData();
				UI::DragButton2("Gravity", worldData.Gravity);

				//tests

				//if (m_SelectedEntity != flecs::entity::null())
				//{
				//	//WC_INFO(m_SelectedEntity.has<ChildNamesComponent>());
				//	auto names = m_SelectedEntity.get_ref<ChildNamesComponent>();
				//	if (names)
				//	{
				//		for (const auto& name : names->childNames)
				//		{
				//			ImGui::Button(name.c_str());
				//		}
				//	}
				//}

				//for (const auto& name : m_Scene.GetParentEntityNames())
				//{
				//	auto entity = m_Scene.GetWorld().lookup(name.c_str());
				//	if (entity)
				//	{
				//		if (ImGui::Button(name.c_str()))
				//		{
				//			m_SelectedEntity = entity;
				//		}
				//	}
				//}




				//}
				//{
				//	auto rt = physicsWorld.GetRestitutionThreshold();
				//	UI::Drag("Restitution Threshold", rt);
				//	physicsWorld.SetRestitutionThreshold(rt);
				//}
				//{
				//	auto ht = physicsWorld.GetHitEventThreshold();
				//	UI::Drag("Hit Event Threshold", ht);
				//	physicsWorld.SetRestitutionThreshold(ht);
				//}
				//{
				//	auto ht = physicsWorld.GetContactHertz();
				//	UI::DragButton2("Hit Event Threshold", ht);
				//	physicsWorld.SetRestitutionThreshold(rt);
				//}
				//{
				//	auto ht = physicsWorld.GetContactDampingRatio();
				//	UI::DragButton2("Hit Event Threshold", ht);
				//	physicsWorld.SetRestitutionThreshold(rt);
				//}
				//{
				//	auto ht = physicsWorld.GetContactPushMaxSpeed();
				//	UI::DragButton2("Hit Event Threshold", ht);
				//	physicsWorld.SetRestitutionThreshold(rt);
				//}
			}
			ImGui::End();
		}

		void ShowEntities()
		{
			// Reset selection if the mouse is clicked outside the tree nodes
			if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsWindowFocused())
			{
				m_SelectedEntity = flecs::entity::null();
			}

			static bool dragMode = false;
			if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl, ImGuiInputFlags_LockThisFrame))
			{
				dragMode = !dragMode;
				WC_INFO(dragMode ? "Changed Mode to : Bond" : "Changed Mode to : Reorder");
			}

			// Recursive function to display entities
			auto displayEntity = [&](flecs::entity entity, auto& displayEntityRef) -> void {
				const bool is_selected = (m_SelectedEntity == entity);

				// Get children of the current entity from ChildNamesComponent
				std::vector<flecs::entity> children;
				if (entity.has<ChildNamesComponent>())
				{
					auto& childNames = entity.get<ChildNamesComponent>()->childNames;
					for (const auto& childName : childNames)
					{
						std::string fullChildName = std::string(entity.name()) + "::" + childName;

						// Now ensure the lookup takes into account the parent and grandparent names
						flecs::entity childEntity = entity; // Start with current entity (parent)
						while (childEntity.parent() != flecs::entity::null()) {
							childEntity = childEntity.parent();  // Traverse upwards to ensure the full entity path
							fullChildName = std::string(childEntity.name()) + "::" + fullChildName;  // Add parent names to the lookup path
						}

						// Lookup the child entity using the constructed full path
						auto childEntityResolved = m_Scene.GetWorld().lookup(fullChildName.c_str());
						if (childEntityResolved)
						{
							children.push_back(childEntityResolved);
						}
					}
				}

				// Set up the tree node flags
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (is_selected)
					node_flags |= ImGuiTreeNodeFlags_Selected;

				if (children.empty())
					node_flags |= ImGuiTreeNodeFlags_Leaf;

				if (m_SelectedEntity != flecs::entity::null()) {
					auto parent = m_SelectedEntity.parent();
					while (parent != flecs::entity::null()) {
						if (parent == entity) {
							ImGui::SetNextItemOpen(true);
							break;
						}
						parent = parent.parent();
					}
				}

				// Push a flag to allow duplicate IDs in the same window
				ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

				// Render the entity
				bool is_open = ImGui::TreeNodeEx(entity.name().c_str(), node_flags);

				ImGui::PopItemFlag();

				// Handle selection on click
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				{
					m_SelectedEntity = entity;
				}

				if(dragMode)
				{
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						WC_INFO("Dragging");
						ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(flecs::entity));
						ImGui::Text("Dragging %s", entity.name().c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
						{
							IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
							flecs::entity droppedEntity = *(const flecs::entity*)payload->Data;

							// Check if the drop target is empty space
							 if (droppedEntity != entity.parent())
							{
								m_Scene.SetChild(entity, droppedEntity);
							}
						}
						ImGui::EndDragDropTarget();
					}


				}
				else if(ImGui::IsItemActive() && !ImGui::IsItemHovered())
				{
					auto& entitiyNames = entity.parent() == flecs::entity::null() ? m_Scene.GetParentEntityNames() : entity.parent().get_ref<ChildNamesComponent>()->childNames;
					auto it = std::find(entitiyNames.begin(), entitiyNames.end(), std::string(entity.name()));

					if (it != entitiyNames.end())
					{
						int index = std::distance(entitiyNames.begin(), it);

						// Get the drag delta in Y-axis
						float drag_delta_y = ImGui::GetMouseDragDelta(0).y;

						// Only consider a drag if the delta is significant
						if (std::abs(drag_delta_y) > 5.0f)  // A threshold to ignore small drags
						{
							// Calculate the target index based on drag direction
							int n_next = index + (drag_delta_y < 0.f ? -1 : 1);

							// Ensure the index stays within bounds and we don't swap to the same position
							if (n_next >= 0 && n_next < entitiyNames.size() && n_next != index)
							{
								std::swap(entitiyNames[index], entitiyNames[n_next]);

								// Reset the mouse drag delta after a swap
								ImGui::ResetMouseDragDelta();
							}
						}
					}
				}

				// Handle right-click and popup menu
				if (ImGui::IsWindowHovered())
				{
					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						ImGui::OpenPopup(std::to_string(entity.id()).c_str());
					}
					else if (m_SelectedEntity != flecs::entity::null() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						ImGui::OpenPopup(std::to_string(m_SelectedEntity.id()).c_str());
					}
				}

				// Display the popup menu
				if (ImGui::BeginPopup(std::to_string(entity.id()).c_str(), ImGuiWindowFlags_NoFocusOnAppearing))
				{
					ImGui::Text("%s", entity.name().c_str());
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

					if (entity.parent() != flecs::entity::null() && ImGui::MenuItem("Remove Child"))
					{
						auto parent = entity.parent();
						m_Scene.RemoveChild(parent, entity);
					}

					if (m_SelectedEntity != flecs::entity::null() && entity != m_SelectedEntity)
					{
						if (ImGui::MenuItem("Set Child of Selected"))
						{
							m_Scene.SetChild(m_SelectedEntity, entity);
						}
					}

					ImGui::Separator();

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.92, 0.25f, 0.2f, 1.f));
					if (ImGui::MenuItem("Delete"))
					{
						m_Scene.KillEntity(entity);
						m_SelectedEntity = flecs::entity::null();
						ImGui::CloseCurrentPopup();
					}
					ImGui::PopStyleColor();

					ImGui::EndPopup();
				}

				// If the node is open, recursively display children
				if (is_open)
				{
					for (const auto& child : children)
					{
						displayEntityRef(child, displayEntityRef);
					}

					ImGui::TreePop(); // Ensure proper pairing of TreeNodeEx and TreePop
				}
				};

			// Display root entities
			for (const auto& name : m_Scene.GetParentEntityNames())
			{
				auto rootEntity = m_Scene.GetWorld().lookup(name.c_str());
				if (rootEntity)
				{
					displayEntity(rootEntity, displayEntity);
				}
			}
		}

		void UI_Entities()
		{
			if (ImGui::Begin("Entities", &showEntities, ImGuiWindowFlags_MenuBar))
			{
				ShowEntities();

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
				ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 40, ImGui::GetWindowSize().y - 40));
				if (ImGui::Button("+", { 30, 30 }))
				{
					ImGui::OpenPopup("Add Entity");
				}
				ImGui::SetItemTooltip("Add Entity");
				ImGui::PopStyleVar();

				if (ImGui::BeginPopupModal("Add Entity", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					ImVec2 center = ImGui::GetMainViewport()->GetCenter();
					ImVec2 windowSize = ImGui::GetWindowSize();
					ImVec2 windowPos = ImVec2(center.x - windowSize.x * 0.5f, center.y - windowSize.y * 0.5f);
					ImGui::SetWindowPos(windowPos, ImGuiCond_Once);

					static std::string name = "Entity " + std::to_string(m_Scene.GetWorld().count<EntityTag>());
					ImGui::InputText("Name", &name);

					if (ImGui::Button("Create") || ImGui::IsKeyPressed(ImGuiKey_Enter))
					{
						if (!name.empty()) 
						{
							m_SelectedEntity = m_Scene.AddEntity(name);
							name = "Entity " + std::to_string(m_Scene.GetWorld().count<EntityTag>());
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

					if (ImGui::BeginPopupModal("WarnEmptyName", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |		ImGuiWindowFlags_NoMove))
					{
						ImGui::Text("Name cannot be empty!");

						if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();

						ImGui::EndPopup();
					}

					ImGui::EndPopup();
				}

			}
			ImGui::End();
		}

		void UI_Properties()
		{
			if (ImGui::Begin("Properties", &showProperties))
			{
				if (m_SelectedEntity != flecs::entity::null())
				{
					std::string nameBuffer = m_SelectedEntity.name().c_str(); // Buffer to hold the entity's name

					// Input name
					if (ImGui::InputText("Name", &nameBuffer)) m_SelectedEntity.set_name(nameBuffer.c_str());

					// Display the entity's ID
					//ImGui::Text("ID: %u", selected_entity.id());

					//NOTE: for every new component, a new if statement is needed
					ImGui::SeparatorText("Components");

					if (m_SelectedEntity.has<TransformComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Transform##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto& p = *m_SelectedEntity.get<TransformComponent>();
							auto& position = const_cast<glm::vec2&>(p.Translation);
							auto& scale = const_cast<glm::vec2&>(p.Scale);
						    auto& realRotation = const_cast<float&>(p.Rotation);
						    auto rotation = glm::degrees(p.Rotation);

							// Draw position UI
							UI::DragButton2("Position", position);
							UI::DragButton2("Scale", scale);
						    UI::Drag("Rotation", rotation, 0.5f, 0.f, 360.f);
						    realRotation = glm::radians(rotation);
						}

						if (!visible) m_SelectedEntity.remove<TransformComponent>(); // add modal popup
					}

					if (m_SelectedEntity.has<SpriteRendererComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Sprite Renderer##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto& s = *m_SelectedEntity.get<SpriteRendererComponent>();
							auto& color = const_cast<glm::vec4&>(s.Color);
							ImGui::ColorEdit4("color", glm::value_ptr(color));
						}

						if (!visible) m_SelectedEntity.remove<SpriteRendererComponent>();
					}

					if (m_SelectedEntity.has<CircleRendererComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Circle Renderer##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto& c = *m_SelectedEntity.get<CircleRendererComponent>();
							auto& thickness = const_cast<float&>(c.Thickness);
							auto& fade = const_cast<float&>(c.Fade);
							auto& color = const_cast<glm::vec4&>(c.Color);

							// Draw circle renderer UI
							ImGui::SliderFloat("Thickness", &thickness, 0.0f, 1.0f);
							ImGui::SliderFloat("Fade", &fade, 0.0f, 1.0f);
							ImGui::ColorEdit4("Color", glm::value_ptr(color));
							ImGui::Separator();
						}

						if (!visible) m_SelectedEntity.remove<CircleRendererComponent>(); // add modal popup
					}

					if (m_SelectedEntity.has<TextRendererComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Text Renderer##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto& t = *m_SelectedEntity.get<TextRendererComponent>();
							auto& text = const_cast<std::string&>(t.Text);
							//auto& font = const_cast<std::string&>(t.Font);
							auto& color = const_cast<glm::vec4&>(t.Color);
							// Draw text renderer UI
							ImGui::InputText("Text", &text);
							//ImGui::InputText("Font", &font);
							ImGui::ColorEdit4("Color", glm::value_ptr(color));
							ImGui::Separator();
						}
						if (!visible) m_SelectedEntity.remove<TextRendererComponent>(); // add modal popup
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
							UI::Checkbox("Sensor", material.Sensor);
							UI::Checkbox("Enable Sensor Events", material.EnableSensorEvents);
							UI::Checkbox("Enable Contact Events", material.EnableContactEvents);
							UI::Checkbox("Enable Hit Events", material.EnableHitEvents);
							UI::Checkbox("Enable Pre-Solve Events", material.EnablePreSolveEvents);
							UI::Checkbox("Invoke Contact Creation", material.InvokeContactCreation);
							UI::Checkbox("Update Body Mass", material.UpdateBodyMass);
						};

					if (m_SelectedEntity.has<RigidBodyComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Rigid Body##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto p = m_SelectedEntity.get_ref<RigidBodyComponent>();

							const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
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

						if (!visible) m_SelectedEntity.remove<RigidBodyComponent>(); // add modal popup
					}

					if (m_SelectedEntity.has<BoxCollider2DComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Box Collider##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto p = m_SelectedEntity.get_ref<BoxCollider2DComponent>();
							auto& offset = const_cast<glm::vec2&>(p->Offset);
							auto& size = const_cast<glm::vec2&>(p->Size);

							UI::DragButton2("Offset", offset);
							UI::DragButton2("Size", size);

							UI_PhysicsMaterial(p->Material);

							ImGui::Separator();

						}

						if (!visible) m_SelectedEntity.remove<BoxCollider2DComponent>(); // add modal popup
					}

					if (m_SelectedEntity.has<CircleCollider2DComponent>())
					{
						bool visible = true;
						if (ImGui::CollapsingHeader("Circle Collider##header", m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
						{
							auto p = m_SelectedEntity.get_ref<CircleCollider2DComponent>();
							auto& offset = const_cast<glm::vec2&>(p->Offset);
							auto& radius = const_cast<float&>(p->Radius);

							UI::DragButton2("Offset", offset);
							UI::Drag("Radius", radius);

							UI_PhysicsMaterial(p->Material);

							ImGui::Separator();

						}

						if (!visible) m_SelectedEntity.remove<CircleCollider2DComponent>(); // add modal popup
					}

					static bool showAddComponent = false;
					static enum { None, Transform, Render, Rigid } menu = None;
					if (ImGui::Button("Add Component"))
					{
						showAddComponent = true;
						menu = None;
					}

					auto ItemAutoClose = [](const char* label, bool disabled) -> bool
						{
							if (ImGui::MenuItem(label, nullptr, nullptr, !disabled))
							{
								showAddComponent = false;
								return true;
							}
							if (disabled)
							{
								ImGui::SetItemTooltip("You can't have more than one of this component!");
							}

							return false;
						};

					if (showAddComponent)
					{
						// Calculate clamp
						{
							// Calculate the desired position for the popup
							ImVec2 popupPos = ImGui::GetItemRectMin();
							popupPos.y = ImGui::GetItemRectMax().y + 5;
							ImVec2 popupSize = { 200, 150 }; // Desired size of the popup

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
						if (ImGui::Begin("Add##Component", &showAddComponent, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar))
						{
							if (!ImGui::IsWindowFocused()) showAddComponent = false;

							//Replace with an arrow button
							if (menu != None && ImGui::Button("<"))
							{
								menu = None;
							}
							ImGui::SameLine();
							switch (menu)
							{
							case None:
								ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Components").x) * 0.5f);
								ImGui::Text("Components");
								break;
							case Transform:
								ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Transform").x) * 0.5f);
								ImGui::Text("Transform");
								break;
							case Render:
								ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Render").x) * 0.5f);
								ImGui::Text("Render");
								break;
							case Rigid:
								ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Rigid Body").x) * 0.5f);
								ImGui::Text("Rigid Body");
								break;
							}

							ImGui::Separator();

							switch (menu)
							{
							case None:
							{
								if (ImGui::MenuItem("Transform")) { menu = Transform; } UI::RenderArrowIcon();
								if (ImGui::MenuItem("Render")) { menu = Render; } UI::RenderArrowIcon();
								if (ImGui::MenuItem("Rigid Body")) { menu = Rigid; } UI::RenderArrowIcon();
								break;
							}
							case Transform:
							{
								if (ItemAutoClose("Transform", m_SelectedEntity.has<TransformComponent>()))m_SelectedEntity.add<TransformComponent>();
								break;
							}
							case Render:
							{
								bool hasRender = m_SelectedEntity.has<SpriteRendererComponent>() || m_SelectedEntity.has<CircleRendererComponent>() || m_SelectedEntity.has<TextRendererComponent>();
								if (ItemAutoClose("Sprite Renderer Component", hasRender)) m_SelectedEntity.add<SpriteRendererComponent>();
								if (ItemAutoClose("Circle Renderer Component", hasRender)) m_SelectedEntity.add<CircleRendererComponent>();
								if (ItemAutoClose("Text Renderer Component", hasRender))	m_SelectedEntity.add<TextRendererComponent>();
								break;
							}
							case Rigid:
							{
								if (ItemAutoClose("Rigid Body Component", m_SelectedEntity.has<RigidBodyComponent>()))		m_SelectedEntity.add<RigidBodyComponent>();
								bool hasCollider = m_SelectedEntity.has<BoxCollider2DComponent>() || m_SelectedEntity.has<CircleCollider2DComponent>();
								if (ItemAutoClose("Box Collider Component", hasCollider))	m_SelectedEntity.add<BoxCollider2DComponent>();
								if (ItemAutoClose("Circle Collider Component", hasCollider))	m_SelectedEntity.add<CircleCollider2DComponent>();
								break;
							}
							}

						}
						ImGui::End();
					}

				}

			}
			ImGui::End();
		}

		void UI_Console()
		{
			if (ImGui::Begin("Console", &showConsole))
			{
				if (ImGui::Button("Clear"))
				{
					//Globals.console.Clear();
					Log::GetConsoleSink()->messages.clear();
					
				}
				
				ImGui::SameLine();
				if (ImGui::Button("Copy"))
				{
					std::string logData;
					for (const auto& msg : Log::GetConsoleSink()->messages)
					{
						logData += msg.payload + "\n"; // Assuming msg.payload is a string
					}
					ImGui::SetClipboardText(logData.c_str());
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

			}
			ImGui::End();
		}

		void UI_Assets()
		{
			static const std::filesystem::path assetsPath = std::filesystem::current_path() / "assets";
			static std::unordered_map<std::string, bool> folderStates;  // Track the expansion state per folder
			static std::filesystem::path selectedFolderPath = assetsPath;
			static std::vector<std::filesystem::path> openedFiles;
			static std::unordered_set<std::string> openedFileNames;
			static bool showIcons;
			static bool previewAsset;

			// Expand all helper func
			std::function<void(const std::filesystem::path&, bool)> setFolderStatesRecursively =
				[&](const std::filesystem::path& path, bool state) {
				for (const auto& entry : std::filesystem::directory_iterator(path))
				{
					if (entry.is_directory())
					{
						folderStates[entry.path().string()] = state;
						setFolderStatesRecursively(entry.path(), state);
					}
				}
				};

			if (ImGui::Begin("Assets", &showAssets, ImGuiWindowFlags_MenuBar))
			{
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::MenuItem("Import"))
					{
						WC_INFO("TODO - Implement Import");
					}

					if (ImGui::MenuItem("Copy Path"))
					{
						ImGui::SetClipboardText(assetsPath.string().c_str());
						WC_INFO("Copied path to clipboard > {}", assetsPath.string());
					}

					if (ImGui::BeginMenu("View##Assets"))
					{
						if (ImGui::MenuItem("Show Icons", nullptr, &showIcons))
						{
							WC_INFO("Show Icons: {}", showIcons);
						}

						if (ImGui::MenuItem("Preview Assets", nullptr, &previewAsset))
						{
							WC_INFO("Preview Asset: {}", previewAsset);
						}

						if (ImGui::MenuItem("Collapse All"))
						{
							for (auto& [key, value] : folderStates)
							{
								value = false;
							}
						}

						if (ImGui::MenuItem("Expand All"))
						{
							setFolderStatesRecursively(assetsPath, true);
						}

						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}

				std::function<void(const std::filesystem::path&)> displayDirectory;

				displayDirectory = [&](const std::filesystem::path& path)
					{
						if (path != assetsPath) ImGui::Indent(20);

						for (const auto& entry : std::filesystem::directory_iterator(path))
						{
							const auto& filenameStr = entry.path().filename().string();
							const auto& fullPathStr = entry.path().string();
							if (entry.is_directory())
							{
								// Use a selectable item for directories with a manual toggle for expanding/collapsing

								// Initialize folder state if it doesn't exist
								auto [it, inserted] = folderStates.try_emplace(fullPathStr, false);
								bool& isOpen = it->second;

								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
								ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
								if (ImGui::ImageButton((filenameStr + "##b" + fullPathStr).c_str(), isOpen ? t_FolderOpen : t_FolderClosed, ImVec2(16, 16)))
								{
									isOpen = !isOpen; // Toggle state
								}
								ImGui::PopStyleColor();
								ImGui::PopStyleVar();
								ImGui::SameLine();
								// Handle the selectable for toggling folder expansion
								if (ImGui::Selectable((filenameStr + "##" + fullPathStr).c_str(), selectedFolderPath == entry.path() && showIcons, ImGuiSelectableFlags_DontClosePopups))
								{
									selectedFolderPath = entry.path();
								}
								if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
								{
									isOpen = !isOpen; // Toggle state
								}

								// If the folder is open, recursively display its contents
								if (isOpen)
								{
									displayDirectory(entry.path());  // Recursively show folder contents
								}
							}
							else
							{
								// Handle files (no expansion, just display)
								ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
								ImGui::TreeNodeEx((filenameStr + "##" + fullPathStr).c_str(), leafFlags);
								if (ImGui::IsItemHovered())
								{
									if (previewAsset) ImGui::OpenPopup(("PreviewAsset##" + fullPathStr).c_str());

									if (ImGui::IsMouseDoubleClicked(0))
									{
										if (openedFileNames.insert(fullPathStr).second)
										{
											openedFiles.push_back(entry.path());
										}
									}

									if (ImGui::BeginPopup(("PreviewAsset##" + fullPathStr).c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMouseInputs))
									{
										ImGui::Text("Preview: %s", filenameStr.c_str());
										ImGui::EndPopup();
									}
								}
							}
						}

						if (path != assetsPath) ImGui::Unindent(20);
					};

				if (showIcons)
				{
					if (ImGui::BeginTable("assets_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
					{
						ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 150.0f);
						ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						// Display directories in a scrollable child window
						ImGui::BeginChild("FoldersChild##1", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
						displayDirectory(assetsPath);
						ImGui::EndChild();

						ImGui::TableSetColumnIndex(1);

						if (ImGui::GetContentRegionMax().x > 200)
						{
							if (ImGui::BeginChild("Path Viewer", ImVec2{ 0, 0 }, true)) // Enable scroll with the third argument
							{
								// Replace with an arrow button
								if (selectedFolderPath == assetsPath)
								{
									ImGui::BeginDisabled();
									ImGui::Button("<<");
									ImGui::EndDisabled();
								}
								else if (ImGui::Button("<<")) selectedFolderPath = selectedFolderPath.parent_path();

								ImGui::SameLine();
								ImGui::Text(selectedFolderPath.string().c_str());
								ImGui::Separator();

								float totalButtonWidth = 50 + ImGui::GetStyle().ItemSpacing.x;
								int itemsPerRow = static_cast<int>((ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x) / totalButtonWidth);

								if (itemsPerRow < 1) itemsPerRow = 1; // Ensure at least 1 button per row

								if (std::filesystem::exists(selectedFolderPath))
								{
									int i = 0;
									for (const auto& entry : std::filesystem::directory_iterator(selectedFolderPath))
									{
										if (i > 0 && i % itemsPerRow != 0)
										{
											ImGui::SameLine();
										}

										if (entry.is_directory())
										{
											ImGui::BeginGroup();
											ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
											ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
											if (ImGui::ImageButton((entry.path().string() + "/").c_str(), std::filesystem::is_empty(entry.path()) ? t_FolderEmpty : t_Folder, { 50, 50 }))
											{
												selectedFolderPath = entry.path();
											}
											ImGui::PopStyleColor();
											ImGui::PopStyleVar();

											std::string filename = entry.path().filename().string();
											float wrapWidth = 50.0f; // Width for wrapping the text

											// Split the text into words
											std::istringstream stream(filename);
											std::vector<std::string> words{ std::istream_iterator<std::string>{stream}, std::istream_iterator<std::string>{} };

											// Display each line centered
											std::string currentLine;
											float currentLineWidth = 0.0f;
											ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 }); // Reduce item spacing for text
											for (const auto& word : words)
											{
												ImVec2 wordSize = ImGui::CalcTextSize(word.c_str());
												if (currentLineWidth + wordSize.x > wrapWidth)
												{
													// Push the current line and start a new one
													ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
													ImGui::TextUnformatted(currentLine.c_str());
													currentLine.clear();
													currentLineWidth = 0.0f;
												}

												if (!currentLine.empty())
												{
													currentLine += " "; // Add a space before the next word
													currentLineWidth += ImGui::CalcTextSize(" ").x;
												}
												currentLine += word;
												currentLineWidth += wordSize.x;
											}

											// Add the last line if there's any remaining text
											if (!currentLine.empty())
											{
												ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
												ImGui::TextUnformatted(currentLine.c_str());
											}
											ImGui::PopStyleVar(); // Restore item spacing for text

											ImGui::EndGroup();
										}
										else
										{
											ImGui::BeginGroup();
											ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
											ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

											if (ImGui::ImageButton((entry.path().string() + "/").c_str(), t_File, { 50, 50 }))
											{
												if (openedFileNames.insert(entry.path().string()).second)
												{
													openedFiles.push_back(entry.path());
												}
											}

											ImGui::PopStyleVar();
											ImGui::PopStyleColor();
											ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 50);
											ImGui::TextWrapped(entry.path().filename().string().c_str());
											ImGui::PopTextWrapPos();
											ImGui::EndGroup();
										}
										i++;
									}
								}
							}
							ImGui::EndChild();
						}

						ImGui::EndTable();
					}
				}
				else
				{
					// Display directories without table
					ImGui::BeginChild("FoldersChild##2", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
					displayDirectory(assetsPath);
					ImGui::EndChild();
				}
			}
			ImGui::End();

			// Handle opened files (tabs or whatever content needs to be displayed)
			for (auto it = openedFiles.begin(); it != openedFiles.end();)
			{
				bool open = true;
				if (ImGui::Begin(it->filename().string().c_str(), &open))
				{
					ImGui::Text("File: %s", it->string().c_str());

				}
				ImGui::End();

				if (!open)
				{
					openedFileNames.erase(it->string());
					it = openedFiles.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

	    void UI_DebugStats()
	    {
	        if (ImGui::Begin("Debug Stats", &showDebugStats))
	        {
	            uint32_t fps = 1.f / Globals.deltaTime;
	            UI::Text(std::format("Frame time: {}ms", Globals.deltaTime * 1000.f));
	            UI::Text(std::format("FPS: {}", fps));
	            UI::Text(std::format("Max FPS: {}", m_PrevMaxFPS));
	            UI::Text(std::format("Min FPS: {}", m_PrevMinFPS));

	            UI::Text(std::format("Average FPS: {}", m_PrevFrameCounter));

	            m_DebugTimer += Globals.deltaTime;

	            m_MaxFPS = glm::max(m_MaxFPS, fps);
	            m_MinFPS = glm::min(m_MinFPS, fps);

	            m_FrameCount++;
	            m_FrameCounter += fps;

	            if (m_DebugTimer >= 1.f)
	            {
	                m_PrevMaxFPS = m_MaxFPS;
	                m_PrevMinFPS = m_MinFPS;
	                m_PrevFrameCounter = m_FrameCounter / m_FrameCount;
	                m_DebugTimer = 0.f;

	                m_FrameCount = 0;
	                m_FrameCounter = 0;

	                m_MaxFPS = m_MinFPS = fps;
	            }
	        }
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

			if (ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
				| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground))
			{
				ImGuiIO& io = ImGui::GetIO();
				if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
				{
					ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
					ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f));
				}

				ImGui::PopStyleVar(3);

				// Main Menu Bar

				if (ImGui::BeginMenuBar())
				{

					// TODO - add Dragging and Turn of GLFW tab bar -> make custom / get from The Cherno
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
							m_Scene.Load("testScene.scene");
						}

						if (ImGui::MenuItem("Save"))
						{
							m_Scene.Save("testScene.scene");
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
						ImGui::MenuItem("Assets", NULL, &showAssets);

					    if (ImGui::BeginMenu("Theme"))
					    {
					        // TODO - Add more themes / custom themes / save themes
					        static const char* themes[] = { "SoDark", "Classic", "Dark", "Light" };
					        static int currentTheme = 0; // Default to SoDark
					        static float hue = 0.0f;

					        if (ImGui::Combo("Select Theme", &currentTheme, themes, IM_ARRAYSIZE(themes)))
					        {
					            switch (currentTheme)
					            {
					                case 0: // SoDark
					                    ImGui::GetStyle() = UI::SoDark(hue);
					                break;
					                case 1: // Classic
					                    ImGui::StyleColorsClassic();
					                break;
					                case 2: // Dark
					                    ImGui::StyleColorsDark();
					                break;
					                case 3: // Light
					                    ImGui::StyleColorsLight();
					                break;
					            }
					        }

					        if (currentTheme == 0 && ImGui::SliderFloat("Hue", &hue, 0.0f, 1.0f))
					        {
					            ImGui::GetStyle() = UI::SoDark(hue);
					        }

					        ImGui::EndMenu();
					    }

						ImGui::EndMenu();
					}

					// Buttons
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

					float buttonSize = ImGui::GetFrameHeightWithSpacing();

					ImGui::SameLine(ImGui::GetContentRegionMax().x - 3 * (buttonSize + ImGui::GetStyle().ItemSpacing.x));
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


				if (showEditor) UI_Editor();
				if (showSceneProperties) UI_SceneProperties();
				if (showEntities) UI_Entities();
				if (showProperties) UI_Properties();
				if (showConsole) UI_Console();
				if (showAssets) UI_Assets();
			    if (showDebugStats) UI_DebugStats();
			}
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
			t_FolderClosed.Destroy();
			t_FolderOpen.Destroy();
			t_File.Destroy();
			t_Folder.Destroy();
			t_FolderEmpty.Destroy();

	        assetManager.Free();
			m_Renderer.Deinit();

	        for (int i = 0; i < FRAME_OVERLAP; i++)
	            m_RenderData[i].Free();
		}
	};
}