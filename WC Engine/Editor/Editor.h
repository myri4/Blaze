#pragma once

#include <fstream>

#include <glm/gtc/type_ptr.hpp>

#include <wc/Utils/List.h>
#include <wc/Utils/FileDialogs.h>

#include "Rendering/Renderer2D.h"

#include "EditorScene.h"

#include "../Globals.h"

using namespace Editor;
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

#define PROJECT_SETTINGS_EXTENSION ".blzproj"
#define PROJECT_USER_SETTINGS_EXTENSION ".blzproj.user"

namespace Project
{
	
}


struct EditorInstance
{
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

	RenderData m_RenderData[FRAME_OVERLAP];
	Renderer2D m_Renderer;

	b2DebugDraw m_PhysicsDebugDraw;

    // Window Buttons
	Texture t_Close;
	Texture t_Minimize;
    Texture t_Maximize;
	Texture t_Collapse;

    // Assets
	Texture t_FolderOpen;
	Texture t_FolderClosed;
	Texture t_File;

    // Scene Editor Buttons
	Texture t_Play;
	Texture t_Simulate;
	Texture t_Stop;

    // Entities
    Texture t_Eye;
    Texture t_EyeClosed;

    // Console
    Texture t_Debug;
    Texture t_Info;
    Texture t_Warning;
    Texture t_Error;
    Texture t_Critical;


	bool allowInput = true;

	bool showEditor = true;
	bool showSceneProperties = true;
	bool showEntities = true;
	bool showProperties = true;
	bool showConsole = true;
	bool showAssets = true;
	bool showDebugStats = false;
	bool showStyleEditor = false;

	EditorScene m_Scene;

	void Create()
	{
		LoadSettings();

		assetManager.Init();

		m_Renderer.Init();

		// Load Textures
		std::string assetPath = "assets/textures/menu/";
		t_Close.Load(assetPath + "close.png");
		t_Minimize.Load(assetPath + "minimize.png");
	    t_Maximize.Load(assetPath + "maximize.png");
		t_Collapse.Load(assetPath + "collapse.png");

		t_FolderOpen.Load(assetPath + "folder-open.png");
		t_FolderClosed.Load(assetPath + "folder-closed.png");
		t_File.Load(assetPath + "file.png");

		t_Play.Load(assetPath + "play.png");
		t_Simulate.Load(assetPath + "simulate.png");
		t_Stop.Load(assetPath + "stop.png");

		t_Eye.Load(assetPath + "eye.png");
		t_EyeClosed.Load(assetPath + "eye-slash.png");

	    t_Debug.Load(assetPath + "debug.png");
	    t_Info.Load(assetPath + "info.png");
	    t_Warning.Load(assetPath + "warning.png");
	    t_Error.Load(assetPath + "error.png");
	    t_Critical.Load(assetPath + "critical.png");

		PhysicsMaterials.emplace_back(PhysicsMaterial()); // @NOTE: Index 0 is the default material
		PhysicsMaterialNames["Default"] = PhysicsMaterials.size() - 1;

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
	}

	void Resize(glm::vec2 size)
	{
		m_Renderer.DestroyScreen();
		m_Renderer.CreateScreen(size);
		m_Scene.camera.Update(m_Renderer.GetAspectRatio());
	}

	void Destroy()
	{
		SaveSettings();

		SavePhysicsMaterials(ProjectRootPath + "\\physicsMaterials.yaml");

		t_Close.Destroy();
		t_Minimize.Destroy();
	    t_Maximize.Destroy();
		t_Collapse.Destroy();

		t_FolderClosed.Destroy();
		t_FolderOpen.Destroy();
		t_File.Destroy();

		t_Play.Destroy();
		t_Simulate.Destroy();
		t_Stop.Destroy();

		t_Eye.Destroy();
		t_EyeClosed.Destroy();

	    t_Debug.Destroy();
	    t_Info.Destroy();
	    t_Warning.Destroy();
	    t_Error.Destroy();
	    t_Critical.Destroy();

		assetManager.Free();
		m_Renderer.Deinit();

		for (int i = 0; i < FRAME_OVERLAP; i++)
			m_RenderData[i].Free();
	}

	void Input()
	{
		if (allowInput)
		{
			auto& GuizmoOp = m_Scene.GuizmoOp;
			auto& camera = m_Scene.camera;
			if (gui::IsKeyPressed(ImGuiKey_G)) GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y | ImGuizmo::OPERATION::TRANSLATE_Z;
			else if (gui::IsKeyPressed(ImGuiKey_R)) GuizmoOp = ImGuizmo::OPERATION::ROTATE_Z;
			else if (gui::IsKeyPressed(ImGuiKey_S)) GuizmoOp = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Y;

			if (gui::IsKeyDown(ImGuiKey_LeftCtrl))
			{
				if (gui::IsKeyPressed(ImGuiKey_Z)) m_Scene.Undo();
				else if (gui::IsKeyPressed(ImGuiKey_Y)) m_Scene.Redo();
				else if (gui::IsKeyPressed(ImGuiKey_S)) m_Scene.Save();
			}

			/*float scroll = Mouse::GetMouseScroll().y;
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
			}*/

			if (Key::GetKey(Key::LeftAlt))
			{
				const glm::vec2& mouse = Globals.window.GetCursorPos();
				glm::vec2 delta = (mouse - camera.m_InitialMousePosition) * 0.01f;
				camera.m_InitialMousePosition = mouse;

				if (Mouse::GetMouse(Mouse::LEFT))
				{
					auto panSpeed = camera.PanSpeed(m_Renderer.m_RenderSize);
					camera.FocalPoint += -camera.GetRightDirection() * delta.x * panSpeed.x * camera.m_Distance;
					camera.FocalPoint += camera.GetUpDirection() * delta.y * panSpeed.y * camera.m_Distance;
				}
				else if (Mouse::GetMouse(Mouse::RIGHT))
				{
					float yawSign = camera.GetUpDirection().y < 0 ? -1.f : 1.f;
					camera.Yaw += yawSign * delta.x * camera.RotationSpeed;
					camera.Pitch += delta.y * camera.RotationSpeed;
				}

				camera.UpdateView();
			}

			float scroll = Mouse::GetMouseScroll().y;
			if (scroll != 0.f)
			{
				float delta = scroll * 0.1f;
				{
					camera.m_Distance -= delta * camera.ZoomSpeed();
					if (camera.m_Distance < 1.f)
					{
						camera.FocalPoint += camera.GetForwardDirection();
						camera.m_Distance = 1.f;
					}
				}
				camera.UpdateView();
			}
		}
	}

