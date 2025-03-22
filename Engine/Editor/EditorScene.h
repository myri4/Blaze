#pragma once

#include "../UI/Widgets.h"

#include "../Scene/Scene.h"
#include "Commands.h"
#include "../Math/Camera.h"

#include "../Utils/YAML.h"

using namespace blaze;
using namespace wc;

namespace Editor
{
	enum class SceneState { Edit, Simulate, Play };

	inline TransformComponent GetEntityWorldTransform(const flecs::entity& entity)
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

		void CreatePhysicsWorld();
		void Create();

		void Destroy();
		void DeleteAllEntities();

		flecs::entity AddEntity();
		flecs::entity AddEntity(const std::string& name);

		void DuplicateEntity(const flecs::entity& ent);

		void KillEntity(const flecs::entity& ent);

		void RemoveChild(const flecs::entity& child);
		void SetChild(const flecs::entity& parent, const flecs::entity& child);

		void SetState(SceneState newState);

		void UpdatePhysics();
		void Update();

		YAML::Node ExportEntity(const flecs::entity& entity);
		flecs::entity LoadEntity(const YAML::Node& entityData);
		flecs::entity LoadEntity(const std::string& path);

		void Save();

		void Save(const std::string& filepath);

		bool Load(const std::string& filepath, const bool clear = true);

		// Undo/Redo
		template<typename T>
		T* PushCommand();

		void Undo();

		void Redo();

		void HandleChange();

		void ChangeTransform(const flecs::entity& entity);
	};
}