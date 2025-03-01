#pragma once

#include "UI/Widgets.h"

#include "Scene.h"
#include <wc/Math/Camera.h>

using namespace blaze;
using namespace wc;

YAML::Node SerializeEntity(const Scene& scene, const flecs::entity& entity)
{
	YAML::Node entityData;

	entityData["Name"] = (entity.name() != "null") ? std::string(entity.name().c_str()) : std::string("null[5ws78@!12]");

	if (entity.has<TransformComponent>())
	{
		auto component = entity.get_ref<TransformComponent>();
		YAML::Node componentData;
		componentData["Translation"] = component->Translation;
		componentData["Scale"] = component->Scale;
		componentData["Rotation"] = glm::degrees(component->Rotation);
		entityData["TransformComponent"] = componentData;
	}

	if (entity.has<TextRendererComponent>())
	{
		auto component = entity.get_ref<TextRendererComponent>();
		YAML::Node componentData;
		componentData["Text"] = component->Text;
		for (const auto& [name, fontID] : assetManager.FontCache) // ew, but there's no other way ig
		{
			if (fontID == component->FontID)
			{
				componentData["Font"] = name;
				break;
			}
		}

		componentData["Color"] = component->Color;
		componentData["Kerning"] = component->Kerning;
		componentData["LineSpacing"] = component->LineSpacing;
		entityData["TextRendererComponent"] = componentData;
	}

	if (entity.has<SpriteRendererComponent>())
	{
		auto component = entity.get_ref<SpriteRendererComponent>();
		YAML::Node componentData;
		componentData["Color"] = component->Color;

		for (const auto& [name, texID] : assetManager.TextureCache) // ew, but there's no other way ig
		{
			if (texID == component->Texture)
			{
				componentData["Texture"] = name;
				break;
			}
		}

		entityData["SpriteRendererComponent"] = componentData;
	}

	if (entity.has<CircleRendererComponent>())
	{
		auto component = entity.get_ref<CircleRendererComponent>();
		YAML::Node componentData;
		componentData["Color"] = component->Color;
		componentData["Thickness"] = component->Thickness;
		componentData["Fade"] = component->Fade;
		entityData["CircleRendererComponent"] = componentData;
	}

	if (entity.has<RigidBodyComponent>())
	{
		auto component = entity.get_ref<RigidBodyComponent>();
		YAML::Node componentData;
		componentData["Type"] = magic_enum::enum_name(component->Type);
		componentData["FixedRotation"] = component->FixedRotation;
		componentData["Bullet"] = component->Bullet;
		componentData["FastRotation"] = component->FastRotation;
		componentData["GravityScale"] = component->GravityScale;
		componentData["LinearDamping"] = component->LinearDamping;
		componentData["AngularDamping"] = component->AngularDamping;
		entityData["RigidBodyComponent"] = componentData;
	}

	if (entity.has<BoxCollider2DComponent>())
	{
		auto component = entity.get_ref<BoxCollider2DComponent>();
		YAML::Node componentData;
		componentData["Offset"] = component->Offset;
		componentData["Size"] = component->Size;

		if (component->MaterialID != 0)
			for (const auto& [name, matID] : PhysicsMaterialNames) // ew, but there's no other way ig
			{
				if (matID == component->MaterialID)
				{
					componentData["Material"] = name;
					break;
				}
			}

		entityData["BoxCollider2DComponent"] = componentData;
	}

	if (entity.has<CircleCollider2DComponent>())
	{
		auto component = entity.get_ref<CircleCollider2DComponent>();
		YAML::Node componentData;
		componentData["Offset"] = component->Offset;
		componentData["Radius"] = component->Radius;

		if (component->MaterialID != 0)
			for (const auto& [name, matID] : PhysicsMaterialNames) // ew, but there's no other way ig
			{
				if (matID == component->MaterialID)
				{
					componentData["Material"] = name;
					break;
				}
			}

		entityData["CircleCollider2DComponent"] = componentData;
	}

	if (entity.has<ScriptComponent>())
	{
		auto component = entity.get_ref<ScriptComponent>();
		YAML::Node componentData;
		componentData["Path"] = component->ScriptInstance.Name;

		entityData["ScriptComponent"] = componentData;
	}

	if (entity.has<EntityOrderComponent>())
	{
		YAML::Node childrenData;
		scene.EntityWorld.query_builder<EntityTag>()
			.with(flecs::ChildOf, entity)
			.each([&](flecs::entity child, EntityTag) {
			YAML::Node childData = SerializeEntity(scene, child);

			childrenData.push_back(childData);
				});

		/*for (const auto& name : entity.get_ref<EntityOrderComponent>()->EntityOrder)
		{
			auto child = EntityWorld.lookup(name.c_str());

			YAML::Node childData = SerializeEntity(child);

			childrenData.push_back(childData);
		}*/

		if (childrenData.size() != 0)
			entityData["Children"] = childrenData;
	}

	return entityData;
}

