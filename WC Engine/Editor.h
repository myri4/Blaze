#pragma once

#include <fstream>

#include <glm/gtc/type_ptr.hpp>

#include <wc/Utils/List.h>
#include <wc/Utils/FileDialogs.h>

#include "Rendering/Renderer2D.h"

#include "UI/Widgets.h"

#include "Scene/Scene.h"

#include "Globals.h"
#include "Project/Settings.h"
#include "Scripting/Script.h"

#include <wc/Math/Camera.h>

using namespace wc;
namespace gui = ImGui;

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

	glm::vec2 WindowPos;
	glm::vec2 RenderSize;

	glm::vec2 ViewPortSize = glm::vec2(1.f);

	OrthographicCamera camera;

	RenderData m_RenderData[FRAME_OVERLAP];
	Renderer2D m_Renderer;

	b2DebugDraw m_PhysicsDebugDraw;

	Texture t_Close;
	Texture t_Minimize;
	Texture t_Collapse;
	Texture t_FolderOpen;
	Texture t_FolderClosed;
	Texture t_File;
	Texture t_FolderEmpty;
	Texture t_Folder;
	Texture t_Reorder;
	Texture t_Bond;
	Texture t_Play;
	Texture t_Simulate;
	Texture t_Stop;
	Texture t_Add;

	bool allowInput = true;

	bool showEditor = true;
	bool showSceneProperties = true;
	bool showEntities = true;
	bool showProperties = true;
	bool showConsole = true;
	bool showFileExplorer = true;
	bool showDebugStats = false;
	bool showStyleEditor = false;

	flecs::entity m_SelectedEntity = flecs::entity::null();

	enum class SceneState { Edit, Simulate, Play };
	SceneState m_SceneState = SceneState::Edit;

	ImGuizmo::OPERATION m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;

	std::string m_ScenePath;
	Scene m_Scene;
	Scene m_TempScene;

	void Create()
	{
		Settings::Load();

		assetManager.Init();

		m_Renderer.Init();

		// Load Textures
		std::string assetPath = "assets/textures/menu/";
		t_Close.Load(assetPath + "close.png");
		t_Minimize.Load(assetPath + "minimize.png");
		t_Collapse.Load(assetPath + "collapse.png");
		t_FolderOpen.Load(assetPath + "folder-open.png");
		t_FolderClosed.Load(assetPath + "folder-closed.png");
		t_File.Load(assetPath + "file.png");
		t_FolderEmpty.Load(assetPath + "folder-empty.png");
		t_Folder.Load(assetPath + "folder-fill.png");
		t_Reorder.Load(assetPath + "reorder.png");
		t_Bond.Load(assetPath + "bond.png");
		t_Play.Load(assetPath + "play.png");
		t_Simulate.Load(assetPath + "simulate.png");
		t_Stop.Load(assetPath + "stop.png");
		t_Add.Load(assetPath + "add.png");

		PhysicsMaterials.push_back(PhysicsMaterial()); // @NOTE: Index 0 is the default material
		LoadPhysicsMaterials("physicsMaterials.yaml");

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
			{ { -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX } },
		};

		m_Renderer.CreateScreen(Globals.window.GetSize());

		//blaze::Script script;
		//script.LoadFromFile("test.lua");
		//script.SetVariable("a", "guz");
		//script.Execute("printSmth");
	}

	void Resize(glm::vec2 size)
	{
		m_Renderer.DestroyScreen();
		m_Renderer.CreateScreen(size);
	}

	void Destroy()
	{
		Settings::Save();

		SavePhysicsMaterials("physicsMaterials.yaml");

		t_Close.Destroy();
		t_Collapse.Destroy();
		t_Minimize.Destroy();
		t_FolderClosed.Destroy();
		t_FolderOpen.Destroy();
		t_File.Destroy();
		t_Folder.Destroy();
		t_FolderEmpty.Destroy();
		t_Reorder.Destroy();
		t_Bond.Destroy();
		t_Play.Destroy();
		t_Simulate.Destroy();
		t_Stop.Destroy();
		t_Add.Destroy();

		assetManager.Free();
		m_Renderer.Deinit();

		for (int i = 0; i < FRAME_OVERLAP; i++)
			m_RenderData[i].Free();
	}

	void Input()
	{
		if (allowInput)
		{
			if (gui::IsKeyPressed(ImGuiKey_G)) m_GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;
			else if (gui::IsKeyPressed(ImGuiKey_R)) m_GuizmoOp = ImGuizmo::OPERATION::ROTATE_Z;
			else if (gui::IsKeyPressed(ImGuiKey_S)) m_GuizmoOp = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Y;

			float scroll = Mouse::GetMouseScroll().y;
			if (scroll != 0.f)
			{
				camera.Zoom += -scroll * Settings::ZoomSpeed;
				camera.Zoom = glm::max(camera.Zoom, 0.05f);
				camera.Update(m_Renderer.GetHalfSize(camera.Zoom));
			}

			glm::vec2 mousePos = (glm::vec2)(Globals.window.GetCursorPos() + Globals.window.GetPosition()) - WindowPos;
			glm::vec2 mouseFinal = m_BeginCameraPosition + m_Renderer.ScreenToWorld(mousePos, camera.Zoom);

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

			renderData.DrawQuad(transform, data.Texture, data.Color, entt.id());
		}
		else if (entt.has<CircleRendererComponent>())
		{
			auto& data = *entt.get<CircleRendererComponent>();
			renderData.DrawCircle(transform, data.Thickness, data.Fade, data.Color, entt.id());
		}
		else if (entt.has<TextRendererComponent>())
		{
			auto& data = *entt.get<TextRendererComponent>();

			if (data.FontID != UINT32_MAX)
				renderData.DrawString(data.Text, assetManager.Fonts[data.FontID], transform, data.Color, data.LineSpacing, data.Kerning, entt.id());
		}
		m_Scene.EntityWorld.query_builder<TransformComponent, EntityTag>()
			.with(flecs::ChildOf, entt)
			.each([&](flecs::entity child, TransformComponent childTransform, EntityTag)
				{
					transform = transform * childTransform.GetTransform();
					RenderEntity(child, transform);
				});
	}

	void Render()
	{
		if (!Project::Exists()) return;
		auto& renderData = m_RenderData[CURRENT_FRAME];

		if (m_Renderer.TextureCapacity < assetManager.Textures.size())
			m_Renderer.AllocateNewDescriptor(assetManager.Textures.capacity());

		if (assetManager.TexturesUpdated)
		{
			assetManager.TexturesUpdated = false;
			m_Renderer.UpdateTextures(assetManager);
		}

		m_Scene.EntityWorld.each([&](flecs::entity entt, TransformComponent& p) {
			if (entt.parent() != 0) return;

			glm::mat4 transform = p.GetTransform();
			RenderEntity(entt, transform);
			});

		if (m_SceneState != SceneState::Edit)
		{
			m_PhysicsDebugDraw.context = &renderData;
			m_Scene.PhysicsWorld.Draw(&m_PhysicsDebugDraw);
		}

		m_Renderer.Flush(renderData, camera.GetViewProjectionMatrix());

		renderData.Reset();
	}

	void Update()
	{
		if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate)
			m_Scene.Update();
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
			m_Scene.PhysicsWorld.Destroy();
			m_Scene.Copy(m_TempScene);
		}

		if (m_SelectedEntity != flecs::entity::null()) m_SelectedEntity = m_Scene.EntityWorld.lookup(selectedEntityName.c_str());
		//WC_INFO("Scene state changed. Selected Entity: {0}", m_SelectedEntity.name().c_str());
	}

	void UI_Editor()
	{
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		if (gui::Begin("Editor", &showEditor))
		{
			allowInput = gui::IsWindowFocused() && gui::IsWindowHovered();

			ImVec2 viewPortSize = gui::GetContentRegionAvail();
			if (ViewPortSize != *((glm::vec2*)&viewPortSize))
			{
				ViewPortSize = { viewPortSize.x, viewPortSize.y };

				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize(ViewPortSize);
			}

			bool allowedSelect = !ImGuizmo::IsOver() && !ImGuizmo::IsUsing();
			//if (m_SelectedEntity == flecs::entity::null()) allowedSelect = true;
			if (Mouse::GetMouse(Mouse::LEFT) && allowedSelect && allowInput)
			{
				auto mousePos = (glm::ivec2)(Mouse::GetCursorPosition() - (glm::uvec2)WindowPos);

				uint32_t width = m_Renderer.m_RenderSize.x;
				uint32_t height = m_Renderer.m_RenderSize.y;

				if (mousePos.x > 0 && mousePos.y > 0 && mousePos.x < width && mousePos.y < height)
				{
					VulkanContext::GetLogicalDevice().WaitIdle();
					// @TODO: Optimize this to download only 1 pixel
					vk::StagingBuffer stagingBuffer;
					stagingBuffer.Allocate(sizeof(uint64_t) * width * height, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
					vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
						auto image = m_Renderer.m_EntityImage;
						image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

						VkBufferImageCopy copyRegion = {
							.imageSubresource = {
								.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								.layerCount = 1,
							},
							.imageExtent = { width, height, 1 },
						};

						vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &copyRegion);

						image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
						});

					auto imagedata = (uint64_t*)stagingBuffer.Map();
					auto id = imagedata[mousePos.x + mousePos.y * width];
					m_SelectedEntity = flecs::entity(m_Scene.EntityWorld, id);

					stagingBuffer.Unmap();
					stagingBuffer.Free();
				}
			}

			auto viewportMinRegion = gui::GetWindowContentRegionMin();
			auto viewportMaxRegion = gui::GetWindowContentRegionMax();
			auto viewportOffset = gui::GetWindowPos();
			ImVec2 viewportBounds[2];
			viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			WindowPos = *((glm::vec2*)&viewportBounds[0]);
			glm::vec2 RenderSize = *((glm::vec2*)&viewportBounds[1]) - WindowPos;

			gui::GetWindowDrawList()->AddImage((ImTextureID)m_Renderer.ImguiImageID, ImVec2(WindowPos.x, WindowPos.y), ImVec2(WindowPos.x + RenderSize.x, WindowPos.y + RenderSize.y));

			gui::SetCursorPosX((gui::GetWindowSize().x - 60 + gui::GetStyle().ItemSpacing.x * 2) * 0.5f);
			bool isPlayingOrSimulating = (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);
			bool isPaused = (m_SceneState == SceneState::Edit);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
			if (isPlayingOrSimulating) gui::BeginDisabled();
			if (gui::ImageButton("play", t_Play, { 10, 10 }) && isPaused) ChangeSceneState(SceneState::Play); gui::SameLine(0, 0); gui::SeparatorEx(ImGuiSeparatorFlags_Vertical); gui::SameLine(0, 0);

			if (gui::ImageButton("simulate", t_Simulate, { 10, 10 }) && isPaused) ChangeSceneState(SceneState::Simulate); gui::SameLine(0, 0); gui::SeparatorEx(ImGuiSeparatorFlags_Vertical); gui::SameLine(0, 0);
			if (isPlayingOrSimulating) gui::EndDisabled();

			if (isPaused) gui::BeginDisabled();
			if (gui::ImageButton("stop", t_Stop, { 10, 10 })) ChangeSceneState(SceneState::Edit);
			if (isPaused) gui::EndDisabled();
			ImGui::PopStyleVar();

			glm::mat4 projection = camera.ProjectionMatrix;
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
						glm::vec3(translation),
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
		gui::PopStyleVar();
		gui::End();
	}

	void UI_SceneProperties()
	{
		if (gui::Begin("Scene Properties", &showSceneProperties))
		{
			if (gui::CollapsingHeader("Physics settings", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto& worldData = m_Scene.PhysicsWorldData;
				ui::DragButton2("Gravity", worldData.Gravity);
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

			if (gui::CollapsingHeader("Physics debug draw", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ui::Checkbox("Use drawing bounds", m_PhysicsDebugDraw.useDrawingBounds);
				ui::Checkbox("Draw shapes", m_PhysicsDebugDraw.drawShapes);
				ui::Checkbox("Draw joints", m_PhysicsDebugDraw.drawJoints);
				ui::Checkbox("Draw joint extras", m_PhysicsDebugDraw.drawJointExtras);
				ui::Checkbox("Draw AABBs", m_PhysicsDebugDraw.drawAABBs);
				ui::Checkbox("Draw mass", m_PhysicsDebugDraw.drawMass);
				ui::Checkbox("Draw body names", m_PhysicsDebugDraw.drawBodyNames);
				ui::Checkbox("Draw contacts", m_PhysicsDebugDraw.drawContacts);
				ui::Checkbox("Draw graph colors", m_PhysicsDebugDraw.drawGraphColors);
				ui::Checkbox("Draw contact normals", m_PhysicsDebugDraw.drawContactNormals);
				ui::Checkbox("Draw contact impulses", m_PhysicsDebugDraw.drawContactImpulses);
				ui::Checkbox("Draw friction impulses", m_PhysicsDebugDraw.drawFrictionImpulses);

			}
		}
		gui::End();
	}

	void ShowEntities()
	{
		if (gui::IsMouseClicked(0) && !gui::IsAnyItemHovered() && gui::IsWindowHovered() && gui::IsWindowFocused())	m_SelectedEntity = flecs::entity::null();

		if (gui::BeginChild("##ShowEntities", { 0, 0 }, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
		{
			auto popup = [&](flecs::entity entity) -> void
				{
					// Display the popup menu
					if (gui::BeginPopup(std::to_string(entity.id()).c_str()))
					{
						gui::Text("%s", entity.name().c_str());
						gui::Separator();

						if (gui::MenuItem("Clone"))
						{
							WC_CORE_INFO("Implement Clone");
							gui::CloseCurrentPopup();
						}

						if (gui::MenuItem("Export"))
						{
							WC_CORE_INFO("Implement Export");
							gui::CloseCurrentPopup();
						}

						if (entity.parent() != flecs::entity::null() && gui::MenuItem("Remove Child"))
						{
							//auto parent = entity.parent();
							m_Scene.RemoveChild(entity);
						}

						if (m_SelectedEntity != flecs::entity::null() && entity != m_SelectedEntity)
						{
							if (gui::MenuItem("Set Child of Selected"))
								m_Scene.SetChild(m_SelectedEntity, entity);
						}

						gui::Separator();

						gui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.92, 0.25f, 0.2f, 1.f));
						if (gui::MenuItem("Delete"))
						{
							m_Scene.KillEntity(entity);
							m_SelectedEntity = flecs::entity::null();
							gui::CloseCurrentPopup();
						}
						gui::PopStyleColor();

						gui::EndPopup();
					}
				};

			// Reorder
			auto separator = [&](flecs::entity entity) -> void
				{
					if (ui::MatchPayloadType("ENTITY"))
					{
						gui::PushStyleColor(ImGuiCol_Separator, { 0.5f, 0.5f, 0.5f, 0.f });
						gui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 4);
						gui::PopStyleColor();

						if (gui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("ENTITY"))
							{
								IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
								flecs::entity droppedEntity = *static_cast<const flecs::entity*>(payload->Data);

								// Only allow reordering if both entities share the same parent.
								if (droppedEntity.parent() == entity.parent())
								{
									// Get the list of names from the proper container.
									// For top-level entities, use the scene's parent names;
									// for children, use the parent's EntityOrderComponent.
									std::vector<std::string>* entityOrder = nullptr;
									if (entity.parent() == flecs::entity::null())
										entityOrder = &m_Scene.EntityOrder;
									else if (entity.parent().has<EntityOrderComponent>())
										entityOrder = &entity.parent().get_ref<EntityOrderComponent>()->EntityOrder;

									if (entityOrder)
									{
										// Find the positions of the target (the entity associated with the separator)
										// and the dropped entity.
										auto targetIt = std::find(entityOrder->begin(), entityOrder->end(), std::string(entity.name()));
										auto droppedIt = std::find(entityOrder->begin(), entityOrder->end(), std::string(droppedEntity.name()));

										if (targetIt != entityOrder->end() && droppedIt != entityOrder->end() && targetIt != droppedIt)
										{
											// Remove the dropped entity from its current position.
											std::string droppedName = *droppedIt;
											entityOrder->erase(droppedIt);

											// Recalculate the target's index (in case removal shifted it).
											int newTargetIndex = std::distance(entityOrder->begin(),
												std::find(entityOrder->begin(), entityOrder->end(), std::string(entity.name())));

											// Insert the dropped entity immediately after the target.
											int insertIndex = newTargetIndex + 1;
											if (insertIndex > static_cast<int>(entityOrder->size()))
												insertIndex = static_cast<int>(entityOrder->size());
											entityOrder->insert(entityOrder->begin() + insertIndex, droppedName);
										}
									}
								}
							}
							gui::EndDragDropTarget();
						}
					}
				};

			auto displayEntity = [&](flecs::entity entity, auto& displayEntityRef) -> void
				{
					const bool is_selected = (m_SelectedEntity == entity);

					std::vector<flecs::entity> children;
					if (entity.has<EntityOrderComponent>())
					{
						auto& entityOrder = entity.get<EntityOrderComponent>()->EntityOrder;
						for (const auto& childName : entityOrder)
						{
							std::string fullChildName = std::string(entity.name()) + "::" + childName;

							flecs::entity childEntity = entity;
							while (childEntity.parent() != flecs::entity::null())
							{
								childEntity = childEntity.parent();
								fullChildName = std::string(childEntity.name()) + "::" + fullChildName;
							}

							if (auto childEntityResolved = m_Scene.EntityWorld.lookup(fullChildName.c_str()))
								children.push_back(childEntityResolved);
						}
					}

					ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (is_selected)
						node_flags |= ImGuiTreeNodeFlags_Selected;

					if (children.empty())
						node_flags |= ImGuiTreeNodeFlags_Leaf;

					if (m_SelectedEntity != flecs::entity::null())
					{
						auto parent = m_SelectedEntity.parent();
						while (parent != flecs::entity::null())
						{
							if (parent == entity)
							{
								gui::SetNextItemOpen(true);
								break;
							}
							parent = parent.parent();
						}
					}

					// Render the entity
					bool is_open = gui::TreeNodeEx(entity.name().c_str(), node_flags);

					if (gui::IsWindowHovered() && gui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						if (gui::IsItemHovered())
						{
							WC_INFO("Hovered {}", entity.name().c_str());
							gui::OpenPopup(std::to_string(entity.id()).c_str());
						}
					}
					popup(entity);

					// Handle selection on click
					if (gui::IsItemClicked() && !gui::IsItemToggledOpen()) m_SelectedEntity = entity;

					if (gui::IsMouseDoubleClicked(0) && gui::IsItemHovered())
					{
						if (entity.has<TransformComponent>())
							camera.Position = entity.get<TransformComponent>()->Translation;
					}

					if (gui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						gui::SetDragDropPayload("ENTITY", &entity, sizeof(flecs::entity));
						gui::Text("Dragging %s", entity.name().c_str());
						gui::EndDragDropSource();
					}

					// Bond
					if (gui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("ENTITY"))
						{
							IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
							flecs::entity droppedEntity = *(const flecs::entity*)payload->Data;

							// Check if the drop target is empty space
							if (droppedEntity != entity.parent())
								m_Scene.SetChild(entity, droppedEntity);
						}
						gui::EndDragDropTarget();
					}

					// If the node is open, recursively display children
					if (is_open)
					{
						gui::TreePop(); // Pop before separator

						separator(entity);

						gui::TreePush(entity.name().c_str());
						for (const auto& child : children)
							displayEntityRef(child, displayEntityRef);
						gui::TreePop(); // Ensure proper pairing of TreeNodeEx and TreePop
					}
					else separator(entity);

				};

			ui::DrawBgRows(10);
			gui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ui::MatchPayloadType("ENTITY") ? (10 - 4) * 0.5f : 10);
			for (const auto& name : m_Scene.EntityOrder)
			{
				auto rootEntity = m_Scene.EntityWorld.lookup(name.c_str());
				if (rootEntity) displayEntity(rootEntity, displayEntity);
			}
			gui::PopStyleVar();

			if (gui::IsWindowHovered() && gui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				if (!gui::IsAnyItemHovered() && m_SelectedEntity != flecs::entity::null())
				{
					WC_INFO("Hovered SELECTED {}", m_SelectedEntity.name().c_str());
					gui::OpenPopup(std::to_string(m_SelectedEntity.id()).c_str());
				}
			}
			popup(m_SelectedEntity);
		}
		gui::EndChild();
	}

	void UI_Entities()
	{
		static bool showPopup = false;
		if (gui::Begin("Entities", &showEntities, ImGuiWindowFlags_MenuBar))
		{
			std::string entityFilter;
			if (gui::BeginMenuBar())
			{
				//TODO - Implement search
				char filterBuff[256];
				//TODO - fix, imgui is DnD active crashes flecs for some reason
				bool buttonDnD = ui::MatchPayloadType("ENTITY") && m_SelectedEntity && m_SelectedEntity.parent();
				//WC_INFO("1: {}", gui::IsDragDropActive());
				gui::SetCursorPosX(gui::GetStyle().ItemSpacing.x);
				gui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
				gui::PushFont(Globals.f_Display.Bold);
				gui::SetNextItemWidth(gui::GetContentRegionAvail().x - gui::CalcTextSize(buttonDnD ? "Remove Parent" : "Add Entity").x - gui::GetStyle().ItemSpacing.x * 2 + gui::GetStyle().WindowPadding.x - gui::GetStyle().FramePadding.x * 2);
				if (gui::InputTextEx("##Search", "Filter by Name", filterBuff, IM_ARRAYSIZE(filterBuff), ImVec2(0, 0), ImGuiInputTextFlags_AutoSelectAll)) { entityFilter = filterBuff; }

				gui::BeginDisabled(buttonDnD);
				ui::PushButtonColor(gui::GetStyle().Colors[ImGuiCol_CheckMark]);
				if (gui::Button(buttonDnD ? "Remove Parent" : "Add Entity")) showPopup = true;
				gui::PopFont();
				gui::PopStyleVar();
				gui::PopStyleColor(3);
				gui::EndDisabled();

				if (buttonDnD && gui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("ENTITY"))
					{
						IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
						flecs::entity droppedEntity = *(const flecs::entity*)payload->Data;

						if (droppedEntity.parent() != flecs::entity::null())
							m_Scene.RemoveChild(droppedEntity);
					}
					gui::EndDragDropTarget();
				}

				gui::EndMenuBar();
			}

			ShowEntities();

			if (showPopup)
			{
				gui::OpenPopup("Add Entity");
				showPopup = false;
			}

			ui::CenterNextWindow();
			if (gui::BeginPopupModal("Add Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImVec2 center = gui::GetMainViewport()->GetCenter();
				ImVec2 windowSize = gui::GetWindowSize();
				ImVec2 windowPos = ImVec2(center.x - windowSize.x * 0.5f, center.y - windowSize.y * 0.5f);
				gui::SetWindowPos(windowPos, ImGuiCond_Once);

				static std::string name = "Entity " + std::to_string(m_Scene.EntityWorld.count<EntityTag>());
				if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
				gui::InputText("Name", &name, ImGuiInputTextFlags_AutoSelectAll);

				const float widgetSize = gui::GetItemRectSize().x;

				gui::BeginDisabled(name.empty());
				if (gui::Button("Create") || gui::IsKeyPressed(ImGuiKey_Enter))
				{
					if (m_Scene.EntityWorld.lookup(name.c_str()) == flecs::entity::null())
					{
						m_SelectedEntity = m_Scene.AddEntity(name);
						name = "Entity " + std::to_string(m_Scene.EntityWorld.count<EntityTag>());
						gui::CloseCurrentPopup();
					}
					else gui::OpenPopup("WarnNameExists");
				}
				gui::EndDisabled();
				if (name.empty()) gui::SetItemTooltip("Name cannot be empty");

				gui::SameLine();
				gui::SetCursorPosX(widgetSize);
				if (gui::Button("Cancel") || gui::IsKeyPressed(ImGuiKey_Escape))
				{
					name = "Entity " + std::to_string(m_Scene.EntityWorld.count<EntityTag>());
					gui::CloseCurrentPopup();
				}

				ui::CenterNextWindow();
				if (gui::BeginPopupModal("WarnNameExists", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
				{
					gui::Text("Name already exists!");

					if (gui::Button("Close") || gui::IsKeyPressed(ImGuiKey_Enter) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();
					gui::EndPopup();
				}

				gui::EndPopup();
			}
		}
		gui::End();
	}

	template<typename T, typename UIFunc>
	void EditComponent(const std::string& name, UIFunc uiFunc)
	{
		if (m_SelectedEntity.has<T>())
		{
			bool visible = true;

			auto& component = *m_SelectedEntity.get_mut<T>();
			if (gui::CollapsingHeader((name + "##header").c_str(), m_SceneState == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
				uiFunc(component);

			if (!visible) m_SelectedEntity.remove<T>(); // add modal popup
		}
	}

	void UI_Properties()
	{
		if (gui::Begin("Properties", &showProperties))
		{
			if (m_SelectedEntity != flecs::entity::null())
			{
				std::string nameBuffer = m_SelectedEntity.name().c_str();

				gui::PushItemWidth(gui::GetContentRegionAvail().x - gui::GetStyle().ItemSpacing.x * 2 - gui::CalcTextSize("Add Component(?)").x - gui::GetStyle().FramePadding.x * 2);
				if (gui::InputText("##Name", &nameBuffer, ImGuiInputTextFlags_EnterReturnsTrue) || gui::IsItemDeactivatedAfterEdit())
				{
					if (!nameBuffer.empty())
					{
						if (m_Scene.EntityWorld.lookup(nameBuffer.c_str()) != flecs::entity::null())
							gui::OpenPopup("WarnNameExists");
						else
						{
							if (m_SelectedEntity.parent() == flecs::entity::null())
							{
								auto& parentNames = m_Scene.EntityOrder;
								for (auto& name : parentNames)
									if (name == m_SelectedEntity.name().c_str()) name = nameBuffer;
							}
							else
							{
								auto& childrenNames = m_SelectedEntity.parent().get_ref<EntityOrderComponent>()->EntityOrder;
								for (auto& name : childrenNames)
									if (name == m_SelectedEntity.name().c_str()) name = nameBuffer;
							}

							m_SelectedEntity.set_name(nameBuffer.c_str());
						}
					}
				}
				const float widgetSize = gui::GetItemRectSize().y;
				static enum { None, Transform, Render, Rigid } menu = None;
				static bool showAddComponent = false;
				//TODO - add icon
				gui::SameLine();  if (gui::Button("Add Component")) { showAddComponent = true; menu = None; }
				ui::HelpMarker("Press ENTER or Deselect to confirm Name change");
				auto ItemAutoClose = [](const char* label, bool disabled) -> bool
					{
						if (gui::MenuItem(label, nullptr, nullptr, !disabled))
						{
							showAddComponent = false;
							return true;
						}
						if (disabled)
							gui::SetItemTooltip("You can't have more than one of this component!");

						return false;
					};

				if (showAddComponent)
				{
					{
						ImVec2 popupPos = gui::GetItemRectMin();
						popupPos.y = gui::GetItemRectMax().y + 5;
						ImVec2 popupSize = { 200, 150 };

						const ImGuiViewport* viewport = gui::GetWindowViewport();
						ImVec2 viewportMin = viewport->Pos;
						ImVec2 viewportMax = { viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y };

						popupPos.x = std::clamp(popupPos.x, viewportMin.x, viewportMax.x - popupSize.x);
						popupPos.y = std::clamp(popupPos.y, viewportMin.y, viewportMax.y - popupSize.y);

						gui::SetNextWindowPos(popupPos);
						gui::SetNextWindowSize(popupSize, ImGuiCond_Once);
					}

					if (gui::Begin("Add##Component", &showAddComponent, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar))
					{
						if (!gui::IsWindowFocused()) showAddComponent = false;

						if (menu != None && gui::ArrowButton("Back", ImGuiDir_Left))
							menu = None;

						gui::SameLine();

						const char* menuText = "";
						switch (menu)
						{
						case None: menuText = "Components"; break;
						case Transform: menuText = "Transform"; break;
						case Render: menuText = "Render"; break;
						case Rigid: menuText = "Rigid Body"; break;
						}
						gui::SetCursorPosX((gui::GetWindowSize().x - gui::CalcTextSize(menuText).x) * 0.5f);
						gui::Text("%s", menuText);

						gui::Separator();

						switch (menu)
						{
						case None:
						{
							if (gui::MenuItem("Transform")) { menu = Transform; } ui::RenderArrowIcon();
							if (gui::MenuItem("Render")) { menu = Render; } ui::RenderArrowIcon();
							if (gui::MenuItem("Rigid Body")) { menu = Rigid; } ui::RenderArrowIcon();
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
					gui::End();
				}

				ui::CenterNextWindow();
				if (gui::BeginPopupModal("WarnNameExists", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
				{
					gui::Text("Name already exists!");
					if (gui::Button("Close") || gui::IsKeyPressed(ImGuiKey_Enter) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();
					gui::EndPopup();
				}

				// Display the entity's ID
				//gui::Text("ID: %u", selected_entity.id());

				//NOTE: for every new component, a new if statement is needed
				gui::SeparatorText("Components");

				if (gui::BeginChild("components"))
				{
					EditComponent<TransformComponent>("Transform", [](auto& component) {
						auto& realRotation = const_cast<float&>(component.Rotation);
						auto rotation = glm::degrees(realRotation);

						ui::DragButton3("Position", component.Translation);
						ui::DragButton2("Scale", component.Scale);
						ui::Drag("Rotation", rotation, 0.5f, 0.f, 360.f);
						realRotation = glm::radians(rotation);
						});

					EditComponent<SpriteRendererComponent>("Sprite Renderer", [&](auto& component)
						{
							gui::ColorEdit4("color", glm::value_ptr(component.Color));
							gui::Button("Texture");

							if (ui::MatchPayloadType("DND_PATH"))
							{
								std::filesystem::path path = static_cast<const char*>(gui::GetDragDropPayload()->Data);
								if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg" ||
									path.extension() == ".bmp" || path.extension() == ".tga")
								{
									if (gui::BeginDragDropTarget())
									{
										if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("DND_PATH"))
										{
											const char* texturePath = static_cast<const char*>(payload->Data);

											component.Texture = assetManager.LoadTexture(texturePath);
										}
										gui::EndDragDropTarget();
									}
								}
							}
						});

					EditComponent<CircleRendererComponent>("Circle Renderer", [](auto& component) {
						ui::Slider("Thickness", component.Thickness, 0.0f, 1.0f);
						ui::Slider("Fade", component.Fade, 0.0f, 1.0f);
						gui::ColorEdit4("Color", glm::value_ptr(component.Color));
						});

					EditComponent<TextRendererComponent>("Text Renderer", [&](auto& component) {
						gui::InputText("Text", &component.Text);
						gui::ColorEdit4("Color", glm::value_ptr(component.Color));
						ui::Drag("Line spacing", component.LineSpacing);
						ui::Drag("Kerning", component.Kerning);

						gui::Button("Font");

						if (ui::MatchPayloadType("DND_PATH"))
						{
							std::filesystem::path path = static_cast<const char*>(gui::GetDragDropPayload()->Data);
							if (path.extension() == ".ttf" || path.extension() == ".otf")
							{
								if (gui::BeginDragDropTarget())
								{
									if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("DND_PATH"))
									{
										const char* fontPath = static_cast<const char*>(payload->Data);
										WC_CORE_INFO(fontPath);

										component.FontID = assetManager.LoadFont(fontPath);
									}
									gui::EndDragDropTarget();
								}
							}
						}
						});

					auto UI_PhysicsMaterial = [&](uint32_t& currentMaterial) // @TODO: Some more error handling for completely invalid IDs?
						{
							ui::Separator("Material");
														
							std::string currentMaterialName = "Unknown";

							for (const auto& [name, matID] : PhysicsMaterialNames)
							{
								if (matID == currentMaterial)
								{
									currentMaterialName = name;
									break;
								}
							}

							if (gui::BeginCombo("Materials", currentMaterialName.c_str()))
							{
								gui::PushStyleColor(ImGuiCol_FrameBg, gui::GetStyle().Colors[ImGuiCol_PopupBg]);
								if (gui::Button("New Material##Button", { gui::GetContentRegionAvail().x, 0 }))
									gui::OpenPopup("Create Material##popup");
								gui::PopStyleColor();

								ui::CenterNextWindow();
								if (gui::BeginPopupModal("Create Material##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
								{
									static std::string name;
									if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
									gui::InputText("Name", &name);
									const float widgetSize = gui::GetItemRectSize().x;

									if (gui::Button("Create") || gui::IsKeyPressed(ImGuiKey_Enter))
									{
										PhysicsMaterials.push_back(PhysicsMaterial());
										currentMaterial = PhysicsMaterials.size() - 1;
										PhysicsMaterialNames[name] = currentMaterial;
										name = "";
										gui::CloseCurrentPopup();
									}
									gui::SameLine();
									gui::SetCursorPosX(widgetSize);
									if (gui::Button("Cancel") || gui::IsKeyPressed(ImGuiKey_Escape))
									{
										name = "";
										gui::CloseCurrentPopup();
									}
									gui::EndPopup();
								}
								gui::Separator();

								for (const auto& [name, matID] : PhysicsMaterialNames)
								{
									bool isSelected = (currentMaterial == matID);
									gui::PushStyleColor(ImGuiCol_Header, gui::GetStyle().Colors[ImGuiCol_PopupBg]);
									if (gui::Selectable(name.c_str(), isSelected))
										currentMaterial = matID;

									gui::PopStyleColor();
									if (isSelected)
										gui::SetItemDefaultFocus();
								}
								gui::EndCombo();
							}

							// Get the selected material
							auto& curMaterial = PhysicsMaterials[currentMaterial];

							gui::BeginDisabled(currentMaterial == 0);
							gui::BeginGroup();
							ui::Drag("Density", curMaterial.Density);
							ui::Drag("Friction", curMaterial.Friction);
							ui::Drag("Restitution", curMaterial.Restitution);
							ui::Drag("Rolling Resistance", curMaterial.RollingResistance);

							ui::Separator();
							gui::ColorEdit4("Debug Color", glm::value_ptr(curMaterial.DebugColor));

							ui::Separator();
							ui::Checkbox("Sensor", curMaterial.Sensor);
							ui::Checkbox("Enable Contact Events", curMaterial.EnableContactEvents);
							ui::Checkbox("Enable Hit Events", curMaterial.EnableHitEvents);
							ui::Checkbox("Enable Pre-Solve Events", curMaterial.EnablePreSolveEvents);
							ui::Checkbox("Invoke Contact Creation", curMaterial.InvokeContactCreation);
							ui::Checkbox("Update Body Mass", curMaterial.UpdateBodyMass);
							gui::EndGroup();
							gui::EndDisabled();

							if (currentMaterial == 0) gui::SetItemTooltip("Cannot edit Default material values");
						};

					EditComponent<RigidBodyComponent>("Rigid Body", [](auto& component) {

						const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
						const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];

						if (gui::BeginCombo("Body Type", currentBodyTypeString))
						{
							for (int i = 0; i < 3; i++)
							{
								bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
								if (gui::Selectable(bodyTypeStrings[i], &isSelected))
								{
									currentBodyTypeString = bodyTypeStrings[i];
									component.Type = BodyType(i);
								}

								if (isSelected)
									gui::SetItemDefaultFocus();
							}
							gui::EndCombo();
						}

						ui::Drag("Gravity Scale", component.GravityScale);
						ui::Drag("Linear Damping", component.LinearDamping);
						ui::Drag("Angular Damping", component.AngularDamping);
						ui::Checkbox("Fixed Rotation", component.FixedRotation);
						ui::Checkbox("Bullet", component.Bullet);
						ui::Checkbox("Fast Rotation", component.FastRotation);
						});

					EditComponent<BoxCollider2DComponent>("Box Collider", [&](auto& component) {
						ui::DragButton2("Offset", component.Offset);
						ui::DragButton2("Size", component.Size);

						UI_PhysicsMaterial(component.MaterialID);
						});

					EditComponent<CircleCollider2DComponent>("Circle Collider", [&](auto& component) {
						ui::DragButton2("Offset", component.Offset);
						ui::Drag("Radius", component.Radius);

						UI_PhysicsMaterial(component.MaterialID);
						});
				}
				gui::EndChild();
			}
		}
		gui::End();
	}

	const std::unordered_map<spdlog::level::level_enum, std::pair<glm::vec4, std::string>> level_colors = { //@TODO: This could be deduced to just an array of colors
		{spdlog::level::debug, {glm::vec4(58.f, 150.f, 221.f, 255.f) / 255.f, "[debug] "}},
		{spdlog::level::info, {glm::vec4(19.f, 161.f, 14.f, 255.f) / 255.f, "[info] "}},
		{spdlog::level::warn, {glm::vec4(249.f, 241.f, 165.f, 255.f) / 255.f, "[warn!] "}},
		{spdlog::level::err, {glm::vec4(231.f, 72.f, 86.f, 255.f) / 255.f, "[-ERROR-] "}},
		{spdlog::level::critical, {glm::vec4(139.f, 0.f, 0.f, 255.f) / 255.f, "[!CRITICAL!] "}}
	};

	void UI_Console()
	{
		if (gui::Begin("Console", &showConsole))
		{
			if (gui::Button("Clear"))
			{
				//Globals.console.Clear();
				Log::GetConsoleSink()->messages.clear();
			}

			gui::SameLine();
			if (gui::Button("Copy"))
			{
				std::string logData;
				for (const auto& msg : Log::GetConsoleSink()->messages)
				{
					auto local_time = msg.time + std::chrono::hours(2); // TODO - fix so it checks timezone
					std::string timeStr = std::format("[{:%H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(local_time));
					logData += timeStr + msg.payload + "\n"; // Assuming msg.payload is a string
				}
				gui::SetClipboardText(logData.c_str());
			}

			// TODO - make work
			gui::SameLine();
			gui::PushItemWidth(ImGui::GetContentRegionAvail().x - gui::CalcTextSize("Filter").x - gui::GetStyle().ItemSpacing.x);
			gui::InputText("Input", const_cast<char*>(""), 0);

			gui::Separator();

			const float footer_height_to_reserve = gui::GetStyle().ItemSpacing.y + gui::GetFrameHeightWithSpacing();
			if (gui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
			{
				for (auto& msg : Log::GetConsoleSink()->messages)
				{
					auto it = level_colors.find(msg.level);
					if (it != level_colors.end())
					{
						const auto& [color, prefix] = it->second;
						auto local_time = msg.time + std::chrono::hours(2); // TODO - fix so it checks timezone
						std::string timeStr = std::format("[{:%H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(local_time));
						gui::PushStyleColor(ImGuiCol_Text, { color.r, color.g, color.b, color.a });
						ui::Text(timeStr + prefix + msg.payload);
						gui::PopStyleColor();
					}
					else
						ui::Text(msg.payload);
				}

				if (/*ScrollToBottom ||*/
					(/*m_ConsoleAutoScroll && */ gui::GetScrollY() >= gui::GetScrollMaxY()))
					gui::SetScrollHereY(1.f);
			}
			gui::EndChild();
		}
		gui::End();
	}

	void UI_Assets()
	{
		auto assetsPath = std::filesystem::path(Project::rootPath);
		static std::unordered_map<std::string, bool> folderStates;  // Track the expansion state per folder
		static std::filesystem::path selectedFolderPath = assetsPath;
		static std::vector<std::filesystem::path> openedFiles;
		static std::unordered_set<std::string> openedFileNames;
		static bool showIcons = true;
		static bool previewAsset = true;

		// reset variables every time root changes
		static std::filesystem::path prevRootPath;
		if (assetsPath != prevRootPath)
		{
			selectedFolderPath = assetsPath;
			folderStates.clear();
			openedFiles.clear();
			openedFileNames.clear();
			prevRootPath = assetsPath;
		}

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

		if (gui::Begin("Assets", &showFileExplorer, ImGuiWindowFlags_MenuBar))
		{
			if (gui::BeginMenuBar())
			{
				if (gui::MenuItem("Import"))
					WC_INFO("TODO - Implement Import");

				if (gui::BeginMenu("View##Assets"))
				{
					if (gui::MenuItem("Show Icons", nullptr, &showIcons))
						WC_INFO("Show Icons: {}", showIcons);

					if (gui::MenuItem("Preview Assets", nullptr, &previewAsset))
						WC_INFO("Preview Asset: {}", previewAsset);

					if (gui::MenuItem("Collapse All"))
						for (auto& [key, value] : folderStates)
							value = false;

					if (gui::MenuItem("Expand All"))
						setFolderStatesRecursively(assetsPath, true);

					gui::EndMenu();
				}

				gui::EndMenuBar();
			}

			std::function<void(const std::filesystem::path&)> displayDirectory;

			displayDirectory = [&](const std::filesystem::path& path)
				{
					if (path != assetsPath) gui::Indent(20);

					for (const auto& entry : std::filesystem::directory_iterator(path))
					{
						const auto& filenameStr = entry.path().filename().string();
						const auto& fullPathStr = entry.path().string();
						if (std::filesystem::exists(entry.path()))
						{
							if (entry.is_directory())
							{
								auto [it, inserted] = folderStates.try_emplace(fullPathStr, false);
								bool& isOpen = it->second;

								gui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
								gui::PushStyleColor(ImGuiCol_Button, gui::GetStyle().Colors[ImGuiCol_WindowBg]);
								if (gui::ImageButton((filenameStr + "##b" + fullPathStr).c_str(), isOpen ? t_FolderOpen : t_FolderClosed, ImVec2(16, 16)))
									isOpen = !isOpen;

								gui::PopStyleColor();
								gui::PopStyleVar();
								gui::SameLine();

								if (gui::Selectable((filenameStr + "##" + fullPathStr).c_str(), selectedFolderPath == entry.path() && showIcons, ImGuiSelectableFlags_DontClosePopups))
									selectedFolderPath = entry.path();

								if (gui::IsItemHovered() && gui::IsMouseDoubleClicked(0))
									isOpen = !isOpen;

								if (isOpen)
									displayDirectory(entry.path());
							}
							else
							{
								ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
								gui::TreeNodeEx((filenameStr + "##" + fullPathStr).c_str(), leafFlags);
								if (gui::IsItemHovered())
								{
									if (previewAsset) gui::OpenPopup(("PreviewAsset##" + fullPathStr).c_str());

									if (gui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
										if (entry.path().extension() != ".scene")
										{
											if (openedFileNames.insert(fullPathStr).second) openedFiles.push_back(entry.path());
										}
										else gui::OpenPopup(("Confirm##" + entry.path().string()).c_str());

									gui::SetNextWindowPos({ gui::GetCursorScreenPos().x + gui::GetItemRectSize().x, gui::GetCursorScreenPos().y });
									if (gui::BeginPopup(("PreviewAsset##" + fullPathStr).c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMouseInputs))
									{
										gui::Text("Preview: %s", filenameStr.c_str());
										gui::EndPopup();
									}
								}

								ui::CenterNextWindow();
								if (gui::BeginPopupModal(("Confirm##" + entry.path().string()).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
								{
									gui::Text("Are you sure you want to load this scene? -> %s", entry.path().filename().string().c_str());
									const float widgetWidth = gui::GetItemRectSize().x;
									if (gui::Button("Yes##Confirm") || gui::IsKeyPressed(ImGuiKey_Enter))
									{
										m_ScenePath = entry.path().string();
										WC_INFO(m_ScenePath);
										gui::CloseCurrentPopup();
									}
									gui::SameLine(widgetWidth - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
									if (gui::Button("Cancel##Confirm") || gui::IsKeyPressed(ImGuiKey_Escape))
										gui::CloseCurrentPopup();

									gui::EndPopup();
								}
							}
						}
					}

					if (path != assetsPath) gui::Unindent(20);
				};

			if (showIcons)
			{
				if (gui::BeginTable("assets_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
				{
					gui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					gui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

					gui::TableNextRow();
					gui::TableSetColumnIndex(0);

					gui::BeginChild("FoldersChild##1", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
					displayDirectory(assetsPath);
					gui::EndChild();

					gui::TableSetColumnIndex(1);

					//show icons
					if (gui::BeginChild("Path Viewer", ImVec2{ 0, 0 }, true))
					{
						if (selectedFolderPath == assetsPath)
						{
							gui::BeginDisabled();
							gui::ArrowButton("Back", ImGuiDir_Left);
							gui::EndDisabled();
						}
						else
						{
							if (gui::ArrowButton("Back", ImGuiDir_Left)) selectedFolderPath = selectedFolderPath.parent_path();

							if (gui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("DND_PATH"))
								{
									const char* payloadPath = static_cast<const char*>(payload->Data);
									std::filesystem::path sourcePath(payloadPath);

									try
									{
										std::filesystem::rename(sourcePath, sourcePath.parent_path().parent_path() / sourcePath.filename());
									}
									catch (const std::exception& e)
									{
										WC_ERROR(e.what());
									}
								}
								gui::EndDragDropTarget();
							}
						}

						static std::string previewPath;
						gui::SameLine();
						//gui::PushItemWidth(gui::GetContentRegionAvail().x - gui::CalcTextSize("Copy Path").x - gui::GetStyle().FramePadding.x * 2 - gui::GetStyle().ItemSpacing.x);
						gui::PushItemWidth(gui::GetContentRegionAvail().x);
						gui::InputText("##PreviewPath", &previewPath, ImGuiInputTextFlags_ReadOnly);
						gui::PopItemWidth();
						if (gui::IsItemHovered() || gui::IsItemActive()) previewPath = assetsPath.string();
						else previewPath = std::filesystem::relative(selectedFolderPath, assetsPath.parent_path()).string();

						gui::Separator();

						constexpr float buttonSize = 70;
						float totalButtonWidth = buttonSize + gui::GetStyle().ItemSpacing.x;
						int itemsPerRow = static_cast<int>((gui::GetContentRegionAvail().x + gui::GetStyle().ItemSpacing.x) / totalButtonWidth);

						if (itemsPerRow < 1) itemsPerRow = 1; // Ensure at least 1 button per row

						if (std::filesystem::exists(selectedFolderPath))
						{
							int i = 0;
							for (const auto& entry : std::filesystem::directory_iterator(selectedFolderPath))
							{
								if (i > 0 && i % itemsPerRow != 0)
									gui::SameLine();

								if (std::filesystem::exists(entry.path()))
								{
									if (entry.is_directory())
									{
										gui::BeginGroup();
										gui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
										gui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
										// TODO - fix: std::filesystem::is_empty(entry.path()) ? t_FolderEmpty : t_Folder
										gui::ImageButton((entry.path().string() + "/").c_str(), t_FolderEmpty, { buttonSize, buttonSize });
										if (gui::IsItemHovered() && gui::IsMouseDoubleClicked(0)) selectedFolderPath = entry.path();

										if (gui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
										{
											std::string path = entry.path().string();
											gui::SetDragDropPayload("DND_PATH", path.c_str(), path.size() + 1);
											gui::Text("Moving %s", entry.path().filename().string().c_str());
											gui::EndDragDropSource();
										}

										if (gui::BeginDragDropTarget())
										{
											if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("DND_PATH"))
											{
												const char* payloadPath = static_cast<const char*>(payload->Data);
												std::filesystem::path sourcePath(payloadPath);
												std::filesystem::path targetPath = entry.path() / sourcePath.filename();

												std::filesystem::rename(sourcePath, targetPath);
											}
											gui::EndDragDropTarget();
										}

										gui::PopStyleColor();
										gui::PopStyleVar();

										std::string filename = entry.path().filename().string();
										float wrapWidth = buttonSize; // Width for wrapping the text

										// Split the text into words
										std::istringstream stream(filename);
										std::vector<std::string> words{ std::istream_iterator<std::string>{stream}, std::istream_iterator<std::string>{} };

										// Display each line centered
										std::string currentLine;
										float currentLineWidth = 0.0f;
										gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 }); // Reduce item spacing for text
										for (const auto& word : words)
										{
											ImVec2 wordSize = gui::CalcTextSize(word.c_str());
											if (currentLineWidth + wordSize.x > wrapWidth)
											{
												// Push the current line and start a new one
												gui::SetCursorPosX(gui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
												gui::TextUnformatted(currentLine.c_str());
												currentLine.clear();
												currentLineWidth = 0.0f;
											}

											if (!currentLine.empty())
											{
												currentLine += " "; // Add a space before the next word
												currentLineWidth += gui::CalcTextSize(" ").x;
											}
											currentLine += word;
											currentLineWidth += wordSize.x;
										}

										// Add the last line if there's any remaining text
										if (!currentLine.empty())
										{
											gui::SetCursorPosX(gui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
											gui::TextUnformatted(currentLine.c_str());
										}
										gui::PopStyleVar(); // Restore item spacing for text

										gui::EndGroup();
									}
									else
									{
										gui::BeginGroup();
										gui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
										gui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
										gui::ImageButton((entry.path().string() + "/").c_str(), t_File, { buttonSize, buttonSize });
										if (gui::IsItemHovered() && gui::IsMouseDoubleClicked(0))
										{
											if (entry.path().extension() != ".scene")
											{
												if (openedFileNames.insert(entry.path().string()).second)
													openedFiles.push_back(entry.path());
											}
											else
												gui::OpenPopup(("Confirm##" + entry.path().string()).c_str());
										}

										// Drag source for files
										if (gui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
										{
											std::string path = entry.path().string();
											gui::SetDragDropPayload("DND_PATH", path.c_str(), path.size() + 1);
											gui::Text("Moving %s", entry.path().filename().string().c_str());
											gui::EndDragDropSource();
										}

										gui::PopStyleVar();
										gui::PopStyleColor();

										gui::PushTextWrapPos(gui::GetCursorPos().x + buttonSize);
										gui::TextWrapped(entry.path().filename().string().c_str());
										gui::PopTextWrapPos();
										gui::EndGroup();

										ui::CenterNextWindow();
										if (gui::BeginPopupModal(("Confirm##" + entry.path().string()).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
										{
											gui::Text("Are you sure you want to load this scene? -> %s", entry.path().filename().string().c_str());
											const float widgetWidth = gui::GetItemRectSize().x;

											if (gui::Button("Yes##Confirm") || gui::IsKeyPressed(ImGuiKey_Enter))
											{
												m_ScenePath = entry.path().string();
												m_Scene.Load(m_ScenePath);
												gui::CloseCurrentPopup();
											}

											gui::SameLine(widgetWidth - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
											if (gui::Button("Cancel##Confirm") || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();

											gui::EndPopup();
										}
									}
								}
								i++;
							}
						}
					}
					gui::EndChild();

					gui::EndTable();
				}
			}
			else
			{
				// Display directories without table
				gui::BeginChild("FoldersChild##2", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
				displayDirectory(assetsPath);
				gui::EndChild();
			}
		}
		gui::End();

		// Handle opened files (tabs or whatever content needs to be displayed)
		for (auto it = openedFiles.begin(); it != openedFiles.end();)
		{
			bool open = true;
			if (gui::Begin(it->filename().string().c_str(), &open))
				gui::Text("File: %s", it->string().c_str());
			gui::End();

			if (!open)
			{
				openedFileNames.erase(it->string());
				it = openedFiles.erase(it);
			}
			else
				++it;
		}
	}

	void UI_DebugStats()
	{
		if (gui::Begin("Debug Stats", &showDebugStats, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize))
		{
			// Draw background if docked, if not, only tint
			ImGuiDockNode* dockNode = gui::GetWindowDockNode();
			if (!dockNode) // Window is floating
			{
				gui::GetWindowDrawList()->AddRectFilled(gui::GetWindowPos(), { gui::GetWindowPos().x + gui::GetWindowSize().x, gui::GetWindowPos().y + gui::GetWindowSize().y }, IM_COL32(20, 20, 20, 60));
			}
			else // Window is docked
			{
				ImVec4 bgColor = gui::GetStyleColorVec4(ImGuiCol_WindowBg);
				gui::GetWindowDrawList()->AddRectFilled(gui::GetWindowPos(),
					{ gui::GetWindowPos().x + gui::GetWindowSize().x, gui::GetWindowPos().y + gui::GetWindowSize().y },
					gui::ColorConvertFloat4ToU32(bgColor));
			}

			uint32_t fps = 1.f / Globals.deltaTime;
			ui::Text(std::format("Frame time: {:.4f}ms", Globals.deltaTime * 1000.f));
			ui::Text(std::format("FPS: {}", fps));
			ui::Text(std::format("Max FPS: {}", m_PrevMaxFPS));
			ui::Text(std::format("Min FPS: {}", m_PrevMinFPS));
			ui::Text(std::format("Average FPS: {}", m_PrevFrameCounter));

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
		gui::End();
	}

	void UI_StyleEditor(ImGuiStyle* ref = nullptr)
	{
		if (gui::Begin("Style editor", &showStyleEditor))
		{
			// You can pass in a reference ImGuiStyle structure to compare to, revert to and save to
			// (without a reference style pointer, we will use one compared locally as a reference)
			ImGuiStyle& style = gui::GetStyle();
			static ImGuiStyle ref_saved_style;

			// Default to using internal storage as reference
			static bool init = true;
			if (init && ref == NULL)
				ref_saved_style = style;
			init = false;
			if (ref == NULL)
				ref = &ref_saved_style;

			gui::PushItemWidth(gui::GetWindowWidth() * 0.50f);

			// Save/Revert button
			if (gui::Button("Save"))
			{
				auto filepath = FileDialogs::SaveFile(Globals.window, "Image (*.style)\0*.style\0");

				if (!filepath.empty())
				{
					std::ofstream file(filepath, std::ios::binary);

					if (file.is_open())
					{
						ImGuiStyle finalStyle = gui::GetStyle();
						//LastStyle = std::filesystem::relative(filepath).string();
						file.write((const char*)&finalStyle, sizeof(finalStyle));
						file.close();
					}
				}
			}
			gui::SameLine();
			if (gui::Button("Load"))
			{
				auto filepath = FileDialogs::OpenFile(Globals.window, "Image (*.style)\0*.style\0");

				if (!filepath.empty())
				{
					std::ifstream file(filepath, std::ios::binary);
					if (file.is_open())
					{
						ImGuiStyle& loadStyle = gui::GetStyle();
						//LastStyle = std::filesystem::relative(filepath).string();
						file.read((char*)&loadStyle, sizeof(loadStyle));
						file.close();
					}
				}
			}

			// Simplified Settings (expose floating-pointer border sizes as boolean representing 0.0f or 1.0f)
			if (ui::Slider("Frame rounding", style.FrameRounding, 0.0f, 12.0f, "%.0f"))
				style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
			{ bool border = (style.WindowBorderSize > 0.0f); if (ui::Checkbox("WindowBorder", border)) { style.WindowBorderSize = border ? 1.0f : 0.0f; } }
			gui::SameLine();
			{ bool border = (style.FrameBorderSize > 0.0f);  if (ui::Checkbox("FrameBorder", border)) { style.FrameBorderSize = border ? 1.0f : 0.0f; } }
			gui::SameLine();
			{ bool border = (style.PopupBorderSize > 0.0f);  if (ui::Checkbox("PopupBorder", border)) { style.PopupBorderSize = border ? 1.0f : 0.0f; } }


			ui::Separator();

			if (gui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
			{
				if (gui::BeginTabItem("Sizes"))
				{
					ui::Separator("Main");
					ui::Slider2("WindowPadding", (float*)(&style.WindowPadding), 0.0f, 20.0f, "%.0f");
					ui::Slider2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
					ui::Slider2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
					ui::Slider2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
					ui::Slider2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
					ui::Slider("IndentSpacing", style.IndentSpacing, 0.0f, 30.0f, "%.0f");
					ui::Slider("ScrollbarSize", style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
					ui::Slider("GrabMinSize", style.GrabMinSize, 1.0f, 20.0f, "%.0f");

					ui::Separator("Borders");
					ui::Slider("WindowBorderSize", style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
					ui::Slider("ChildBorderSize", style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
					ui::Slider("PopupBorderSize", style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
					ui::Slider("FrameBorderSize", style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
					ui::Slider("TabBorderSize", style.TabBorderSize, 0.0f, 1.0f, "%.0f");
					ui::Slider("TabBarBorderSize", style.TabBarBorderSize, 0.0f, 2.0f, "%.0f");

					ui::Separator("Rounding");
					ui::Slider("WindowRounding", style.WindowRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("ChildRounding", style.ChildRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("FrameRounding", style.FrameRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("PopupRounding", style.PopupRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("ScrollbarRounding", style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("GrabRounding", style.GrabRounding, 0.0f, 12.0f, "%.0f");
					ui::Slider("TabRounding", style.TabRounding, 0.0f, 12.0f, "%.0f");

					ui::Separator("Tables");
					ui::Slider2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
					gui::SliderAngle("TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);

					ui::Separator("Widgets");
					ui::Slider2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
					auto window_menu_button_position = style.WindowMenuButtonPosition + 1;
					if (gui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
						style.WindowMenuButtonPosition = (ImGuiDir)(window_menu_button_position - 1);
					gui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
					ui::Slider2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
					gui::SameLine(); ui::HelpMarker("Alignment applies when a button is larger than its text content.");
					ui::Slider2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
					gui::SameLine(); ui::HelpMarker("Alignment applies when a selectable is larger than its text content.");
					ui::Slider("SeparatorTextBorderSize", style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
					ui::Slider2("SeparatorTextAlign", (float*)&style.SeparatorTextAlign, 0.0f, 1.0f, "%.2f");
					ui::Slider2("SeparatorTextPadding", (float*)&style.SeparatorTextPadding, 0.0f, 40.0f, "%.0f");
					ui::Slider("LogSliderDeadzone", style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");

					ui::Separator("Docking");
					ui::Slider("DockingSplitterSize", style.DockingSeparatorSize, 0.0f, 12.0f, "%.0f");

					gui::SeparatorText("Tooltips");
					for (int n = 0; n < 2; n++)
						if (gui::TreeNodeEx(n == 0 ? "HoverFlagsForTooltipMouse" : "HoverFlagsForTooltipNav"))
						{
							ImGuiHoveredFlags* p = (n == 0) ? &style.HoverFlagsForTooltipMouse : &style.HoverFlagsForTooltipNav;
							gui::CheckboxFlags("ImGuiHoveredFlags_DelayNone", p, ImGuiHoveredFlags_DelayNone);
							gui::CheckboxFlags("ImGuiHoveredFlags_DelayShort", p, ImGuiHoveredFlags_DelayShort);
							gui::CheckboxFlags("ImGuiHoveredFlags_DelayNormal", p, ImGuiHoveredFlags_DelayNormal);
							gui::CheckboxFlags("ImGuiHoveredFlags_Stationary", p, ImGuiHoveredFlags_Stationary);
							gui::CheckboxFlags("ImGuiHoveredFlags_NoSharedDelay", p, ImGuiHoveredFlags_NoSharedDelay);
							gui::TreePop();
						}

					gui::SeparatorText("Misc");
					ui::Slider2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f"); gui::SameLine(); ui::HelpMarker("Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");

					gui::EndTabItem();
				}

				if (gui::BeginTabItem("Colors"))
				{
					static ImGuiTextFilter filter;
					filter.Draw("Filter colors", gui::GetFontSize() * 16);

					static ImGuiColorEditFlags alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf;
					if (gui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None)) { alpha_flags = ImGuiColorEditFlags_None; } gui::SameLine();
					if (gui::RadioButton("Alpha", alpha_flags == ImGuiColorEditFlags_AlphaPreview)) { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } gui::SameLine();
					if (gui::RadioButton("Both", alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } gui::SameLine();
					ui::HelpMarker(
						"In the color list:\n"
						"Left-click on color square to open color picker,\n"
						"Right-click to open edit options menu.");

					gui::SetNextWindowSizeConstraints(ImVec2(0.0f, gui::GetTextLineHeightWithSpacing() * 10), ImVec2(FLT_MAX, FLT_MAX));
					gui::BeginChild("##colors", ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
					gui::PushItemWidth(gui::GetFontSize() * -12);
					for (int i = 0; i < ImGuiCol_COUNT; i++)
					{
						const char* name = gui::GetStyleColorName(i);
						if (!filter.PassFilter(name))
							continue;
						gui::PushID(i);
						gui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
						if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
						{
							// Tips: in a real user application, you may want to merge and use an icon font into the main font,
							// so instead of "Save"/"Revert" you'd use icons!
							// Read the FAQ and docs/FONTS.md about using icon fonts. It's really easy and super convenient!
							gui::SameLine(0.0f, style.ItemInnerSpacing.x); if (gui::Button("Save")) { ref->Colors[i] = style.Colors[i]; }
							gui::SameLine(0.0f, style.ItemInnerSpacing.x); if (gui::Button("Revert")) { style.Colors[i] = ref->Colors[i]; }
						}
						gui::SameLine(0.0f, style.ItemInnerSpacing.x);
						gui::TextUnformatted(name);
						gui::PopID();
					}
					gui::PopItemWidth();
					gui::EndChild();

					gui::EndTabItem();
				}

				if (gui::BeginTabItem("Rendering"))
				{
					ui::Checkbox("Anti-aliased lines", style.AntiAliasedLines);
					gui::SameLine();
					ui::HelpMarker("When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

					ui::Checkbox("Anti-aliased lines use texture", style.AntiAliasedLinesUseTex);
					gui::SameLine();
					ui::HelpMarker("Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

					ui::Checkbox("Anti-aliased fill", style.AntiAliasedFill);
					gui::PushItemWidth(gui::GetFontSize() * 8);
					ui::Drag("Curve Tessellation Tolerance", style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
					if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

					// When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
					ui::Drag("Circle Tessellation Max Error", style.CircleTessellationMaxError, 0.005f, 0.10f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
					const bool show_samples = gui::IsItemActive();
					if (show_samples)
						gui::SetNextWindowPos(gui::GetCursorScreenPos());
					if (show_samples && gui::BeginTooltip())
					{
						gui::TextUnformatted("(R = radius, N = number of segments)");
						gui::Spacing();
						ImDrawList* draw_list = gui::GetWindowDrawList();
						const float min_widget_width = gui::CalcTextSize("N: MMM\nR: MMM").x;
						for (int n = 0; n < 8; n++)
						{
							const float RAD_MIN = 5.0f;
							const float RAD_MAX = 70.0f;
							const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * (float)n / (8.0f - 1.0f);

							gui::BeginGroup();

							gui::Text("R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

							const float canvas_width = std::max(min_widget_width, rad * 2.0f);
							const float offset_x = floorf(canvas_width * 0.5f);
							const float offset_y = floorf(RAD_MAX);

							const ImVec2 p1 = gui::GetCursorScreenPos();
							draw_list->AddCircle(ImVec2(p1.x + offset_x, p1.y + offset_y), rad, gui::GetColorU32(ImGuiCol_Text));
							gui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));

							/*
							const ImVec2 p2 = gui::GetCursorScreenPos();
							draw_list->AddCircleFilled(ImVec2(p2.x + offset_x, p2.y + offset_y), rad, gui::GetColorU32(ImGuiCol_Text));
							gui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));
							*/

							gui::EndGroup();
							gui::SameLine();
						}
						gui::EndTooltip();
					}
					gui::SameLine();
					ui::HelpMarker("When drawing circle primitives with \"num_segments == 0\" tesselation will be calculated automatically.");

					ui::Drag("Global Alpha", style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
					ui::Drag("Disabled Alpha", style.DisabledAlpha, 0.005f, 0.0f, 1.0f, "%.2f"); gui::SameLine(); ui::HelpMarker("Additional alpha multiplier for disabled items (multiply over current value of Alpha).");
					gui::PopItemWidth();

					gui::EndTabItem();
				}

				gui::EndTabBar();
			}

			gui::PopItemWidth();

			gui::End();
		}
	}

	void WindowButtons() const
	{
		// Buttons
		gui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		gui::PushStyleColor(ImGuiCol_Button, gui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
		gui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
		gui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

		//get frame height gets font size
		gui::PushFont(Globals.f_Default.Menu);
		float buttonSize = gui::GetFrameHeightWithSpacing() - 16;
		gui::PopFont();

		gui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
		gui::SetCursorPosX(gui::GetWindowSize().x - (buttonSize + gui::GetStyle().FramePadding.x * 2) * 3);
		if (gui::ImageButton("collapse", t_Collapse, { buttonSize, buttonSize }))
			Globals.window.Minimize();

		gui::SameLine(0, 0);
		if (gui::ImageButton("minimize", t_Minimize, { buttonSize, buttonSize }))
			Globals.window.SetMaximized(!Globals.window.IsMaximized());

		gui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.25f, 0.2f, 1.f));
		gui::SameLine(0, 0);
		static bool showCloseWarn = false;
		if (gui::ImageButton("close", t_Close, { buttonSize, buttonSize }))
		{
			if (Project::Exists())
				showCloseWarn = true;
			else
				Globals.window.Close();
		}
		if (showCloseWarn)
		{
			gui::OpenPopup("Close Project Warning");
			showCloseWarn = false;
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(2);
		ui::CenterNextWindow();
		if (gui::BeginPopupModal("Close Project Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ui::Text("Are you sure you want to EXIT?");
			ui::Text("Unsaved changes will be lost!");
			if (gui::Button("Close", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter))
			{
				Globals.window.Close();
				gui::CloseCurrentPopup();
			}
			gui::SameLine(gui::CalcTextSize("Unsaved changes will be lost!").x - gui::GetContentRegionMax().x * 0.3f + gui::GetStyle().ItemSpacing.x);
			if (gui::Button("Cancel", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape))
				gui::CloseCurrentPopup();

			gui::EndPopup();
		}

		// Drag when hovering tab bar
		//@TODO - Maybe Reposition this
		static bool dragging = false;
		glm::vec2 mousePos = Globals.window.GetCursorPos() + Globals.window.GetPosition();
		static glm::vec2 mouseCur = {};

		if (ImGui::IsMouseClicked(0) && Globals.window.GetCursorPos().y <= gui::GetFrameHeightWithSpacing() && !ImGui::IsAnyItemHovered() && !Globals.window.IsMaximized())
		{
			// TODO - FIX: check if not hovering edge
			bool top = (Globals.window.GetCursorPos().y >= -Globals.window.borderSize) && (Globals.window.GetCursorPos().y <= Globals.window.borderSize);
			bool left = (Globals.window.GetCursorPos().x >= -Globals.window.borderSize) && (Globals.window.GetCursorPos().x <= Globals.window.borderSize);
			bool right = (Globals.window.GetCursorPos().x >= Globals.window.GetSize().x - Globals.window.borderSize) && (Globals.window.GetCursorPos().x <= Globals.window.GetSize().x + Globals.window.borderSize);

			if (!top && !left && !right)
			{
				dragging = true;
				mouseCur = Globals.window.GetCursorPos();
			}
		}
		if (dragging && ImGui::IsMouseDown(0))
		{
			Globals.window.SetPosition({ mousePos.x - mouseCur.x, mousePos.y - mouseCur.y });
		}
		if (ImGui::IsMouseReleased(0)) dragging = false;
	}

	void UI()
	{
		const ImGuiViewport* viewport = gui::GetMainViewport();
		gui::SetNextWindowPos(viewport->WorkPos);
		gui::SetNextWindowSize(viewport->WorkSize);
		gui::SetNextWindowViewport(viewport->ID);

		gui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 7.0f));
		gui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		gui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, Project::Exists() ? ImVec2(0.f, 0.f) : ImVec2(50.f, 50.f));

		static ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		if (gui::Begin("DockSpace", nullptr, windowFlags))
		{
			if (!Project::Exists())
			{
				gui::PopStyleVar(4);
				windowFlags &= ~ImGuiWindowFlags_NoBackground;
				//windowFlags &= ~ImGuiWindowFlags_MenuBar;

				if (gui::BeginMenuBar())
				{
					WindowButtons();
					gui::EndMenuBar();
				}

				static bool openProjNamePopup = false; // New persistent flag

				gui::PushFont(Globals.f_Default.Big);
				if (gui::Button("New Project"))
				{
					gui::OpenPopup("New Project");
				}
				gui::PopFont();

				// File dialog logic
				std::string newProjectPath = ui::FileDialog("New Project", ".");
				static std::string newProjectSavePath;
				if (!newProjectPath.empty())
				{
					newProjectSavePath = newProjectPath;
					openProjNamePopup = true;
				}

				if (openProjNamePopup)
				{
					gui::OpenPopup("New Project - Name");
					openProjNamePopup = false;
				}

				ui::CenterNextWindow();
				if (gui::BeginPopupModal("New Project - Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					static std::string projName = "Untitled";
					if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
					gui::InputText("Project Name", &projName);
					const float widgetWidth = gui::GetItemRectSize().x;

					gui::BeginDisabled(projName.empty());
					if (gui::Button("OK", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter))
					{
						if (Project::ExistListProject(projName))
						{
							gui::OpenPopup("Project Already Exists");
						}
						else
						{
							Project::Create(newProjectSavePath, projName);
							newProjectSavePath.clear();
							openProjNamePopup = false;
							gui::CloseCurrentPopup();
						}
					}
					gui::EndDisabled();
					if (projName.empty())gui::SetItemTooltip("Project name cannot be empty!");

					gui::SameLine(widgetWidth - gui::GetContentRegionMax().x * 0.3f + gui::GetStyle().ItemSpacing.x);
					if (gui::Button("Cancel", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape))
					{
						projName = "Untitled";
						newProjectSavePath.clear();
						openProjNamePopup = false;
						gui::CloseCurrentPopup();
					}

					ui::CenterNextWindow();
					if (gui::BeginPopupModal("Project Already Exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
					{
						gui::Text("Project with this name already exists!");
						if (gui::Button("OK", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();
						gui::EndPopup();
					}

					gui::EndPopup();
				}

				gui::SameLine();

				gui::PushFont(Globals.f_Default.Big);
				if (gui::Button("Open Project"))
				{
					gui::OpenPopup("Open Project");
				}
				gui::PopFont();
				std::string openProjectPath = ui::FileDialog("Open Project", ".");
				if (!openProjectPath.empty())
				{
					Project::Load(openProjectPath);
				}

				gui::Separator();

				if (gui::BeginChild("Project Display", ImVec2(0, 0)))
				{
					for (auto project : Project::savedProjectPaths)
					{
						gui::PushFont(Globals.f_Default.Big);
						std::filesystem::path path = project;
						if (std::filesystem::exists(path))
						{
							if (gui::Button((path.filename().string() + "##" + path.string()).c_str()))
							{
								Project::Load(project);
							}

							gui::SameLine(0, 100);
							gui::Text("FullPath: %s", project.c_str());
							gui::SameLine();
							if (gui::Button(("Delete##" + path.string()).c_str()))
							{
								gui::OpenPopup(("Delete Project##" + project).c_str());
							}
						}
						else WC_CORE_WARN("Project path does not exist: {0}", project);
						gui::PopFont();

						ui::CenterNextWindow();
						if (gui::BeginPopupModal(("Delete Project##" + project).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
						{
							gui::Text("Are you sure you want to delete this project?");
							if (gui::Button("Yes##Delete", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter))
							{
								Project::Delete(project);
								gui::CloseCurrentPopup();
							}

							gui::SameLine(gui::CalcTextSize("Are you sure you want to delete this project?").x - gui::GetContentRegionAvail().x * 0.3f + gui::GetStyle().ItemSpacing.x);
							if (gui::Button("No##Delete", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();

							gui::EndPopup();
						}
					}
				}
				gui::EndChild();
			}
			else
			{
				gui::PopStyleVar(4);
				windowFlags |= ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground;

				ImGuiIO& io = gui::GetIO();
				if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
				{
					ImGuiID dockspace_id = gui::GetID("MainDockSpace");
					gui::DockSpace(dockspace_id, ImVec2(0.f, 0.f));
				}

				if (gui::BeginMenuBar())
				{
					// TODO - add Dragging and Turn of GLFW tab bar -> make custom / get from The Cherno
					if (gui::IsWindowHovered() && !gui::IsAnyItemHovered())
					{
						//WC_CORE_INFO("Empty space on main menu bar is hovered)"
					}

					if (ui::BeginMenuFt(("[" + Project::name + "]").c_str(), Globals.f_Default.Menu))
					{
						if (gui::MenuItem("Change", "CTRL + P")) Project::Reset();

						// Delete - TODO: FIX IF NEEDED
						/*ImGui::Separator();
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.f, 0.f, 0.f, 1.f));
						if (ui::MenuItemButton("Delete")) gui::OpenPopup("Delete Project");
						ImGui::PopStyleColor();

						if (gui::BeginPopupModal("Delete Project"))
						{
							gui::Text("Are you sure you want to delete this project?");
							if (gui::Button("Yes"))
							{
								const std::string path = Project::rootPath;


								gui::CloseCurrentPopup();
							}
							gui::SameLine(gui::GetContentRegionAvail().x - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
							if (gui::Button("Cancel")) gui::CloseCurrentPopup();
							gui::EndPopup();
						}*/

						gui::EndMenu();
					}

					gui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

					if (ui::BeginMenuFt("File", Globals.f_Default.Menu))
					{
						gui::SeparatorText("Scene");

						static bool openSceneNamePopup = false;
						if (ui::MenuItemButton("New", "CTRL + N", false)) gui::OpenPopup("New Scene");

						std::string newScenePath = ui::FileDialog("New Scene", ".", Project::rootPath);
						static std::string newSceneSavePath;
						if (!newScenePath.empty())
						{
							newSceneSavePath = newScenePath;
							openSceneNamePopup = true;
						}

						if (openSceneNamePopup)
						{
							gui::OpenPopup("New Scene - Name");
							openSceneNamePopup = false;
						}

						ui::CenterNextWindow();
						if (gui::BeginPopupModal("New Scene - Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
						{
							static std::string sceneName = "newScene";
							if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
							gui::InputText("Scene Name", &sceneName);
							const float widgetWidth = gui::GetItemRectSize().x;

							gui::BeginDisabled(sceneName.empty());
							if (gui::Button("OK", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter))
							{
								m_Scene.Destroy();
								m_ScenePath = newSceneSavePath + '/' + sceneName + ".scene";
								m_SelectedEntity = flecs::entity::null();

								sceneName = "newScene";
								newSceneSavePath.clear();
								openSceneNamePopup = false;
								gui::CloseCurrentPopup();
							}
							gui::EndDisabled();
							if (sceneName.empty()) gui::SetItemTooltip("Scene name cannot be empty!");

							gui::SameLine(widgetWidth - gui::GetContentRegionAvail().x * 0.3f + gui::GetStyle().ItemSpacing.x);
							if (gui::Button("Cancel", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape))
							{
								sceneName = "newScene";
								newSceneSavePath.clear();
								openSceneNamePopup = false;
								gui::CloseCurrentPopup();
							}

							gui::EndPopup();
						}

						if (ui::MenuItemButton("Open", "CTRL + O", false))
						{
							gui::OpenPopup("Open Scene");
						}

						std::string sOpenPath = ui::FileDialog("Open Scene", ".scene", Project::rootPath);
						if (!sOpenPath.empty())
						{
							m_ScenePath = sOpenPath;
							m_Scene.Load(m_ScenePath);
							gui::CloseCurrentPopup();
						}

						gui::SeparatorText("File");

						if (gui::MenuItem("Save", "CTRL + S"))
						{
							m_Scene.Save(m_ScenePath);
						}

						if (gui::MenuItem("Save As", "CTRL + A + S"))
						{
							m_Scene.Save(m_ScenePath);
						}

						if (gui::MenuItem("Undo", "CTRL + Z"))
						{
							WC_INFO("Undo");
						}

						if (gui::MenuItem("Redo", "CTRL + Y"))
						{
							WC_INFO("Redo");
						}

						gui::EndMenu();
					}

					if (ui::BeginMenuFt("View", Globals.f_Default.Menu))
					{
						gui::MenuItem("Editor", nullptr, &showEditor);
						gui::MenuItem("Scene properties", nullptr, &showSceneProperties);
						gui::MenuItem("Entities", nullptr, &showEntities);
						gui::MenuItem("Properties", nullptr, &showProperties);
						gui::MenuItem("Console", nullptr, &showConsole);
						gui::MenuItem("File Explorer", nullptr, &showFileExplorer);
						gui::MenuItem("Debug Statistics", nullptr, &showDebugStats);
						gui::MenuItem("Style Editor", nullptr, &showStyleEditor);

						if (gui::BeginMenu("Theme"))
						{
							// TODO - Add more themes / custom themes / save themes
							static const char* themes[] = { "SoDark", "Classic", "Dark", "Light" };
							static int currentTheme = 0;
							static float hue = 0.0f;

							if (gui::Combo("Select Theme", &currentTheme, themes, IM_ARRAYSIZE(themes)))
							{
								switch (currentTheme)
								{
								case 0:
									gui::GetStyle() = ui::SoDark(hue);
									break;
								case 1:
									gui::StyleColorsClassic();
									break;
								case 2:
									gui::StyleColorsDark();
									break;
								case 3:
									gui::StyleColorsLight();
									break;
								}
							}

							if (currentTheme == 0 && gui::SliderFloat("Hue", &hue, 0.0f, 1.0f))
							{
								gui::GetStyle() = ui::SoDark(hue);
							}

							gui::EndMenu();
						}

						gui::EndMenu();
					}

					WindowButtons();

					gui::EndMenuBar();
				}

				//Display tabs - check project again because we can change state with menu bar (inside this else-case)
				if (Project::Exists())
				{
					if (showEditor) UI_Editor();
					if (showSceneProperties) UI_SceneProperties();
					if (showEntities) UI_Entities();
					if (showProperties) UI_Properties();
					if (showConsole) UI_Console();
					if (showFileExplorer) UI_Assets();
					if (showDebugStats) UI_DebugStats();
					if (showStyleEditor) UI_StyleEditor();
				}
			}
		}
		gui::End();
	}
};