	void RenderEntity(flecs::entity entt, glm::mat4& transform)
	{
	    if (entt.get<EntityTag>()->showEntity)
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
	        m_Scene.m_Scene.EntityWorld.query_builder<TransformComponent, EntityTag>()
                .with(flecs::ChildOf, entt)
                .each([&](flecs::entity child, TransformComponent childTransform, EntityTag)
                    {
                        transform = transform * childTransform.GetTransform();
                        RenderEntity(child, transform);
                    });
	    }
	}

	void Render()
	{
		if (!ProjectExists()) return;
		auto& renderData = m_RenderData[CURRENT_FRAME];

		if (m_Renderer.TextureCapacity < assetManager.Textures.size())
			m_Renderer.AllocateNewDescriptor(assetManager.Textures.capacity());

		if (assetManager.TexturesUpdated)
		{
			assetManager.TexturesUpdated = false;
			m_Renderer.UpdateTextures(assetManager);
		}

		m_Scene.m_Scene.EntityWorld.each([&](flecs::entity entt, TransformComponent& p) {
			if (entt.parent() != 0) return;

			glm::mat4 transform = p.GetTransform();
			RenderEntity(entt, transform);
			});

		if (m_Scene.State != SceneState::Edit)
		{
			m_PhysicsDebugDraw.context = &renderData;
			m_Scene.m_Scene.PhysicsWorld.Draw(&m_PhysicsDebugDraw);
		}

		m_Renderer.Flush(renderData, m_Scene.camera.GetViewProjectionMatrix());

		renderData.Reset();
	}

	void Update() {	m_Scene.Update(); }

	void UI_Editor()
	{
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

		if (gui::Begin("Editor", &showEditor))
		{
		    //gui::Spacing();

		    gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
		    if (gui::BeginTabBar("ScenesBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_DrawSelectedOverline | ImGuiTabBarFlags_Reorderable))
		    {
		        gui::TabItemSpacing("##TabSpacing", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_Invisible, 0.f);

		        ImGuiTabItemFlags flags = ImGuiTabBarFlags_NoTooltip;
		        gui::BeginDisabled(SceneState::Play == m_Scene.State || SceneState::Simulate == m_Scene.State);
		        for (const auto& scene : savedProjectScenes)
		        {
		            if (scene == m_Scene.Path) flags |= ImGuiTabItemFlags_SetSelected;
		            else flags &= ~ImGuiTabItemFlags_SetSelected;

		            bool open = true;
		            if (gui::BeginTabItem(std::filesystem::path(scene).stem().string().c_str(), &open, flags))
		            {
		                // Updates if open
		                gui::EndTabItem();
		            }

		            // Handle tab selection
		            if (gui::IsItemClicked())
		            {
		                if (true /*changesInCurrentScene*/)
		                {
		                    if (m_Scene.Path != scene) gui::OpenPopup(("Confirm##SceneChange" + scene).c_str());
		                }
		                /*else
		                {
		                    if (m_Scene.Path != scene)
		                    {
		                        //WC_INFO("Opening scene: {}", scene);
		                        m_Scene.Save(m_Scene.Path);
		                        m_Scene.Path = scene;
		                        m_Scene.Load(scene);
		                    }
		                }*/
		            }

		            gui::PopStyleVar(2);
		            ui::CenterNextWindow();
		            if (gui::BeginPopupModal(("Confirm##SceneChange" + scene).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		            {
		                gui::Text("Save changes to %s before changing", std::filesystem::path(m_Scene.Path).filename().string().c_str());
		                float textSize = gui::CalcTextSize("Save Changes to ").x + gui::CalcTextSize(std::filesystem::path(m_Scene.Path).filename().string().c_str()).x + gui::CalcTextSize(" before closing?").x;
		                float spacing = textSize * 0.05f;
		                if (gui::Button("Yes", {textSize * 0.3f, 0}) || gui::IsKeyPressed(ImGuiKey_Enter))
		                {
		                    //WC_CORE_INFO("Save scene before changing");
		                    //WC_INFO("Opening scene: {}", scene);
		                    m_Scene.Save();
		                    m_Scene.Path = scene;
		                    m_Scene.Load(scene);
		                    gui::CloseCurrentPopup();
		                }
		                gui::SameLine(0, spacing);
		                if (gui::Button("No", {textSize * 0.3f, 0}))
		                {
		                    //WC_INFO("Opening scene: {}", scene);
		                    m_Scene.Path = scene;
		                    m_Scene.Load(scene);
		                    gui::CloseCurrentPopup();
		                }
		                gui::SameLine(0, spacing);
		                if (gui::Button("Cancel", {textSize * 0.3f, 0}) || gui::IsKeyPressed(ImGuiKey_Escape))
		                {
		                    gui::CloseCurrentPopup();
		                }

		                gui::EndPopup();
		            }
		            gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		            gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

		            // Handle tab closure
		            if (!open)
		            {
		                //WC_INFO("Closing scene: {}", scene);
		                bool wasActive = (m_Scene.Path == scene);

		                if (wasActive)
		                {
		                    m_Scene.Save();
		                    m_Scene.Destroy();
		                }

		                RemoveSceneFromList(scene);

		                if (wasActive)
		                {
		                    if (!savedProjectScenes.empty())
		                    {
		                        m_Scene.Path = savedProjectScenes.front();
		                        m_Scene.Load(m_Scene.Path);
		                    }
		                    else
		                    {
		                        m_Scene.Path.clear();
		                    }
		                }
		            }
		        }
		        gui::EndDisabled();

		        gui::EndTabBar();
		    }
		    gui::PopStyleVar();

			allowInput = gui::IsWindowFocused() && gui::IsWindowHovered();

			ImVec2 viewPortSize = gui::GetContentRegionAvail();
			if (ViewPortSize != *((glm::vec2*)&viewPortSize))
			{
				ViewPortSize = { viewPortSize.x, viewPortSize.y };

				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize(ViewPortSize);
			}

			bool allowedSelect = !ImGuizmo::IsOver() && !ImGuizmo::IsUsing();
			//if (m_Scene.SelectedEntity == flecs::entity::null()) allowedSelect = true;
			if (!Key::GetKey(Key::LeftAlt) && Mouse::GetMouse(Mouse::LEFT) && allowedSelect && allowInput)
			{
				auto mousePos = (glm::ivec2)(Mouse::GetCursorPosition() - (glm::uvec2)WindowPos);

				uint32_t width = m_Renderer.m_RenderSize.x;
				uint32_t height = m_Renderer.m_RenderSize.y;

				if (mousePos.x > 0 && mousePos.y > 0 && mousePos.x < width && mousePos.y < height)
				{
					VulkanContext::GetLogicalDevice().WaitIdle();
					vk::StagingBuffer stagingBuffer;
					stagingBuffer.Allocate(sizeof(uint64_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT);
					vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
						auto image = m_Renderer.m_EntityImage;
						image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

						VkBufferImageCopy copyRegion = {
							.imageSubresource = {
								.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								.layerCount = 1,
							},
							.imageExtent = { 1, 1, 1 },
							.imageOffset = { mousePos.x, mousePos.y, 0},
						};

						vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &copyRegion);

						image.SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
						});

					auto imagedata = (uint64_t*)stagingBuffer.Map();
					auto id = imagedata[0];
					m_Scene.SelectedEntity = flecs::entity(m_Scene.m_Scene.EntityWorld, id);

					stagingBuffer.Unmap();
					stagingBuffer.Free();
				}
			}

		    auto viewportOffset = gui::GetWindowPos();
		    ImVec2 cursorPos = gui::GetCursorPos(); // Position relative to window's content area
		    ImVec2 availSize = gui::GetContentRegionAvail(); // Available region size

		    ImVec2 viewportBounds[2];
		    // Calculate absolute screen position of the available region's start and end
		    viewportBounds[0] = ImVec2(viewportOffset.x + cursorPos.x, viewportOffset.y + cursorPos.y);
		    viewportBounds[1] = ImVec2(viewportBounds[0].x + availSize.x, viewportBounds[0].y + availSize.y);

			WindowPos = *((glm::vec2*)&viewportBounds[0]);
			glm::vec2 RenderSize = *((glm::vec2*)&viewportBounds[1]) - WindowPos;

			gui::GetWindowDrawList()->AddImage((ImTextureID)m_Renderer.ImguiImageID, ImVec2(WindowPos.x, WindowPos.y), ImVec2(WindowPos.x + RenderSize.x, WindowPos.y + RenderSize.y));

			gui::SetCursorPosX((gui::GetWindowSize().x - 60 + gui::GetStyle().ItemSpacing.x * 2) * 0.5f);
			bool isPlayingOrSimulating = (m_Scene.State == SceneState::Play || m_Scene.State == SceneState::Simulate);
			bool isPaused = (m_Scene.State == SceneState::Edit);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
			if (isPlayingOrSimulating) gui::BeginDisabled();
			if (gui::ImageButton("play", t_Play, { 10, 10 }) && isPaused) m_Scene.SetState(SceneState::Play); gui::SameLine(0, 0); gui::SeparatorEx(ImGuiSeparatorFlags_Vertical); gui::SameLine(0, 0);

			if (gui::ImageButton("simulate", t_Simulate, { 10, 10 }) && isPaused) m_Scene.SetState(SceneState::Simulate); gui::SameLine(0, 0); gui::SeparatorEx(ImGuiSeparatorFlags_Vertical); gui::SameLine(0, 0);
			if (isPlayingOrSimulating) gui::EndDisabled();

			if (isPaused) gui::BeginDisabled();
			if (gui::ImageButton("stop", t_Stop, { 10, 10 })) m_Scene.SetState(SceneState::Edit);
			if (isPaused) gui::EndDisabled();
			ImGui::PopStyleVar();

			glm::mat4 projection = m_Scene.camera.Projection;
			projection[1][1] *= -1;

			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(WindowPos.x, WindowPos.y, RenderSize.x, RenderSize.y);
			if (m_Scene.SelectedEntity != flecs::entity::null() && m_Scene.SelectedEntity.has<TransformComponent>())
			{
				glm::mat4 local_transform = m_Scene.SelectedEntity.get<TransformComponent>()->GetTransform();
				glm::mat4 world_transform = local_transform;
				glm::mat4 deltaMatrix;

				// Build the world transform by accumulating parent transforms
				auto parent = m_Scene.SelectedEntity.parent();
				while (parent != flecs::entity::null() && parent.has<TransformComponent>())
				{
					glm::mat4 parent_transform = parent.get<TransformComponent>()->GetTransform();
					world_transform = parent_transform * world_transform;
					parent = parent.parent();
				}

				static bool wasUsingGuizmo = false;
				if (ImGuizmo::Manipulate(
					glm::value_ptr(m_Scene.camera.ViewMatrix),
					glm::value_ptr(projection),
					m_Scene.GuizmoOp,
					ImGuizmo::MODE::WORLD,
					glm::value_ptr(world_transform),
					glm::value_ptr(deltaMatrix)
				))
				{
					wasUsingGuizmo = true;
					// Convert world transform back to local space
					glm::mat4 parent_world_transform(1.f);
					parent = m_Scene.SelectedEntity.parent();
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
					m_Scene.SelectedEntity.set<TransformComponent>({
						glm::vec3(translation),
						glm::vec2(scale),
						rotation.z
						});

					if (m_Scene.SelectedEntity.has<RigidBodyComponent>())
					{
						auto body = m_Scene.SelectedEntity.get_ref<RigidBodyComponent>()->body;
						if (body.IsValid())
						{
							body.SetLinearVelocity({ 0.f, 0.f });
							body.SetAngularVelocity(0.f);
							body.SetTransform(translation, b2MakeRot(rotation.z));
						}
					}
				}
				else
				{
					if (wasUsingGuizmo)
						m_Scene.ChangeTransform(m_Scene.SelectedEntity);

					wasUsingGuizmo = false;
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
				auto& worldData = m_Scene.m_Scene.PhysicsWorldData;
				ui::DragButton2("Gravity", worldData.Gravity);

				ui::Drag("Near", m_Scene.camera.NearClip, 0.1f);
				ui::Drag("Far", m_Scene.camera.FarClip, 0.1f);
				ui::Drag3("Camera position", glm::value_ptr(m_Scene.camera.Position));
				ui::Drag3("Focal point", glm::value_ptr(m_Scene.camera.FocalPoint));
				m_Scene.camera.Update(m_Renderer.GetAspectRatio());
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

	void EntityRightClickMenu(const flecs::entity& entity)
	{
		// Display the popup menu
		gui::PopStyleVar(); // pop window padding
		if (gui::BeginPopup(std::to_string(entity.id()).c_str(), ImGuiWindowFlags_NoSavedSettings))
		{
			gui::Text("%s", entity.name().c_str());
			gui::Separator();

			if (gui::MenuItem("Clone"))
			{
				m_Scene.DuplicateEntity(entity);
				gui::CloseCurrentPopup();
			}

			if (ui::MenuItemButton("Export"))
			{
				gui::OpenPopup("Export Entity");
			}
			std::string exportPath = ui::FileDialog("Export Entity", ".", ProjectRootPath);
			if (!exportPath.empty())
			{
				YAML::Node entityData = m_Scene.ExportEntity(entity); // TODO - fix with merge
				YAMLUtils::SaveFile(exportPath + "\\" + std::string(entity.name().c_str()) + ".blzent", entityData);
			}

			gui::Separator();

			gui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.92, 0.25f, 0.2f, 1.f));
			if (gui::MenuItem("Delete"))
			{
				m_Scene.KillEntity(entity);
				gui::CloseCurrentPopup();
			}
			gui::PopStyleColor();

			gui::EndPopup();
		}
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	}

	void EntityReorderSeparator(const flecs::entity& entity)
	{
		gui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 3);
		gui::PushStyleColor(ImGuiCol_Separator, ImVec4(0, 0, 0, 0));
		gui::PushStyleColor(ImGuiCol_SeparatorHovered, gui::GetStyle().Colors[ImGuiCol_DragDropTarget]);
		ui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 4, ui::MatchPayloadType("ENTITY"));
		gui::PopStyleColor(2);
		gui::PopStyleVar();

		gui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0, 0, 0, 0));
		if (gui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("ENTITY"))
			{
				IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
				flecs::entity droppedEntity = *static_cast<const flecs::entity*>(payload->Data);

				if (droppedEntity.parent() == entity.parent())
				{
					std::vector<std::string>* entityOrder = nullptr;
					if (entity.parent() == flecs::entity::null())
						entityOrder = &m_Scene.m_Scene.EntityOrder;
					else if (entity.parent().has<EntityOrderComponent>())
						entityOrder = &entity.parent().get_ref<EntityOrderComponent>()->EntityOrder;

					if (entityOrder)
					{
						auto targetIt = std::find(entityOrder->begin(), entityOrder->end(), std::string(entity.name()));
						auto droppedIt = std::find(entityOrder->begin(), entityOrder->end(), std::string(droppedEntity.name()));

						if (targetIt != entityOrder->end() && droppedIt != entityOrder->end() && targetIt != droppedIt)
						{
							std::string droppedName = *droppedIt;
							entityOrder->erase(droppedIt);

							int insertIndex = std::distance(entityOrder->begin(), std::find(entityOrder->begin(), entityOrder->end(), std::string(entity.name()))) + 1;
							if (insertIndex > static_cast<int>(entityOrder->size()))
								insertIndex = static_cast<int>(entityOrder->size());
							entityOrder->insert(entityOrder->begin() + insertIndex, droppedName);
						}
					}
				}
			}
			gui::EndDragDropTarget();
		}
		gui::PopStyleColor();
	}

	void DisplayEntity(const flecs::entity& entity)
	{
		const bool is_selected = (m_Scene.SelectedEntity == entity);

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

				if (auto childEntityResolved = m_Scene.m_Scene.EntityWorld.lookup(fullChildName.c_str()))
					children.push_back(childEntityResolved);
			}
		}

		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (is_selected)
			node_flags |= ImGuiTreeNodeFlags_Selected;

		if (children.empty())
			node_flags |= ImGuiTreeNodeFlags_Leaf;

		if (m_Scene.SelectedEntity != flecs::entity::null())
		{
			auto parent = m_Scene.SelectedEntity.parent();
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
		//gui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ui::MatchPayloadType("ENTITY") ? (10 - 4) * 0.5f : 14);
		gui::SetNextItemAllowOverlap();
		bool isOpen = gui::TreeNodeEx(entity.name().c_str(), node_flags);
		float height = gui::GetItemRectSize().y;

		static bool openPopup = false;
		if (gui::IsWindowHovered() && gui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			if (gui::IsItemHovered())
			{
				//WC_INFO("Hovered {}", entity.name().c_str());
				openPopup = true;
			}
		}

		// Handle selection on click
		if (gui::IsItemClicked() && !gui::IsItemToggledOpen()) m_Scene.SelectedEntity = entity;

		if (gui::IsMouseDoubleClicked(0) && gui::IsItemHovered())
			if (entity.has<TransformComponent>())
			{
				auto& camera = m_Scene.camera;
				camera.FocalPoint = glm::vec3(glm::vec2(entity.get<TransformComponent>()->Translation), camera.FocalPoint.z); // @TODO: its still kinda buggy
				camera.Yaw = 0.f;
				camera.Pitch = 0.f;
				camera.UpdateView();
			}

		gui::PopStyleVar();
		if (gui::BeginDragDropSource())
		{
			gui::SetDragDropPayload("ENTITY", &entity, sizeof(flecs::entity));
			gui::Text("Dragging %s", entity.name().c_str());
			gui::EndDragDropSource();
		}
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		// Bond
		if (gui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("ENTITY"))
			{
				IM_ASSERT(payload->DataSize == sizeof(flecs::entity));
				flecs::entity droppedEntity = *static_cast<const flecs::entity*>(payload->Data);

				// Check if the drop target is empty space
				if (droppedEntity != entity.parent())
					m_Scene.SetChild(entity, droppedEntity);
			}
			gui::EndDragDropTarget();
		}

		gui::SameLine(gui::GetContentRegionMax().x - 20.f - gui::GetStyle().ItemSpacing.x);
		gui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
		gui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 3);
		gui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
		//gui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});
		gui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0 });
		if (gui::ImageButton(("Show##" + std::string(entity.name())).c_str(), entity.get<EntityTag>()->showEntity ? t_Eye : t_EyeClosed, { height, height }))
		{
			entity.set<EntityTag>({ !entity.get<EntityTag>()->showEntity });
			//WC_INFO("Set to: {}", entity.get<EntityTag>()->showEntity);
		}
		gui::PopStyleVar(2);
		gui::PopStyleColor(2); // 3

		if (openPopup)
		{
			gui::OpenPopup(std::to_string(entity.id()).c_str());
			openPopup = false;
		}
		EntityRightClickMenu(entity);

		if (isOpen)
		{
			gui::TreePop();

			EntityReorderSeparator(entity);

			if (entity.is_alive())
			{
				gui::TreePush(entity.name().c_str());

				for (const auto& child : children)
					DisplayEntity(child);
				gui::TreePop();
			}
		}
		else
			EntityReorderSeparator(entity);
	};

	void UI_Entities()
	{
		static bool showPopup = false;
	    gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		if (gui::Begin("Entities", &showEntities, ImGuiWindowFlags_MenuBar))
		{
			std::string entityFilter;
			if (gui::BeginMenuBar())
			{
			    gui::BeginDisabled(m_Scene.Path.empty());
				bool buttonDnD = ui::MatchPayloadType("ENTITY") && m_Scene.SelectedEntity && m_Scene.SelectedEntity.parent();
				gui::SetCursorPosX(gui::GetStyle().ItemSpacing.x);
				gui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
				gui::PushFont(Globals.f_Display.Bold);
				gui::SetNextItemWidth(gui::GetContentRegionAvail().x - gui::CalcTextSize(buttonDnD ? "Remove Parent" : "Add Entity").x - gui::GetStyle().ItemSpacing.x * 2 + gui::GetStyle().WindowPadding.x - gui::GetStyle().FramePadding.x * 2);
				static char filterBuff[256];
				gui::InputTextEx("##Search", m_Scene.Path.empty() ? "Select a Scene" : "Filter by Name", filterBuff, IM_ARRAYSIZE(filterBuff), ImVec2(0, 0), ImGuiInputTextFlags_None);
			    entityFilter = filterBuff;

				gui::BeginDisabled(buttonDnD);
				ui::PushButtonColor(gui::GetStyle().Colors[ImGuiCol_CheckMark]);
				if (gui::Button(buttonDnD ? "Remove Parent" : "Add Entity")) showPopup = true;
				gui::PopFont();
				gui::PopStyleVar();
				gui::PopStyleColor(3);
				gui::EndDisabled();
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

		    gui::Spacing();
            //WC_INFO(entityFilter);
		    //TODO - Implement search
			// Display entities
			{
				if (gui::IsMouseClicked(0) && !gui::IsAnyItemHovered() && gui::IsWindowHovered() && gui::IsWindowFocused())	m_Scene.SelectedEntity = flecs::entity::null();

				if (gui::BeginChild("##ShowEntities", { 0, 0 }, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
				{
					ui::DrawBgRows(10);
					// This is not an iterator because 'DisplayEntity' function changes the size of the entity order vector
					for (uint32_t i = 0; i < m_Scene.m_Scene.EntityOrder.size(); i++)
					{
						const auto& name = m_Scene.m_Scene.EntityOrder[i];
						auto rootEntity = m_Scene.m_Scene.EntityWorld.lookup(name.c_str());
						if (rootEntity) DisplayEntity(rootEntity);
					}

					if (gui::IsWindowHovered() && gui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						if (!gui::IsAnyItemHovered() && m_Scene.SelectedEntity != flecs::entity::null())
							gui::OpenPopup(std::to_string(m_Scene.SelectedEntity.id()).c_str());
					}
					EntityRightClickMenu(m_Scene.SelectedEntity);
				}
				gui::EndChild();
			}

			if (showPopup)
			{
				gui::OpenPopup("Add Entity");
				showPopup = false;
			}

			ui::CenterNextWindow();
		    gui::PopStyleVar();
			if (gui::BeginPopupModal("Add Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImVec2 center = gui::GetMainViewport()->GetCenter();
				ImVec2 windowSize = gui::GetWindowSize();
				ImVec2 windowPos = ImVec2(center.x - windowSize.x * 0.5f, center.y - windowSize.y * 0.5f);
				gui::SetWindowPos(windowPos, ImGuiCond_Once);

				static std::string name = "Entity";
				if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
				gui::InputText("Name", &name, ImGuiInputTextFlags_AutoSelectAll);

				const float widgetSize = gui::GetItemRectSize().x;

				gui::BeginDisabled(name.empty() || m_Scene.m_Scene.EntityWorld.lookup(name.c_str()) != flecs::entity::null());
				if (gui::Button("Create") || ui::IsKeyPressedDissabled(ImGuiKey_Enter))
				{
					m_Scene.SelectedEntity = m_Scene.AddEntity(name);
					name = "Entity";
					gui::CloseCurrentPopup();
				}
				gui::EndDisabled();
				if (name.empty()) gui::SetItemTooltip("Name cannot be empty");
			    if (m_Scene.m_Scene.EntityWorld.lookup(name.c_str()) != flecs::entity::null()) gui::SetItemTooltip("Name already exists");

				gui::SameLine();
				gui::SetCursorPosX(widgetSize);
				if (gui::Button("Cancel") || gui::IsKeyPressed(ImGuiKey_Escape))
				{
					name = "Entity";
					gui::CloseCurrentPopup();
				}
				gui::EndPopup();
			}
		}
		gui::End();
	}

	template<typename T, typename UIFunc>
	void EditComponent(const std::string& name, UIFunc uiFunc)
	{
		if (m_Scene.SelectedEntity.has<T>())
		{
			bool visible = true;

			auto& component = *m_Scene.SelectedEntity.get_mut<T>();
			if (gui::CollapsingHeader((name + "##header").c_str(), m_Scene.State == SceneState::Edit ? &visible : NULL, ImGuiTreeNodeFlags_DefaultOpen))
				uiFunc(component);

			if (!visible) m_Scene.SelectedEntity.remove<T>(); // add modal popup
		}
	}

	void UI_Properties()
	{
		if (gui::Begin("Properties", &showProperties))
		{
			if (m_Scene.SelectedEntity == flecs::entity::null())
			{
			    ImVec2 textSize = gui::CalcTextSize("Select an Entity to view it's Properties");
			    gui::SetCursorPos({(gui::GetWindowSize().x - textSize.x) * 0.5f, (gui::GetWindowSize().y - textSize.y) * 0.5f});
			    gui::TextDisabled("Select an Entity to view it's Properties");
			}
		    else
			{
				std::string nameBuffer = m_Scene.SelectedEntity.name().c_str();

				gui::PushItemWidth(gui::GetContentRegionAvail().x - gui::GetStyle().ItemSpacing.x * 2 - gui::CalcTextSize("Add Component(?)").x - gui::GetStyle().FramePadding.x * 2);
				if (gui::InputText("##Name", &nameBuffer, ImGuiInputTextFlags_EnterReturnsTrue) || gui::IsItemDeactivatedAfterEdit())
				{
					if (!nameBuffer.empty())
					{
						if (m_Scene.m_Scene.EntityWorld.lookup(nameBuffer.c_str()) != flecs::entity::null())
							gui::OpenPopup("WarnNameExists");
						else
						{
							if (m_Scene.SelectedEntity.parent() == flecs::entity::null())
							{
								auto& parentNames = m_Scene.m_Scene.EntityOrder;
								for (auto& name : parentNames)
									if (name == m_Scene.SelectedEntity.name().c_str()) name = nameBuffer;
							}
							else
							{
								auto& childrenNames = m_Scene.SelectedEntity.parent().get_ref<EntityOrderComponent>()->EntityOrder;
								for (auto& name : childrenNames)
									if (name == m_Scene.SelectedEntity.name().c_str()) name = nameBuffer;
							}

							m_Scene.SelectedEntity.set_name(nameBuffer.c_str());
						}
					}
				}
				static enum { None, Transform, Render, Rigid, Script } menu = None;
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
					    ui::CloseIfCursorFarFromCenter(showAddComponent);
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
							case Script: menuText = "Script"; break;
						}
						gui::SetCursorPosX((gui::GetWindowSize().x - gui::CalcTextSize(menuText).x) * 0.5f);
						gui::Text("%s", menuText);

						gui::Separator();

						switch (menu)
						{
							case None:
							{
								if (gui::MenuItem("Transform"))  { menu = Transform; } ui::RenderArrowIcon(ImGuiDir_Right);
								if (gui::MenuItem("Render"))     { menu = Render; } ui::RenderArrowIcon(ImGuiDir_Right);
								if (gui::MenuItem("Rigid Body")) { menu = Rigid; } ui::RenderArrowIcon(ImGuiDir_Right);
								if (gui::MenuItem("Script"))     { menu = Script; } ui::RenderArrowIcon(ImGuiDir_Right);
								break;
							}
							case Transform:
							{
								if (ItemAutoClose("Transform", m_Scene.SelectedEntity.has<TransformComponent>())) m_Scene.SelectedEntity.add<TransformComponent>();
								break;
							}
							case Render:
							{
								bool hasRender = m_Scene.SelectedEntity.has<SpriteRendererComponent>() || m_Scene.SelectedEntity.has<CircleRendererComponent>() || m_Scene.SelectedEntity.has<TextRendererComponent>();
								if (ItemAutoClose("Sprite Renderer Component", hasRender)) m_Scene.SelectedEntity.add<SpriteRendererComponent>();
								if (ItemAutoClose("Circle Renderer Component", hasRender)) m_Scene.SelectedEntity.add<CircleRendererComponent>();
								if (ItemAutoClose("Text Renderer Component", hasRender))   m_Scene.SelectedEntity.add<TextRendererComponent>();
								break;
							}
							case Rigid:
							{
								if (ItemAutoClose("Rigid Body Component", m_Scene.SelectedEntity.has<RigidBodyComponent>()))		m_Scene.SelectedEntity.add<RigidBodyComponent>();
								bool hasCollider = m_Scene.SelectedEntity.has<BoxCollider2DComponent>() || m_Scene.SelectedEntity.has<CircleCollider2DComponent>();
								if (ItemAutoClose("Box Collider Component", hasCollider))	m_Scene.SelectedEntity.add<BoxCollider2DComponent>();
								if (ItemAutoClose("Circle Collider Component", hasCollider))	m_Scene.SelectedEntity.add<CircleCollider2DComponent>();
								break;
							}
							case Script:
							{
								if (ItemAutoClose("Script Component", m_Scene.SelectedEntity.has<ScriptComponent>()))	m_Scene.SelectedEntity.add<ScriptComponent>();
								break;
							}
						}
					}
					gui::End();
				}

				ui::CenterNextWindow();
				if (gui::BeginPopupModal("WarnNameExists", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
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
					        std::string name = "None";
					        for (auto& [texturePath, id] : assetManager.TextureCache)
					        {
					            if (id == component.Texture)
                                {
					                name = std::filesystem::path(texturePath).stem().string();
					                break;
                                }
                            }
							gui::Text("Texture: "); gui::SameLine();
					        if (ui::MatchPayloadType("DND_PATH")) ui::PushButtonColor(gui::GetStyle().Colors[ImGuiCol_CheckMark], 0.8f, 0.9f, Globals.f_Display.Bold);
					        if (gui::Button(name.c_str()))
					        {
					            gui::OpenPopup("Chose Texture");
					        }
					        if (ui::MatchPayloadType("DND_PATH")) {gui::PopStyleColor(3); gui::PopFont();}
					        std::string newTexturePath = ui::FileDialog("Chose Texture", ".png,.jpg", ProjectRootPath, true);
					        if (!newTexturePath.empty())
					        {
                                //WC_INFO("Implement import - {}", newTexturePath);
					            component.Texture = assetManager.LoadTexture(newTexturePath);
					        }

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

					        if (component.Texture != 0)
					        {
                                gui::SameLine();
                                if (gui::Button("X"))
                                {
                                    component.Texture = 0;
                                }
                            }
						});

					EditComponent<CircleRendererComponent>("Circle Renderer", [](auto& component) {
						ui::Slider("Thickness", component.Thickness, 0.0f, 1.0f);
						ui::Slider("Fade", component.Fade, 0.0f, 1.0f);
						gui::ColorEdit4("Color", glm::value_ptr(component.Color));
						});

					EditComponent<TextRendererComponent>("Text Renderer", [&](auto& component) {
						//gui::InputText("Text", &component.Text);
						int newLines = 1;
					    for (char c : component.Text)
					    {
                            if (c == '\n') newLines++;
                        }
						gui::InputTextMultiline("Text", &component.Text, {0, gui::GetStyle().FramePadding.y * 2 + gui::GetFontSize() * std::min(4, newLines)}, ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_AllowTabInput);
						gui::ColorEdit4("Color", glm::value_ptr(component.Color));
						ui::Drag("Line spacing", component.LineSpacing, 0.01f);
						ui::Drag("Kerning", component.Kerning, 0.01f);

					    std::string name = "None";
                        for (auto& [texturePath, id] : assetManager.FontCache)
                        {
                            if (id == component.FontID)
                            {
                                name = std::filesystem::path(texturePath).stem().string();
                                break;
                            }
                        }
					    gui::Text("Font: "); gui::SameLine();
					    if (ui::MatchPayloadType("DND_PATH")) ui::PushButtonColor(gui::GetStyle().Colors[ImGuiCol_CheckMark], 0.8f, 0.9f, Globals.f_Display.Bold);
					    if (gui::Button(name.c_str()))
					    {
					        gui::OpenPopup("Chose Font");
					    }
					    if (ui::MatchPayloadType("DND_PATH")) {gui::PopStyleColor(3); gui::PopFont();}
					    std::string newFontPath = ui::FileDialog("Chose Font", ".ttf,.otf", ProjectRootPath, true);
                        if (!newFontPath.empty())
                        {
                            //WC_INFO("Implement import - {}", newTexturePath);
                            component.FontID = assetManager.LoadFont(newFontPath);
                        }

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

										component.FontID = assetManager.LoadFont(fontPath);
									}
									gui::EndDragDropTarget();
								}
							}
						}

					    if (component.FontID != UINT32_MAX)
					    {
                            gui::SameLine();
                            if (gui::Button("X"))
                            {
                                component.FontID = UINT32_MAX;
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
								if (gui::BeginPopupModal("Create Material##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
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

					EditComponent<ScriptComponent>("Script editor", [&](auto& component) {
						auto& script = component.ScriptInstance;
						gui::Button("Script");
						
						if (ui::MatchPayloadType("DND_PATH"))
						{
							std::filesystem::path path = static_cast<const char*>(gui::GetDragDropPayload()->Data);
							if (path.extension() == ".lua" || path.extension() == ".luau")
							{
								if (gui::BeginDragDropTarget())
								{
									if (const ImGuiPayload* payload = gui::AcceptDragDropPayload("DND_PATH"))
									{
										const char* scriptPath = static_cast<const char*>(payload->Data);
										
										component.ScriptInstance.Load(ScriptBinaries[LoadScriptBinary(scriptPath)]);
									}
									gui::EndDragDropTarget();
								}
							}
						}

						gui::SameLine();
						if (gui::Button("Reload script"))
							component.ScriptInstance.Load(ScriptBinaries[LoadScriptBinary(component.ScriptInstance.Name, true)]);

						if (script.L)
						{
							ui::Text(std::format("Stack size: {}", script.GetTop()));
							auto it = ScriptBinaryCache.find(script.Name);
							if (it != ScriptBinaryCache.end())
							{
								const auto& binID = it->second;
								for (const auto& variableName : ScriptBinaries[binID].VariableNames)
								{
									script.GetGlobal(variableName);
									if (script.IsString() && !script.IsNumber())
									{
										auto temp = script.To<std::string>();
										if (gui::InputText(variableName.c_str(), &temp))
											script.SetVariable(variableName, temp);
									}
									else if (script.IsNumber())
									{
										auto temp = script.To<double>();
										if (ui::Drag(variableName, temp))
											script.SetVariable(variableName, temp);
									}
									else if (script.IsBool())
									{
										auto temp = script.To<bool>();
										if (ui::Checkbox(variableName, temp))
											script.SetVariable(variableName, temp);
									}
									else
										ui::Text("Unknown type: " + variableName);

									script.Pop();
								}
							}
						}
					});
				}
				gui::EndChild();
			}
		}
		gui::End();
	}

	const std::unordered_map<spdlog::level::level_enum, std::pair<glm::vec4, std::string>> level_colors = { //@TODO: This could be deduced to just an array of colors
		{spdlog::level::trace,    {glm::vec4(0.f  , 184.f, 217.f, 255.f) / 255.f, "Debug"}}, // trace = debug
		{spdlog::level::info,     {glm::vec4(34.f , 197.f, 94.f , 255.f) / 255.f, "Info"}},
		{spdlog::level::warn,     {glm::vec4(255.f, 214.f, 102.f, 255.f) / 255.f, "Warn"}},
		{spdlog::level::err,      {glm::vec4(255.f, 86.f , 48.f , 255.f) / 255.f, "ERROR"}},
		{spdlog::level::critical, {glm::vec4(183.f, 29.f , 24.f , 255.f) / 255.f, "CRITICAL"}}
	};

	void UI_Console()
	{
		if (gui::Begin("Console", &showConsole, ImGuiWindowFlags_MenuBar))
		{
		    static bool showDebug = true;
		    static bool showInfo = true;
		    static bool showWarn = true;
		    static bool showError = true;
		    static bool showCritical = true;

		    if (gui::BeginMenuBar())
		    {
			    if (gui::MenuItem("Clear"))
			    {
			    	Log::GetConsoleSink()->messages.clear();
			    }

			    if (gui::MenuItem("Copy"))
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

		        if (gui::BeginMenu("Show"))
		        {
                    gui::MenuItem("Debug", nullptr, &showDebug);
                    gui::MenuItem("Info", nullptr, &showInfo);
                    gui::MenuItem("Warning", nullptr, &showWarn);
                    gui::MenuItem("Errors", nullptr, &showError);
                    gui::MenuItem("Critical", nullptr, &showCritical);

		            gui::EndMenu();
		        }

		        gui::EndMenuBar();
		    }

		    if (!Log::GetConsoleSink()->messages.empty())
		    {
		        if (gui::BeginTable("ConsoleTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit))
		        {
		            ImGui::TableSetupColumn("");
		            ImGui::TableSetupColumn("Level");
		            ImGui::TableSetupColumn("Time");
		            ImGui::TableSetupColumn("Message");
		            ImGui::TableHeadersRow();

		            for (auto& msg : Log::GetConsoleSink()->messages)
		            {
		                bool showMsg = true;
		                switch (msg.level)
		                {
		                    case spdlog::level::trace: showMsg = showDebug; break;
		                    case spdlog::level::info: showMsg = showInfo; break;
		                    case spdlog::level::warn: showMsg = showWarn; break;
		                    case spdlog::level::err: showMsg = showError; break;
		                    case spdlog::level::critical: showMsg = showCritical; break;
		                    default: showMsg = true; break;
		                }

		                if (showMsg)
		                {
		                    gui::TableNextRow();
		                    auto it = level_colors.find(msg.level);
		                    ImVec4 color;
		                    std::string prefix;
		                    std::string timeStr;
		                    Texture texture;
		                    if (it != level_colors.end())
		                    {
		                        color = conv::GlmToImVec4(it->second.first);
		                        prefix = it->second.second;
		                        auto local_time = msg.time + std::chrono::hours(2); // TODO - fix so it checks timezone
		                        timeStr = std::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::seconds>(local_time));
		                        switch (msg.level)
		                        {
		                            case spdlog::level::trace: texture = t_Debug; break;
		                            case spdlog::level::info: texture = t_Info; break;
		                            case spdlog::level::warn: texture = t_Warning; break;
		                            case spdlog::level::err: texture = t_Error; break;
		                            case spdlog::level::critical: texture = t_Critical; break;
		                            default: texture = t_Info; break;
		                        }
		                    }

		                    gui::PushFont(Globals.f_Default.Menu);
		                    gui::TableSetColumnIndex(0);
		                    gui::Image(texture, { 20, 20 }, {0, 0}, {1, 1}, color); gui::SameLine();
		                    /*gui::SetItemTooltip(prefix.c_str());*/

		                    gui::TableSetColumnIndex(1);
		                    gui::TextColored(color, prefix.c_str());

		                    gui::TableSetColumnIndex(2);
		                    gui::TextColored(color, timeStr.c_str());

		                    gui::TableSetColumnIndex(3);
		                    gui::TextColored(color, msg.payload.c_str());
		                    gui::PopFont();
		                }

		            }

		            gui::EndTable();
		        }
		    }
		    else
		    {
		        ImVec2 textSize = gui::CalcTextSize("No Messages to Display");
		        gui::SetCursorPos({(gui::GetWindowSize().x - gui::GetFrameHeightWithSpacing() - textSize.x) * 0.5f, (gui::GetWindowSize().y - textSize.y) * 0.5f});
		        gui::TextDisabled("No Messages to Display");
		    }
		}
		gui::End();
	}

    void SaveStringToFile(const std::filesystem::path& filePath, const std::string& content) 
	{
	    std::ofstream file(filePath);
	    file << content;
	}

	void UI_Assets()
    {
	    const std::set<std::string> textEditorExt = {".txt", ".scene", ".yaml", ".blzproj", /*.blzproj.user*/".user", ".blzent", ".lua", ".luau", ".luarc" };
        auto assetsPath = std::filesystem::path(ProjectRootPath);
        static std::unordered_map<std::string, bool> folderStates;  // Track the expansion state per folder
        static std::filesystem::path selectedFolderPath = assetsPath;
        static std::vector<std::filesystem::path> openedFiles;
        static std::unordered_set<std::string> openedFileNames;
        static bool showFolders = true;
        static bool showIcons = true;
        static bool previewAsset = true;
	    ImGuiID dockId;

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

        // Expand all helper function
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

        // Lambda to handle file double click logic
        auto openFileOnDoubleClick = [&](const std::filesystem::path& filePath)
        {
            if (filePath.extension() == ".scene")
            {
                gui::OpenPopup(("Confirm##Scene" + filePath.string()).c_str());
            }
            else if (filePath.extension() == ".blzent")
            {
                gui::OpenPopup(("Confirm##Blzent" + filePath.string()).c_str());
            }
            else
            {
                WC_INFO(filePath.extension().string());
                if (openedFileNames.insert(filePath.string()).second)
                    openedFiles.push_back(filePath);
            }
        };

	    auto openFilePopups = [&](const std::filesystem::path& filePath)
	    {
	        ui::CenterNextWindow();
	        if (gui::BeginPopupModal(("Confirm##Scene" + filePath.string()).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	        {
	            gui::Text("Are you sure you want to load this scene? -> %s", filePath.filename().string().c_str());
	            const float widgetWidth = gui::GetItemRectSize().x;
	            if (gui::Button("Yes##Confirm") || gui::IsKeyPressed(ImGuiKey_Enter))
	            {
	                if (m_Scene.Path != filePath.string())
	                {
	                    //WC_INFO("Assets: Opening scene: {}", filePath.string());
	                    //save old scene
	                    m_Scene.Save();
	                    m_Scene.Destroy();

	                    //load new scene
	                    AddSceneToList(filePath.string());
	                    m_Scene.SelectedEntity = flecs::entity::null();
	                    m_Scene.Load(filePath.string());
	                }
	                gui::CloseCurrentPopup();
	            }
	            gui::SameLine(widgetWidth - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
	            if (gui::Button("Cancel##Confirm") || gui::IsKeyPressed(ImGuiKey_Escape))
	                gui::CloseCurrentPopup();
	            gui::EndPopup();
	        }

	        ui::CenterNextWindow();
	        if (gui::BeginPopupModal(("Confirm##Blzent" + filePath.string()).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	        {
	            if (!m_Scene.Path.empty())
	            {
	                gui::Text("Are you sure you want to add this entity to the current scene? -> %s", filePath.filename().string().c_str());
	                static std::string name = "#@#";
	                if (name == "#@#") name = filePath.stem().string();
	                const float widgetWidth = gui::GetItemRectSize().x;
	                gui::BeginDisabled(name.empty() || m_Scene.m_Scene.EntityWorld.lookup(name.c_str()) != flecs::entity::null());
	                if (gui::Button("Yes##Confirm") || ui::IsKeyPressedDissabled(ImGuiKey_Enter))
	                {
	                    m_Scene.LoadEntity(filePath.string()); // TODO - fix with merge
	                    name = "#@#";
	                    gui::CloseCurrentPopup();
	                }
	                gui::EndDisabled();
	                if (name.empty()) gui::SetItemTooltip("Name cannot be empty");
	                if (m_Scene.m_Scene.EntityWorld.lookup(name.c_str()) != flecs::entity::null()) gui::SetItemTooltip("Name already exists");

	                gui::SameLine(widgetWidth - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
	                if (gui::Button("Cancel##Confirm") || gui::IsKeyPressed(ImGuiKey_Escape))
	                {
	                    name = "#@#";
	                    gui::CloseCurrentPopup();
	                }
	            }
	            else
	            {
	                gui::Text("Open a scene to Import an Entity");
	                if (gui::Button("Close##Confirm") || gui::IsKeyPressed(ImGuiKey_Enter))
	                {
	                    gui::CloseCurrentPopup();
	                }
	            }
	            gui::EndPopup();
	        }
	    };

	    auto openRightClick = [&](const std::string& file)
	    {
	        std::filesystem::path filePath = file;
	        if (gui::BeginPopup(("##RightClick" + file).c_str(), ImGuiWindowFlags_NoSavedSettings))
            {
	            ui::CloseIfCursorFarFromCenter();
                gui::Text(filePath.filename().string().c_str());
	            gui::Separator();
	            if (!std::filesystem::is_directory(filePath)) { if (ui::MenuItemButton("Open File")) openFileOnDoubleClick(filePath); }
	            else if (gui::MenuItem("Open in File Explorer")) FileDialogs::OpenInFileExplorer(file);
                gui::BeginDisabled(filePath == assetsPath);
	            if (ui::MenuItemButton("Rename")) gui::OpenPopup(("Rename##Rename" + file).c_str());
	            gui::EndDisabled();
	            if (filePath == assetsPath) gui::SetItemTooltip("Cannot rename root folder");
	            if (std::filesystem::is_directory(filePath)) if (ui::MenuItemButton("New File")) gui::OpenPopup(("New File##NewFile" + file).c_str());
	            gui::PushStyleColor(ImGuiCol_HeaderHovered, { 1.f, 0.f, 0.f, 0.5f });
	            if (ui::MenuItemButton("Delete")) gui::OpenPopup(("Delete Warning##DeleteWarn" + file).c_str());
	            gui::PopStyleColor();

	            ui::CenterNextWindow();
	            if (gui::BeginPopupModal(("Delete Warning##DeleteWarn" + file).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	            {
	                gui::Text("Are you sure you want to delete %s?", filePath.filename().string().c_str());
	                const float widgetWidth = gui::GetItemRectSize().x;
                    if (gui::Button("Yes##DeleteWarn") || gui::IsKeyPressed(ImGuiKey_Enter))
                    {
                        std::error_code ec;
                        std::filesystem::remove_all(filePath);
                        if (ec) {WC_ERROR("Failed to delete file: {}", ec.message());}
                        else
                        {
                            if (std::filesystem::is_directory(filePath))
                            {
                                folderStates.erase(filePath.string());
                            }
                            else
                            {
                                if (SceneExistInList(file))
                                {
                                    if (m_Scene.Path == file)
                                    {
                                        m_Scene.Path.clear();
                                        m_Scene.Destroy();
                                    }
                                    RemoveSceneFromList(file);
                                }
                                openedFiles.erase(std::remove(openedFiles.begin(), openedFiles.end(), filePath), openedFiles.end());
                                openedFileNames.erase(filePath.string());
                            }
                        }
                        gui::CloseCurrentPopup();
                    }
                    gui::SameLine(widgetWidth - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
                    if (gui::Button("Cancel##DeleteWarn") || gui::IsKeyPressed(ImGuiKey_Escape))
                        gui::CloseCurrentPopup();

	                gui::EndPopup();
	            }

	            ui::CenterNextWindow();
	            if (gui::BeginPopupModal(("Rename##Rename" + file).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	            {
	                static std::string newName = "#@#";
	                if (newName == "#@#") newName = filePath.stem().string();
	                if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
	                gui::InputText(filePath.extension().string().empty() ? "Folder" : filePath.extension().string().c_str(), &newName);
	                const float widgetSize = gui::GetItemRectSize().x;
	                gui::BeginDisabled(newName.empty());
	                if (gui::Button("Rename") || ui::IsKeyPressedDissabled(ImGuiKey_Enter))
                    {
                        std::error_code ec;
	                    std::filesystem::path newFilePath = filePath.parent_path() / (newName + filePath.extension().string());
                        std::filesystem::rename(filePath, newFilePath, ec);
                        if (ec) { WC_ERROR("Failed to rename file: {}", ec.message()); }
                        else
                        {
                            //WC_INFO("Renaming: {}, is DIR: {}", newFilePath.string(), is_directory(newFilePath));
                            if (is_directory(newFilePath))
                            {
                                folderStates.erase(newFilePath.string());
                                folderStates[(newFilePath.parent_path() / newName).string()] = false;
                                if (selectedFolderPath == filePath) selectedFolderPath = newFilePath;
                            }
                            else
                            {
                                if (openedFileNames.find(filePath.string()) != openedFileNames.end()) {
                                    // Remove the old entry from both data structures
                                    openedFiles.erase(std::remove(openedFiles.begin(), openedFiles.end(), filePath), openedFiles.end());
                                    openedFileNames.erase(filePath.string());

                                    // Push back the renamed file's new path
                                    openedFiles.push_back(newFilePath);
                                    openedFileNames.insert(newFilePath.string());
                                }
                            }
                            newName = "#@#";
                        }
                        gui::CloseCurrentPopup();
                    }
	                gui::EndDisabled();
	                if (newName.empty()) gui::SetItemTooltip("Name cannot be empty");
	                gui::SameLine(widgetSize - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
	                if (gui::Button("Cancel") || gui::IsKeyPressed(ImGuiKey_Escape))
	                {
	                    newName = "#@#";
	                    gui::CloseCurrentPopup();
	                }

	                gui::EndPopup();
	            }

                ui::CenterNextWindow();
	            if (gui::BeginPopupModal(("New File##NewFile" + file).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	            {
	                static std::string newName = "New File";
	                if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
	                gui::InputText("Name", &newName);
	                const float widgetSize = gui::GetItemRectSize().x;
	                gui::BeginDisabled(newName.empty());
	                if (gui::Button("Create") || ui::IsKeyPressedDissabled(ImGuiKey_Enter))
	                {
	                    std::error_code ec;
	                    std::filesystem::path newFilePath = filePath / newName;
	                    if (newFilePath.extension().string().empty()) {std::filesystem::create_directory(newFilePath, ec);}
                        else
                        {
	                        std::ofstream newFile(newFilePath);
	                        if (!newFile.is_open()){ WC_ERROR("Failed to create file: {}", ec.message());}
	                        else
	                        {
	                            //OPEN FILE - IF NEEDED
	                            //openedFiles.push_back(newFilePath);
	                            //openedFileNames.insert(newFilePath.string());
	                        }
                        }
	                    newName = "New File";
	                    gui::CloseCurrentPopup();
	                }
	                gui::EndDisabled();
	                if (newName.empty()) gui::SetItemTooltip("Name cannot be empty");
	                gui::SameLine(widgetSize - gui::CalcTextSize("Cancel").x - gui::GetStyle().FramePadding.x);
	                if (gui::Button("Cancel") || gui::IsKeyPressed(ImGuiKey_Escape))
	                {
	                    newName = "New File";
	                    gui::CloseCurrentPopup();
	                }
	                gui::EndPopup();
	            }

	            openFilePopups(filePath);
	            gui::EndPopup();
            }
	    };

        if (gui::Begin("Assets", &showAssets, ImGuiWindowFlags_MenuBar))
        {
            dockId = gui::GetWindowDockID();

            if (gui::BeginMenuBar())
            {
                if (gui::MenuItem("Import")) gui::OpenPopup("Import Asset##Assets");
                std::string importPath = ui::FileDialog("Import Asset##Assets", ".*");
                if (!importPath.empty())
                {
                    std::filesystem::path importFilePath = importPath;
                    std::filesystem::path importDestPath = selectedFolderPath / importFilePath.filename();
                    std::error_code ec;
                    std::filesystem::copy(importFilePath, importDestPath, ec);
                    if (ec) { WC_ERROR("Failed to copy file: {}", ec.message()); }
                    else
                    {
                        if (std::filesystem::is_directory(importDestPath))
                        {
                            folderStates[importDestPath.string()] = false;
                        }
                        else
                        {
                            //AUTO OPEN FILE IF NEEDED
                            //openedFiles.push_back(importDestPath);
                            //openedFileNames.insert(importDestPath.string());
                        }
                    }
                }

                if (gui::MenuItem("Open Path"))
                    FileDialogs::OpenInFileExplorer(selectedFolderPath.string());

                if (gui::BeginMenu("View##Assets"))
                {
                    gui::MenuItem("Show Folders", nullptr, &showFolders);
                    gui::MenuItem("Show Icons", nullptr, &showIcons);
                    gui::MenuItem("Preview Assets", nullptr, &previewAsset);

                    if (showFolders)
                    {
                        gui::Separator();
                        if (gui::MenuItem("Collapse All"))
                            setFolderStatesRecursively(assetsPath, false);

                        if (gui::MenuItem("Expand All"))
                            setFolderStatesRecursively(assetsPath, true);
                    }

                    gui::EndMenu();
                }

                gui::EndMenuBar();
            }

            // Recursive display for the folder tree (text view)
            std::function<void(const std::filesystem::path&)> displayDirectory = [&](const std::filesystem::path& path)
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

                            if (gui::IsItemHovered())
                            {
                                if (gui::IsMouseDoubleClicked(0))
                                    isOpen = !isOpen;

                                if (gui::IsMouseClicked(ImGuiMouseButton_Right))
                                    gui::OpenPopup(("##RightClick" + entry.path().string()).c_str());
                            }

                            if (isOpen)
                                displayDirectory(entry.path());
                        }
                        else
                        {
                            ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                            gui::TreeNodeEx((filenameStr + "##" + fullPathStr).c_str(), leafFlags);
                            if (gui::IsItemHovered())
                            {
                                if (previewAsset)
                                    gui::OpenPopup(("PreviewAsset##" + fullPathStr).c_str());

                                if (gui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                    openFileOnDoubleClick(entry.path());

                                if (gui::IsMouseClicked(ImGuiMouseButton_Right))
                                    gui::OpenPopup(("##RightClick" + entry.path().string()).c_str());

                                //gui::SetNextWindowPos({ gui::GetCursorScreenPos().x + gui::GetItemRectSize().x, gui::GetCursorScreenPos().y });
                                if (gui::BeginPopup(("PreviewAsset##" + fullPathStr).c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoSavedSettings))
                                {
                                    gui::Text("Preview: %s", filenameStr.c_str());
                                    gui::EndPopup();
                                }
                            }

                            openFilePopups(entry.path());
                        }
                    }
                    openRightClick(entry.path().string());
                }

                if (path != assetsPath) gui::Unindent(20);
            };

            // Display directory as icons (icon view)
            std::function<void(const std::filesystem::path&)> displayDirectoryIcons = [&](const std::filesystem::path& path)
            {
                if (selectedFolderPath == assetsPath)
                {
                    gui::BeginDisabled();
                    gui::ArrowButton("Back", ImGuiDir_Left);
                    gui::EndDisabled();
                }
                else
                {
                    if (ui::MatchPayloadType("DND_PATH")) ui::PushButtonColor(gui::GetStyle().Colors[ImGuiCol_CheckMark], 0.8f, 0.9f, Globals.f_Display.Bold);
                    if (gui::ArrowButton("Back", ImGuiDir_Left)) selectedFolderPath = selectedFolderPath.parent_path();
                    if (ui::MatchPayloadType("DND_PATH")) {gui::PopStyleColor(3); gui::PopFont();}

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
                gui::PushItemWidth(gui::GetContentRegionAvail().x);
                gui::InputText("##PreviewPath", &previewPath, ImGuiInputTextFlags_ReadOnly);
                gui::PopItemWidth();
                if (gui::IsItemHovered() || gui::IsItemActive())
                    previewPath = assetsPath.string();
                else
                    previewPath = std::filesystem::relative(selectedFolderPath, assetsPath.parent_path()).string();

                if (gui::BeginChild("Path Viewer", ImVec2{ 0, 0 }, true))
                {
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
                                    gui::ImageButton((entry.path().string() + "/").c_str(), ui::MatchPayloadType("DND_PATH") ? t_FolderOpen : t_FolderClosed, { buttonSize, buttonSize });
                                    if (gui::IsItemHovered())
                                    {
                                        if ( gui::IsMouseDoubleClicked(0))
                                            selectedFolderPath = entry.path();

                                        if (gui::IsMouseClicked(ImGuiMouseButton_Right))
                                            gui::OpenPopup(("##RightClick" + entry.path().string()).c_str());
                                    }

                                    if (gui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                                    {
                                        std::string mPath = entry.path().string();
                                        gui::SetDragDropPayload("DND_PATH", mPath.c_str(), mPath.size() + 1);
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
                                    std::istringstream stream(filename);
                                    std::vector<std::string> words{ std::istream_iterator<std::string>{stream}, std::istream_iterator<std::string>{} };

                                    std::string currentLine;
                                    float currentLineWidth = 0.0f;
                                    gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
                                    for (const auto& word : words)
                                    {
                                        ImVec2 wordSize = gui::CalcTextSize(word.c_str());
                                        if (currentLineWidth + wordSize.x > wrapWidth)
                                        {
                                            gui::SetCursorPosX(gui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
                                            gui::TextUnformatted(currentLine.c_str());
                                            currentLine.clear();
                                            currentLineWidth = 0.0f;
                                        }
                                        if (!currentLine.empty())
                                        {
                                            currentLine += " ";
                                            currentLineWidth += gui::CalcTextSize(" ").x;
                                        }
                                        currentLine += word;
                                        currentLineWidth += wordSize.x;
                                    }
                                    if (!currentLine.empty())
                                    {
                                        gui::SetCursorPosX(gui::GetCursorPosX() + (wrapWidth - currentLineWidth) / 2.0f);
                                        gui::TextUnformatted(currentLine.c_str());
                                    }
                                    gui::PopStyleVar();
                                    gui::EndGroup();
                                }
                                else
                                {
                                    gui::BeginGroup();
                                    gui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
                                    gui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                                    gui::ImageButton((entry.path().string() + "/").c_str(), t_File, { buttonSize, buttonSize });
                                    if (gui::IsItemHovered())
                                    {
                                        if (gui::IsMouseDoubleClicked(0))
                                            openFileOnDoubleClick(entry.path());

                                        if (gui::IsMouseClicked(ImGuiMouseButton_Right))
                                            gui::OpenPopup(("##RightClick" + entry.path().string()).c_str());
                                    }

                                    if (gui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                                    {
                                        std::string mPath = entry.path().string();
                                        gui::SetDragDropPayload("DND_PATH", mPath.c_str(), mPath.size() + 1);
                                        gui::Text("Moving %s", entry.path().filename().string().c_str());
                                        gui::EndDragDropSource();
                                    }

                                    gui::PopStyleVar();
                                    gui::PopStyleColor();

                                    gui::PushTextWrapPos(gui::GetCursorPos().x + buttonSize);
                                    gui::TextWrapped(entry.path().filename().string().c_str());
                                    gui::PopTextWrapPos();
                                    gui::EndGroup();

                                    openFilePopups(entry.path());
                                }
                            }
                            i++;

                            openRightClick(entry.path().string());
                        }
                        if (i == 0)
                        {
                            ImVec2 textSize = gui::CalcTextSize("This folder is Empty");
                            gui::SetCursorPos({(gui::GetWindowSize().x - textSize.x) * 0.5f, (gui::GetWindowSize().y - textSize.y) * 0.5f});
                            gui::TextDisabled("This folder is Empty");
                        }
                    }
                }
                gui::EndChild();
            };

            if (showFolders && showIcons)
            {
                if (gui::BeginTable("assets_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
                {
                    gui::TableNextRow();

                    gui::TableSetColumnIndex(0);
                    gui::BeginChild("FoldersChild", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
                    displayDirectory(assetsPath);
                    gui::EndChild();


                    gui::TableSetColumnIndex(1);
                    displayDirectoryIcons(selectedFolderPath);

                    gui::EndTable();
                }
            }
            else
            {
                if (showFolders)
                {
                    gui::Spacing();
                    gui::TableSetColumnIndex(0);
                    gui::BeginChild("FoldersChild", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar);
                    displayDirectory(assetsPath);
                    gui::EndChild();
                }
                else if (showIcons)
                {
                    gui::Spacing();
                    displayDirectoryIcons(selectedFolderPath);
                }
            }

            if (gui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && !gui::IsAnyItemHovered() && gui::IsMouseClicked(ImGuiMouseButton_Right)) gui::OpenPopup(("##RightClick" + selectedFolderPath.string()).c_str());
            openRightClick(selectedFolderPath.string());
        }
        gui::End();

	    for (auto it = openedFiles.begin(); it != openedFiles.end(); )
        {
            static std::unordered_map<std::string, std::string> fileBuffers;
            bool openFile = true;
            std::string fileKey = it->string();

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

            // For text-editable files, check if unsaved.
            if (textEditorExt.contains(it->extension().string()))
            {
				const std::string fileContent = OpenFile(fileKey);
                if (fileBuffers.find(fileKey) == fileBuffers.end())
					fileBuffers[fileKey] = fileContent;

                if (fileContent != fileBuffers[fileKey])
                    flags |= ImGuiWindowFlags_UnsavedDocument;
            }

	        ImGui::SetNextWindowDockID(dockId, ImGuiCond_Appearing); // Dock to target
            if (gui::Begin((it->filename().string() + "##" + fileKey).c_str(), &openFile, flags))
            {
                if (textEditorExt.contains(it->extension().string()))
                {
                    std::string& tempChange = fileBuffers[fileKey];

                    if (gui::BeginMenuBar())
                    {
                        if (gui::Button(("Save##" + fileKey).c_str()))
                            SaveStringToFile(*it, fileBuffers[fileKey]);

                        gui::BeginDisabled((flags& ImGuiWindowFlags_UnsavedDocument) == 0);
                        if (gui::Button(("Revert All##" + fileKey).c_str()))
                            tempChange = OpenFile(fileKey);
                        
						gui::EndDisabled();
                        gui::EndMenuBar();
                    }

					gui::InputTextMultiline(("##" + fileKey).c_str(), &tempChange, gui::GetContentRegionAvail(), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine);
                }
                else
                    gui::Text("File: %s", fileKey.c_str());
            }
            gui::End();

            if (!openFile)
            {
                openedFileNames.erase(fileKey);
                fileBuffers.erase(fileKey);
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
					ui::HelpMarker("Faster lines using texture data. Require backend to render with bi-linear filtering (not point/nearest filtering).");

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

	void WindowButtons()
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
		if (gui::ImageButton("minimize", Globals.window.IsMaximized() ? t_Minimize : t_Maximize, { buttonSize, buttonSize }))
		    Globals.window.SetMaximized(!Globals.window.IsMaximized());

		gui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.25f, 0.2f, 1.f));
		gui::SameLine(0, 0);
		static bool showCloseWarn = false;
		if (gui::ImageButton("close", t_Close, { buttonSize, buttonSize }))
		{
			if (ProjectExists())
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
	    if (gui::BeginPopupModal("Close Project Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
	    {
	        gui::Text("Save Changes to [%s] before closing?", ProjectName.c_str());
	        float textSize = gui::CalcTextSize("Save Changes to [").x + gui::CalcTextSize(ProjectName.c_str()).x + gui::CalcTextSize("] before closing?").x;
	        float spacing = textSize * 0.05f;
	        if (gui::Button("Yes", {textSize * 0.3f, 0}) || gui::IsKeyPressed(ImGuiKey_Enter))
	        {
				SaveProject();
	            Globals.window.Close();
	            gui::CloseCurrentPopup();
	        }
	        gui::SameLine(0, spacing);
	        if (gui::Button("No", {textSize * 0.3f, 0}))
	        {
	            Globals.window.Close();
	            gui::CloseCurrentPopup();
	        }
	        gui::SameLine(0, spacing);
	        if (gui::Button("Cancel", {textSize * 0.3f, 0}) || gui::IsKeyPressed(ImGuiKey_Escape))
	        {
	            gui::CloseCurrentPopup();
	        }
	        gui::EndPopup();
	    }

		// Drag when hovering tab bar
		//@TODO - Maybe Reposition this
		static bool dragging = false;
		glm::vec2 mousePos = Globals.window.GetCursorPos() + Globals.window.GetPosition();
		static glm::vec2 mouseCur = {};

		if (ImGui::IsMouseClicked(0) && Globals.window.GetCursorPos().y <= gui::GetFrameHeightWithSpacing() && !ImGui::IsAnyItemHovered())
		{
			// TODO - FIX: check if not hovering edge
			bool top = (Globals.window.GetCursorPos().y >= -Globals.window.borderSize) && (Globals.window.GetCursorPos().y <= Globals.window.borderSize);
			bool left = (Globals.window.GetCursorPos().x >= -Globals.window.borderSize) && (Globals.window.GetCursorPos().x <= Globals.window.borderSize);
			bool right = (Globals.window.GetCursorPos().x >= Globals.window.GetSize().x - Globals.window.borderSize) && (Globals.window.GetCursorPos().x <= Globals.window.GetSize().x + Globals.window.borderSize);

			if (!top && !left && !right)
			{
			    if (Globals.window.IsMaximized())
			    {
			        Globals.window.SetMaximized(false);
			        Globals.window.SetPosition(glm::vec2(mousePos.x - Globals.window.GetSize().x * 0.5f, mousePos.y - Globals.window.borderSize * 2.5f));
			    }

				dragging = true;
				mouseCur = Globals.window.GetCursorPos();
			}
		}

		if (dragging && ImGui::IsMouseDown(0))
			Globals.window.SetPosition({ mousePos.x - mouseCur.x, mousePos.y - mouseCur.y });

		if (ImGui::IsMouseReleased(0))
			dragging = false;
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
		gui::PushStyleVar(ImGuiStyleVar_WindowPadding, ProjectExists() ? ImVec2(0.f, 0.f) : ImVec2(50.f, 50.f));

		static ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		if (gui::Begin("DockSpace", nullptr, windowFlags))
		{
			if (!ProjectExists())
			{
				gui::PopStyleVar(4);
				windowFlags &= ~ImGuiWindowFlags_NoBackground;
				//windowFlags &= ~ImGuiWindowFlags_MenuBar;

				if (gui::BeginMenuBar())
				{
					WindowButtons();
					gui::EndMenuBar();
				}

			    //gui::SameLine(0, 50.f + gui::GetStyle().ItemSpacing.x);

			    gui::PushFont(Globals.f_Default.Big);
				if (gui::Button("New Project"))
				{
					gui::OpenPopup("New Project");
				}
				gui::PopFont();

				// File dialog logic
				std::string newProjectPath = ui::FileDialog("New Project", ".", "", false, ".");
				if (!newProjectPath.empty())
				{
				    std::string projName = std::filesystem::path(newProjectPath).filename().string();
				    std::string projPath = std::filesystem::path(newProjectPath).parent_path().string();
				    if (ProjectExistInList(projName))
				    {
				        gui::OpenPopup("Project Already Exists");
				    }
				    else
				    {
				        CreateProject(projPath, projName);
				    }
				}
			    ui::CenterNextWindow();
			    if (gui::BeginPopupModal("Project Already Exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			    {
			        gui::Text("Project with this name already exists!");
			        if (gui::Button("OK", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();
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
					LoadProject(openProjectPath);

				gui::Separator();

				if (gui::BeginChild("Project Display", ImVec2(0, 0)))
                {
                    constexpr int columns = 3;
				    const float projectWidth = (gui::GetContentRegionAvail().x - 50.f /*ItemSpacing x*/ * 2) / columns;
				    gui::PushStyleVar(ImGuiStyleVar_WindowPadding, {projectWidth * 0.1f, projectWidth * 0.1f});
				    const float itemSpacing = gui::GetStyle().ItemSpacing.x; // get spacing before push
				    gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(50.f, 50.f));

				    int ind = 0;
				    bool showPopup = false;
                    for (const auto& project : savedProjectPaths)
                    {
                        std::filesystem::path path = project;
                        if (std::filesystem::exists(path))
                        {
                            gui::PushStyleColor(ImGuiCol_Border, { 1.f, 0.f, 0.f, 0.f });
                            gui::PushStyleColor(ImGuiCol_ChildBg, conv::ImVec4Offset(gui::GetStyle().Colors[ImGuiCol_WindowBg], 0.07f));
                            // calculate windowHeight manually
                            const float windowHeight = projectWidth * 0.1f * 2 + projectWidth * 0.45f + gui::CalcTextSize(path.filename().string().c_str()).y + gui::CalcTextSize(path.string().c_str()).y + itemSpacing * 3;
                            if (gui::BeginChild(("##Project" + project).c_str(), {projectWidth, windowHeight}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse))
                            {
                                gui::PopStyleVar();
                                gui::Button("* ScreenShots *", {projectWidth * 0.8f, projectWidth * 0.45f});

                                gui::Spacing();

                                gui::PushFont(Globals.f_Default.Big);
                                gui::Text(path.filename().string().c_str());
                                gui::PopFont();

                                gui::TextDisabled(path.string().c_str());
                                //gui::SetWindowSize({gui::GetWindowSize().x, gui::GetCursorPos().y + 50.f}); // doesnt work?
                                if (gui::IsItemHovered())
                                {
                                    ImVec2 textMin = gui::GetItemRectMin();
                                    ImVec2 textMax = gui::GetItemRectMax();
                                    float underlineY = textMax.y + 1.0f;
                                    gui::GetWindowDrawList()->AddLine(ImVec2(textMin.x, underlineY), ImVec2(textMax.x, underlineY), gui::GetColorU32(ImGuiCol_TextDisabled), 1.0f);

                                    if (gui::IsMouseClicked(0))
                                    {
                                        FileDialogs::OpenInFileExplorer(path.string());
                                    }
                                }
                                else
                                {
                                    if (!gui::IsAnyItemHovered() && gui::IsWindowHovered())
                                    {
                                        gui::GetWindowDrawList()->AddRectFilled(gui::GetWindowPos(), {gui::GetWindowPos().x + gui::GetWindowSize().x, gui::GetWindowPos().y + gui::GetWindowSize().y}, gui::GetColorU32(ImGuiCol_Button));
										if (gui::IsMouseClicked(0))
											LoadProject(project);

                                        if (gui::IsMouseClicked(ImGuiMouseButton_Right))
                                        {
                                            //WC_CORE_INFO("RIGHT");
                                            showPopup = true;
                                        }
                                    }
                                }
                                gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(50.f, 50.f));
                            }
                            gui::EndChild();

                            gui::PopStyleColor(2);

                            if ((ind + 1) % columns != 0) gui::SameLine();

                            ind++;
                        }
                        else
                        {
                            WC_CORE_WARN("Project path does not exist: {0}", project);
                        }

                        if (showPopup)
                        {
                            gui::OpenPopup(("RightClick##" + project).c_str());
                            showPopup = false;
                        }

                        static bool showDeletePopup = false;
                        gui::PopStyleVar(2);
                        if (gui::BeginPopup(("RightClick##" + project).c_str()))
                        {
                            gui::Text(path.filename().string().c_str());
                            gui::Separator();
                            if (gui::MenuItem("Open in File Explorer"))
                            {
                                FileDialogs::OpenInFileExplorer(path.string());
                            }

                            if (gui::MenuItem("Rename"))
                            {
                                WC_CORE_INFO("Rename");
                            }

                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.f, 0.f, 0.f, 0.5f));
                            if (ui::MenuItemButton("Delete"))
                            {
                                showDeletePopup = true;
                            }
                            ImGui::PopStyleColor();

                            gui::EndPopup();
                        }

                        if (showDeletePopup)
                        {
                            gui::OpenPopup(("Delete Project##" + project).c_str());
                            showDeletePopup = false;
                        }

                        ui::CenterNextWindow();
                        if (gui::BeginPopupModal(("Delete Project##" + project).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
                        {
                            gui::Text("Are you sure you want to delete this project?");
                            if (gui::Button("Yes##Delete", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Enter))
                            {
                                DeleteProject(project);
                                gui::CloseCurrentPopup();
                            }

                            gui::SameLine(gui::CalcTextSize("Are you sure you want to delete this project?").x - gui::GetContentRegionAvail().x * 0.3f + gui::GetStyle().ItemSpacing.x);
                            if (gui::Button("No##Delete", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape)) gui::CloseCurrentPopup();

                            gui::EndPopup();
                        }

                        gui::PushStyleVar(ImGuiStyleVar_WindowPadding, {projectWidth * 0.1f, projectWidth * 0.1f});
                        gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(50.f, 50.f));
                    }
				    gui::PopStyleVar(2);
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
					if (ui::BeginMenuFt(("[" + ProjectName + "]").c_str(), Globals.f_Default.Menu))
					{
						if (gui::MenuItem("Change", "CTRL + P"))
							ResetProject();

					    if (gui::MenuItem("Open in File Explorer", "CTRL + L"))
					        FileDialogs::OpenInFileExplorer(ProjectRootPath);

					    if (ui::MenuItemButton("Rename"))
							gui::OpenPopup("Rename Project");

					    ui::CenterNextWindow();
					    if (gui::BeginPopupModal("Rename Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
					    {
					        static std::string newName = "#@#";
					        if (newName == "#@#") newName = ProjectName;
					        if (!gui::IsAnyItemHovered())gui::SetKeyboardFocusHere();
					        gui::InputText("New Name", &newName);
					        const float widgetWidth = gui::GetItemRectSize().x;

					        gui::BeginDisabled(newName.empty());
					        if (gui::Button("OK", { gui::GetContentRegionMax().x * 0.3f, 0 }) || ui::IsKeyPressedDissabled(ImGuiKey_Enter))
                            {
                                RenameProject(newName);
                                newName = "#@#";
                                gui::CloseCurrentPopup();
                            }
					        gui::EndDisabled();
					        if (newName.empty()) gui::SetItemTooltip("Project name cannot be empty!");
					        gui::SameLine(widgetWidth - gui::GetContentRegionMax().x * 0.3f + gui::GetStyle().ItemSpacing.x);
					        if (gui::Button("Cancel", { gui::GetContentRegionMax().x * 0.3f, 0 }) || gui::IsKeyPressed(ImGuiKey_Escape))
                            {
                                newName = "#@#";
                                gui::CloseCurrentPopup();
                            }
					        gui::EndPopup();
					    }

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

						if (ui::MenuItemButton("New", "CTRL + N", false)) gui::OpenPopup("New Scene");

						std::string newScenePath = ui::FileDialog("New Scene", ".", ProjectRootPath, true, ".scene");
						if (!newScenePath.empty())
						{
						    //save old scene
						    m_Scene.Save();
						    m_Scene.Destroy();

						    //create new scene
						    AddSceneToList(newScenePath);
						    m_Scene.Save(newScenePath);
						}

						if (ui::MenuItemButton("Open", "CTRL + O", false))
							gui::OpenPopup("Open Scene");

						std::string sOpenPath = ui::FileDialog("Open Scene", ".scene", ProjectRootPath, true);
						if (!sOpenPath.empty())
						{
							m_Scene.Load(m_Scene.Path);
							gui::CloseCurrentPopup();
						}

						gui::SeparatorText("File");

						if (gui::MenuItem("Save", "CTRL + S"))
						{
							m_Scene.Save();
							SavePhysicsMaterials(ProjectRootPath + "\\physicsMaterials.yaml");
						}

						if (gui::MenuItem("Save As", "CTRL + A + S"))
							m_Scene.Save();

						if (gui::MenuItem("Undo", "CTRL + Z"))
							m_Scene.Undo();

						if (gui::MenuItem("Redo", "CTRL + Y"))
							m_Scene.Redo();

						gui::EndMenu();
					}

					if (ui::BeginMenuFt("View", Globals.f_Default.Menu))
					{
						gui::MenuItem("Editor", nullptr, &showEditor);
						gui::MenuItem("Scene properties", nullptr, &showSceneProperties);
						gui::MenuItem("Entities", nullptr, &showEntities);
						gui::MenuItem("Properties", nullptr, &showProperties);
						gui::MenuItem("Console", nullptr, &showConsole);
						gui::MenuItem("Assets", nullptr, &showAssets);
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
								case 0: gui::GetStyle() = ui::SoDark(hue); break;
								case 1: gui::StyleColorsClassic(); break;
								case 2: gui::StyleColorsDark(); break;
								case 3: gui::StyleColorsLight(); break;
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
				if (ProjectExists())
				{
					if (showEditor) UI_Editor();
					if (showSceneProperties) UI_SceneProperties();
					if (showEntities) UI_Entities();
					if (showProperties) UI_Properties();
					if (showConsole) UI_Console();
					if (showAssets) UI_Assets();
					if (showDebugStats) UI_DebugStats();
					if (showStyleEditor) UI_StyleEditor();
				}
			}
		}
		gui::End();
	}

	// [PROJECT MANAGING]
	std::string ProjectName;
	std::string ProjectRootPath;
	std::string ProjectFirstScene;

	std::string texturePath;
	std::string fontPath;
	std::string soundPath;

	std::string scenesPath;
	std::string scriptsPath;
	std::string entitiesPath;

	std::vector<std::string> savedProjectPaths; // @NOTE: This could just be std::set
	std::vector<std::string> savedProjectScenes;

	bool AutoCleanUpProjectOnExit = true; // If this flag is set, when exiting the application all materials and entities that are not used in any scenes will be deleted.

	void AddSceneToList(const std::string& path)
	{
		if (std::find(savedProjectScenes.begin(), savedProjectScenes.end(), path) == savedProjectScenes.end())
			savedProjectScenes.push_back(path);
	}

	void RemoveSceneFromList(const std::string& filepath) { std::erase(savedProjectScenes, filepath); }

	bool SceneExistInList(const std::string& path)
	{
		for (auto& each : savedProjectScenes)
			if (std::filesystem::path(each).string() == path)
				return true;

		return false;
	}

	void AddProjectToList(const std::string& path)
	{
		if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), path) == savedProjectPaths.end())
			savedProjectPaths.push_back(path);
	}

	void RemoveProjectFromList(const std::string& filepath) { std::erase(savedProjectPaths, filepath); }

	bool ProjectExistInList(const std::string& pName)
	{
		for (auto& each : savedProjectPaths)
			if (std::filesystem::path(each).filename().string() == pName)
				return true;

		return false;
	}

	void ResetProject()
	{
		ProjectName = "";
		ProjectRootPath = "";
		ProjectFirstScene = "";
		savedProjectScenes.clear();
	}

	bool IsProject(const std::string& filepath) { return std::filesystem::exists(filepath + "\\settings" + PROJECT_SETTINGS_EXTENSION); }
	std::string GetProjectSettingsPath() { return ProjectRootPath + "\\settings" + PROJECT_SETTINGS_EXTENSION; }
	std::string GetProjectUserSettingsPath() { return ProjectRootPath + "\\settings" + PROJECT_USER_SETTINGS_EXTENSION; }

	void SaveProjectData()
	{
		if (ProjectRootPath.empty()) return;

		YAML::Node data;
		YAML_SAVE_VAR(data, texturePath);
		YAML_SAVE_VAR(data, fontPath);
		YAML_SAVE_VAR(data, soundPath);

		YAML_SAVE_VAR(data, scenesPath);
		YAML_SAVE_VAR(data, scriptsPath);
		YAML_SAVE_VAR(data, entitiesPath);

		YAML::Node openedScenes;
		for (auto& scene : savedProjectScenes)
			openedScenes.push_back(scene);

		YAML::Node userData;
		userData["OpenedScenes"] = openedScenes;

		YAMLUtils::SaveFile(GetProjectSettingsPath(), data);
		YAMLUtils::SaveFile(GetProjectUserSettingsPath(), userData);
	}

	void SaveProject()
	{
		SaveProjectData();
		m_Scene.Save(); // @TODO: This should save all opened scenes
	}

	bool LoadProject(const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("{} does not exist", filepath);
			return false;
		}

		if (!IsProject(filepath))
		{
			WC_CORE_ERROR("{} is not a Blaze project", filepath);
			return false;
		}

		ProjectName = std::filesystem::path(filepath).stem().string();
		ProjectRootPath = filepath;

		YAML::Node data = YAML::LoadFile(GetProjectSettingsPath());
		if (data)
		{
			YAML_LOAD_VAR(data, texturePath);
			YAML_LOAD_VAR(data, fontPath);
			YAML_LOAD_VAR(data, soundPath);

			YAML_LOAD_VAR(data, scenesPath);
			YAML_LOAD_VAR(data, scriptsPath);
			YAML_LOAD_VAR(data, entitiesPath);
		}
		AddProjectToList(ProjectRootPath);

		LoadPhysicsMaterials(ProjectRootPath + "\\physicsMaterials.yaml");

		if (std::filesystem::exists(GetProjectUserSettingsPath()))
		{
			YAML::Node userDataScenes = YAML::LoadFile(GetProjectUserSettingsPath());
			for (const auto& openedScene : userDataScenes["OpenedScenes"])
			{
				auto path = openedScene.as<std::string>();
				AddSceneToList(path);
				m_Scene.Load(path);
			}
		}
		return true;
	}

	void CreateProject(const std::string& filepath, const std::string& pName)
	{
		if (ProjectExistInList(pName))
		{
			WC_CORE_WARN("Project with this name already exists: {}", pName);
			return;
		}

		ProjectName = pName;
		ProjectRootPath = filepath + "\\" + pName; // @TODO: I think the '\\' are OS specific

		texturePath = ProjectRootPath + "\\Textures";
		fontPath = ProjectRootPath + "\\Fonts";
		soundPath = ProjectRootPath + "\\Sounds";

		scenesPath = ProjectRootPath + "\\Scenes";
		scriptsPath = ProjectRootPath + "\\Scripts";
		entitiesPath = ProjectRootPath + "\\Entities";

		AddProjectToList(ProjectRootPath);
		//std::string assetDir = ProjectRootPath + "\\Assets";
		std::filesystem::create_directory(ProjectRootPath);

		std::filesystem::create_directory(texturePath);
		std::filesystem::create_directory(fontPath);
		std::filesystem::create_directory(soundPath);

		std::filesystem::create_directory(scenesPath);
		std::filesystem::create_directory(scriptsPath);
		std::filesystem::create_directory(entitiesPath);

		SaveProjectData();
	}

	void DeleteProject(const std::string& filepath) // @TODO: This function should accept project index from savedProjectPaths
	{
		if (std::filesystem::exists(filepath))
		{
			if (IsProject(filepath))
			{
				std::filesystem::remove_all(filepath);
				RemoveProjectFromList(filepath);
				ResetProject();
				WC_CORE_INFO("Deleted project: {}", filepath);
			}
			else WC_CORE_WARN("Directory is not a .blz project: {}", filepath);
		}
		else
			WC_CORE_WARN("Project path does not exist: {}", filepath);
	}

	void RenameProject(const std::string& newName)
	{
		if (newName.empty())
		{
			WC_CORE_WARN("Project name cannot be empty");
			return;
		}

		if (ProjectExistInList(newName))
		{
			WC_CORE_WARN("Project with this name already exists: {}", newName);
			return;
		}

		RemoveProjectFromList(ProjectRootPath);
		std::filesystem::rename(ProjectRootPath, ProjectRootPath.substr(0, ProjectRootPath.find_last_of('\\') + 1) + newName);
		ProjectRootPath = ProjectRootPath.substr(0, ProjectRootPath.find_last_of('\\') + 1) + newName;
		AddProjectToList(ProjectRootPath);
		ProjectName = newName;
		SaveProjectData();
	}

	bool ProjectExists() { return !ProjectName.empty() && !ProjectRootPath.empty(); }
	
	// [SETTINGS MANAGING]

	float ZoomSpeed = 1.f;

	void SaveSettings()
	{
		YAML::Node data;
		YAML_SAVE_VAR(data, ZoomSpeed);
		data["projectPaths"] = savedProjectPaths;

		YAMLUtils::SaveFile("settings.yaml", data);
	}
	
	void LoadSettings()
	{
		std::string filepath = "settings.yaml";
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("Save file does not exist: {}", filepath);
			return;
		}

		YAML::Node data = YAML::LoadFile(filepath);
		if (!data)
		{
			WC_CORE_ERROR("Couldn't open file {}", filepath);
			return;
		}
		YAML_LOAD_VAR(data, ZoomSpeed);
		if (data["projectPaths"])
		{
			bool shouldSave = false;

			for (const auto& iPath : data["projectPaths"])
			{
				auto path = iPath.as<std::string>();
				if (std::filesystem::exists(path) && IsProject(path))
					savedProjectPaths.push_back(path);
				else
				{
					std::erase(savedProjectPaths, path);
					shouldSave = true;
				}
			}

			if (shouldSave) SaveSettings();
		}
	}
};