flecs::entity DeserializeEntity(Scene& scene, const YAML::Node& entityData)
{
	flecs::entity entity;
	std::string name = entityData["Name"].as<std::string>();

	if (name == "null[5ws78@!12]") entity = scene.AddEntity("null");
	else if (name != "null") entity = scene.AddEntity(name);
	else entity = scene.AddEntity();

	auto transformComponent = entityData["TransformComponent"];
	if (transformComponent)
	{
		entity.set<TransformComponent>({
			transformComponent["Translation"].as<glm::vec3>(),
			transformComponent["Scale"].as<glm::vec2>(),
			glm::radians(transformComponent["Rotation"].as<float>())
			});
	}

	{
		auto componentData = entityData["TextRendererComponent"];
		if (componentData)
		{
			TextRendererComponent component;
			component.Text = componentData["Text"].as<std::string>();
			component.Color = componentData["Color"].as<glm::vec4>();
			component.Kerning = componentData["Kerning"].as<float>();
			component.LineSpacing = componentData["LineSpacing"].as<float>();

			if (componentData["Font"]) component.FontID = assetManager.LoadFont(componentData["Font"].as<std::string>());

			entity.set<TextRendererComponent>(component);
		}
	}

	{
		auto componentData = entityData["SpriteRendererComponent"];
		if (componentData)
		{
			SpriteRendererComponent component;
			component.Color = componentData["Color"].as<glm::vec4>();
			component.Texture = assetManager.LoadTexture(componentData["Texture"].as<std::string>());

			entity.set<SpriteRendererComponent>(component);
		}
	}

	auto circleRendererComponent = entityData["CircleRendererComponent"];
	if (circleRendererComponent)
	{
		entity.set<CircleRendererComponent>({
			circleRendererComponent["Color"].as<glm::vec4>(),
			circleRendererComponent["Thickness"].as<float>(),
			circleRendererComponent["Fade"].as<float>()
			});
	}

	auto rigidBodyComponent = entityData["RigidBodyComponent"];
	if (rigidBodyComponent)
	{
		RigidBodyComponent component = {
			.Type = magic_enum::enum_cast<BodyType>(rigidBodyComponent["Type"].as<std::string>()).value(),
			.FixedRotation = rigidBodyComponent["FixedRotation"].as<bool>(),
			.Bullet = rigidBodyComponent["Bullet"].as<bool>(),
			.FastRotation = rigidBodyComponent["FastRotation"].as<bool>(),
			.GravityScale = rigidBodyComponent["GravityScale"].as<float>(),
			.LinearDamping = rigidBodyComponent["LinearDamping"].as<float>(),
			.AngularDamping = rigidBodyComponent["AngularDamping"].as<float>(),
		};
		entity.set<RigidBodyComponent>(component);
	}

	{
		auto componentData = entityData["BoxCollider2DComponent"];
		if (componentData)
		{
			BoxCollider2DComponent component;
			component.Offset = componentData["Offset"].as<glm::vec2>();
			component.Size = componentData["Size"].as<glm::vec2>();
			if (componentData["Material"]) component.MaterialID = PhysicsMaterialNames[componentData["Material"].as<std::string>()];

			entity.set<BoxCollider2DComponent>(component);
		}
	}

	{
		auto componentData = entityData["CircleCollider2DComponent"];
		if (componentData)
		{
			CircleCollider2DComponent component;
			component.Offset = componentData["Offset"].as<glm::vec2>();
			component.Radius = componentData["Radius"].as<float>();
			if (componentData["Material"]) component.MaterialID = PhysicsMaterialNames[componentData["Material"].as<std::string>()];

			entity.set<CircleCollider2DComponent>(component);
		}
	}

	{
		auto componentData = entityData["ScriptComponent"];
		if (componentData)
		{
			ScriptComponent component;
			component.ScriptInstance.Load(ScriptBinaries[LoadScriptBinary(componentData["Path"].as<std::string>())]);

			entity.set<ScriptComponent>(component);
		}
	}

	auto childEntities = entityData["Children"];
	if (childEntities)
		for (const auto& child : childEntities)
			scene.SetChild(entity, DeserializeEntity(scene, child), false);

	return entity;
}

YAML::Node toYAML(const Scene& scene)
{
	YAML::Node metaData;
	YAML::Node entitiesData;

	for (auto& name : scene.EntityOrder)
	{
		auto entity = scene.EntityWorld.lookup(name.c_str());

		entitiesData.push_back(SerializeEntity(scene, entity));
	}

	if (scene.PhysicsWorld.IsValid())
	{
		metaData["Gravity"] = scene.PhysicsWorld.GetGravity();
	}

	if (entitiesData)
		metaData["Entities"] = entitiesData;

	return metaData;
}

void fromYAML(Scene& scene, const YAML::Node& data)
{
	auto entities = data["Entities"];
	if (entities)
	{
		for (const auto& entity : entities)
			DeserializeEntity(scene, entity);
	}
}

void SaveScene(const Scene& scene, const std::string& filepath) { YAMLUtils::SaveFile(filepath, toYAML(scene)); }

