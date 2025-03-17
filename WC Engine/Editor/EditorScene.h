#pragma once

#include "../UI/Widgets.h"

#include "../Scene/Scene.h"
#include "Commands.h"
#include <wc/Math/Camera.h>

using namespace blaze;
using namespace wc;

namespace Editor
{
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
			/*scene.EntityWorld.query_builder<EntityTag>()
				.with(flecs::ChildOf, entity)
				.each([&](flecs::entity child, EntityTag) {
				YAML::Node childData = SerializeEntity(scene, child);

				childrenData.push_back(childData);
					});*/

			for (const auto& name : entity.get_ref<EntityOrderComponent>()->EntityOrder)
			{
				auto child = entity.lookup(name.c_str());

				if (child)
				{
					YAML::Node childData = SerializeEntity(scene, child);

					childrenData.push_back(childData);
				}
				else
				{
					WC_ERROR("Could not find entity with name '{}'", name.c_str());
				}
			}

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

		try
		{
			auto transformComponent = entityData["TransformComponent"];
			if (transformComponent)
			{
				entity.set<TransformComponent>({
					transformComponent["Translation"].as<glm::vec3>(),
					transformComponent["Scale"].as<glm::vec3>(),
					glm::radians(transformComponent["Rotation"].as<glm::vec3>())
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
		}
		catch (std::exception ex)
		{
			WC_ERROR(ex.what());
			return entity;
		}
		return entity;
	}

	YAML::Node toYAML(const Scene& scene)
	{
		YAML::Node metaData;
		YAML::Node entitiesData;

		for (const auto& name : scene.EntityOrder)
		{
			auto entity = scene.EntityWorld.lookup(name.c_str());

			if (entity)
			{
				entitiesData.push_back(SerializeEntity(scene, entity));
			}
			else
			{
				WC_ERROR("Could not find entity with name '{}'", name.c_str());
			}
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

	void CopyScene(const Scene& srcScene, Scene& dstScene)
	{
		dstScene.DeleteAllEntities();
		fromYAML(dstScene, toYAML(srcScene));
	}

	TransformComponent GetEntityWorldTransform(const flecs::entity& entity)
	{
		TransformComponent tc = *entity.get<TransformComponent>();

		auto parent = entity.parent();
		while (parent != flecs::entity::null())
		{
			if (parent.has<TransformComponent>())
			{
				auto parent_tc = *parent.get<TransformComponent>();
				tc.Translation += parent_tc.Translation;
				tc.Rotation += parent_tc.Rotation;
				tc.Scale *= parent_tc.Scale;

				parent = parent.parent();
			}
		}

		return tc;
	}

	enum class SceneState { Edit, Simulate, Play };

	struct EditorScene
	{
		// @NOTE: maybe we should move the entity ordering in this struct as this is a feature that is only being used by the editor
		Scene m_Scene;
		Scene m_TempScene;

		std::string Path;

		float snapStrength = 1.0f;

		flecs::entity SelectedEntity = flecs::entity::null();
		std::vector<void*> CommandBuffer;
		uint32_t CommandIndex = 0;

		EditorCamera camera;

		SceneState State = SceneState::Edit;
		ImGuizmo::OPERATION GuizmoOp = ImGuizmo::OPERATION::TRANSLATE;

		void CreatePhysicsWorld() { m_Scene.CreatePhysicsWorld(); }
		void Create() { m_Scene.Create(); }

		void Destroy()
		{
			m_Scene.Destroy();

			SelectedEntity = flecs::entity::null();
			CommandIndex = 0;

			for (const auto& cmd : CommandBuffer)
				delete cmd;

			CommandBuffer.clear();

			GuizmoOp = ImGuizmo::OPERATION::TRANSLATE;
			Path.clear();
		}
		void DeleteAllEntities() { m_Scene.DeleteAllEntities(); }

		flecs::entity AddEntity() { return m_Scene.AddEntity(); }
		flecs::entity AddEntity(const std::string& name) { return m_Scene.AddEntity(name); }

		void DuplicateEntity(const flecs::entity& ent)
		{
			auto ent2 = m_Scene.CopyEntity(ent);
			m_Scene.SetEntityName(ent2, std::string(ent.name().c_str()) + " Clone");
		}

		void KillEntity(const flecs::entity& ent)
		{
			if (SelectedEntity == ent) SelectedEntity = flecs::entity::null();
			m_Scene.KillEntity(ent);
		}

		void RemoveChild(const flecs::entity& child) { m_Scene.RemoveChild(child); }
		void SetChild(const flecs::entity& parent, const flecs::entity& child) { m_Scene.SetChild(parent, child); }

		void SetState(SceneState newState)
		{
			std::string selectedEntityName;
			if (SelectedEntity != flecs::entity::null()) selectedEntityName = SelectedEntity.name().c_str();

			State = newState;

			if (newState == SceneState::Play || newState == SceneState::Simulate)
			{
				CopyScene(m_Scene, m_TempScene);

				CreatePhysicsWorld();

				m_Scene.EntityWorld.each([this](ScriptComponent& script)
					{
						if (script.ScriptInstance)
							script.ScriptInstance.state.Execute("Create");
					});
			}
			else if (newState == SceneState::Edit)
			{
				m_Scene.EntityWorld.each([this](ScriptComponent& script)
					{
						if (script.ScriptInstance)
							script.ScriptInstance.state.Execute("Destroy");
					});
				m_Scene.PhysicsWorld.Destroy();

				CopyScene(m_TempScene, m_Scene);
			}

			if (SelectedEntity != flecs::entity::null()) SelectedEntity = m_Scene.EntityWorld.lookup(selectedEntityName.c_str());
		}

		void UpdatePhysics() { m_Scene.UpdatePhysics(); }
		void Update()
		{
			if (State == SceneState::Play || State == SceneState::Simulate)
				m_Scene.Update();
		}

		YAML::Node ExportEntity(const flecs::entity& entity) const { return SerializeEntity(m_Scene, entity); }
		flecs::entity LoadEntity(const YAML::Node& entityData) { return DeserializeEntity(m_Scene, entityData); } // bruh :: is absolutely not required, thanks clang
		flecs::entity LoadEntity(const std::string& path) { return LoadEntity(YAML::LoadFile(path)); }

		void Save()
		{
			YAML::Node data = toYAML(m_Scene);
			data["CameraFocalPoint"] = camera.FocalPoint; // @TODO: Camera loading doesnt work
			data["CameraYaw"] = camera.Yaw;
			data["CameraPitch"] = camera.Pitch;
			data["CameraDistance"] = camera.m_Distance;
			YAMLUtils::SaveFile(Path, data);
		}

		void Save(const std::string& filepath)
		{
			Path = filepath;
			Save();
		}

		bool Load(const std::string& filepath, const bool clear = true)
		{
			Path = filepath;

			if (clear) m_Scene.DeleteAllEntities();
			if (!std::filesystem::exists(Path))
			{
				WC_CORE_ERROR("{} does not exist.", Path);
				return false;
			}

			YAML::Node data = YAML::LoadFile(Path);

			fromYAML(m_Scene, data);

			if (data["CameraFocalPoint"]) camera.FocalPoint = data["CameraFocalPoint"].as<glm::vec3>();
			if (data["CameraYaw"]) camera.Yaw = data["CameraYaw"].as<float>();
			if (data["CameraPitch"]) camera.Pitch = data["CameraPitch"].as<float>();
			if (data["CameraDistance"]) camera.m_Distance = data["CameraDistance"].as<float>();
			camera.UpdateView();
			return true;
		}

		// Undo/Redo
		template<typename T>
		T* PushCommand()
		{
			T* cmd = new T(); // call the constructor
			cmd->type = T::cmd_type;

			if (CommandBuffer.size() == 0)
			{
				WC_INFO("Should make a first snapshot");
			}

			if ((int32_t)CommandIndex < int32_t(CommandBuffer.size()) - 1)
			{
				for (size_t i = CommandIndex; i < CommandBuffer.size(); i++)
					delete CommandBuffer[i];

				CommandBuffer[CommandIndex] = cmd;
				CommandBuffer.resize(CommandIndex + 1);
			}
			else
			{
				CommandBuffer.push_back(cmd);
			}
			CommandIndex++;
			return cmd;
		}

		void Undo()
		{
			if (CommandIndex == 0) return;
			CommandIndex--;

			HandleChange();
		}

		void Redo()
		{
			if (CommandIndex >= CommandBuffer.size() - 1) return;
			CommandIndex++;

			HandleChange();
		}

		void HandleChange()
		{
			ICommand* command = (ICommand*)CommandBuffer[CommandIndex];

			WC_INFO("Command index: {}", CommandIndex);

			switch (command->type)
			{
			case CommandType::TransformChange:
				CMD_TransformChange& cmd = *(CMD_TransformChange*)command;
				cmd.Entity.set(cmd.Transform);
				break;
			}
		}

		void ChangeTransform(const flecs::entity& entity)
		{
			auto& cmd = *PushCommand<CMD_TransformChange>();
			cmd.Entity = entity;
			cmd.Transform = *entity.get<TransformComponent>();
		}
	};
}