bool LoadScene(Scene& scene, const std::string& filepath, const bool clear = true)
{
	if (clear) scene.DeleteAllEntities();
	if (!std::filesystem::exists(filepath))
	{
		WC_CORE_ERROR("{} does not exist.", filepath);
		return false;
	}

	fromYAML(scene, YAML::LoadFile(filepath));
	Project::AddSceneToList(filepath);
	return true;
}

void CopyScene(const Scene& srcScene, Scene& dstScene)
{
	dstScene.DeleteAllEntities();
	fromYAML(dstScene, toYAML(srcScene));
}

enum class SceneState { Edit, Simulate, Play };

struct EditorScene
{
	// @NOTE: maybe we should move the entity ordering in this struct as this is a feature that is only being used by the editor
	Scene m_Scene;
	Scene m_TempScene;

	std::string Path;

	flecs::entity SelectedEntity = flecs::entity::null();

	EditorCamera camera;

	SceneState State = SceneState::Edit;
	ImGuizmo::OPERATION GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y | ImGuizmo::OPERATION::TRANSLATE_Z;

	void EditorInput(glm::vec2 RenderSize)
	{
		if (gui::IsKeyPressed(ImGuiKey_G)) GuizmoOp = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y | ImGuizmo::OPERATION::TRANSLATE_Z;
		else if (gui::IsKeyPressed(ImGuiKey_R)) GuizmoOp = ImGuizmo::OPERATION::ROTATE_Z;
		else if (gui::IsKeyPressed(ImGuiKey_S)) GuizmoOp = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Y;

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
				auto panSpeed = camera.PanSpeed(RenderSize);
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

	void CreatePhysicsWorld() { m_Scene.CreatePhysicsWorld(); }
	void Create() { m_Scene.Create(); }

	void Destroy() { m_Scene.Destroy(); }
	void DeleteAllEntities() { m_Scene.DeleteAllEntities(); }

	flecs::entity AddEntity() { return m_Scene.AddEntity(); }
	flecs::entity AddEntity(const std::string& name) { return m_Scene.AddEntity(name); }

	void CopyEntity(const flecs::entity& ent, const flecs::entity& ent2) { m_Scene.CopyEntity(ent, ent2); }
	void DuplicateEntity(const flecs::entity& ent) 
	{ 
		//auto ent2 = AddEntity(std::string(ent.name()) + " Clone");
		//m_Scene.CopyEntity(ent, ent2); 
		WC_CORE_INFO("TODO: Implement clone");
	}
	void KillEntity(const flecs::entity& ent) 
	{ 
		if (SelectedEntity == ent) SelectedEntity = flecs::entity::null();
		m_Scene.KillEntity(ent); 
	}

	void RemoveChild(const flecs::entity& child) { m_Scene.RemoveChild(child); }
	void SetChild(const flecs::entity& parent, const flecs::entity& child) { m_Scene.SetChild(parent, child); }

	void Start() 
	{ 
		CreatePhysicsWorld();

		m_Scene.EntityWorld.each([this](ScriptComponent& script)
			{
				if (script.ScriptInstance.L)
					script.ScriptInstance.Execute("Create");
			});
	}
	void Stop() 
	{ 
		m_Scene.EntityWorld.each([this](ScriptComponent& script)
			{
				if (script.ScriptInstance.L)
					script.ScriptInstance.Execute("Destroy");
			});
		m_Scene.PhysicsWorld.Destroy();
	}

	void SetState(SceneState newState)
	{
		std::string selectedEntityName;
		if (SelectedEntity != flecs::entity::null()) selectedEntityName = SelectedEntity.name().c_str();
		//WC_INFO("Changing scene state. Selected Entity: {0}", selectedEntityName);

		State = newState;

		if (newState == SceneState::Play || newState == SceneState::Simulate)
		{
			CopyScene(m_Scene, m_TempScene);
			Start();
		}
		else if (newState == SceneState::Edit)
		{
			Stop();
			CopyScene(m_TempScene, m_Scene);
		}

		if (SelectedEntity != flecs::entity::null()) SelectedEntity = m_Scene.EntityWorld.lookup(selectedEntityName.c_str());
		//WC_INFO("Scene state changed. Selected Entity: {0}", m_SelectedEntity.name().c_str());
	}

	void UpdatePhysics() { m_Scene.UpdatePhysics(); }
	void Update() 
	{ 
		if (State == SceneState::Play || State == SceneState::Simulate)
			m_Scene.Update();
	}

	YAML::Node SerializeEntity(const flecs::entity& entity) const { return SerializeEntity(entity); }
	flecs::entity DeserializeEntity(const YAML::Node& entityData) { return ::DeserializeEntity(m_Scene, entityData); } // bruh :: is absolutely not required, thanks clang

	void Save() { SaveScene(m_Scene, Path); }
	bool Load(const std::string& filepath, const bool clear = true)
	{
		Path = filepath;
		return LoadScene(m_Scene, Path, clear);
	}
};