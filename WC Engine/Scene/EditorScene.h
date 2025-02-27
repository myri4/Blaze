#pragma once

#include "Scene.h"

using namespace blaze;

enum class SceneState { Edit, Simulate, Play };

struct EditorScene
{
	// @NOTE: maybe we should move the entity ordering in this struct as this is a feature that is only being used by the editor
	Scene m_Scene;
	Scene m_TempScene;

	std::string Path;

	flecs::entity SelectedEntity = flecs::entity::null();

	SceneState State = SceneState::Edit;

	void CreatePhysicsWorld() { m_Scene.CreatePhysicsWorld(); }
	void Create() { m_Scene.Create(); }

	void Destroy() { m_Scene.Destroy(); }
	void DeleteAllEntities() { m_Scene.DeleteAllEntities(); }

	flecs::entity AddEntity() { return m_Scene.AddEntity(); }
	flecs::entity AddEntity(const std::string& name) { return m_Scene.AddEntity(name); }

	void CloneEntity(const flecs::entity& ent, const flecs::entity& ent2) { m_Scene.CloneEntity(ent, ent2); }
	void KillEntity(const flecs::entity& ent) { m_Scene.KillEntity(ent); }

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
			m_TempScene.Copy(m_Scene);
			Start();
		}
		else if (newState == SceneState::Edit)
		{
			Stop();
			m_Scene.Copy(m_TempScene);
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

	// @TODO: Move all YAML serialization functions to here
	YAML::Node SerializeEntity(const flecs::entity& entity) const { return m_Scene.SerializeEntity(entity); }
	flecs::entity DeserializeEntity(const YAML::Node& entityData) { return m_Scene.DeserializeEntity(entityData); }

	YAML::Node toYAML() const { return m_Scene.toYAML(); }
	void fromYAML(const YAML::Node& data) { m_Scene.fromYAML(data); }

	void Save() { Save(Path); }
	void Save(const std::string& filepath) { m_Scene.Save(filepath); }
	bool Load(const std::string& filepath, const bool clear = true) 
	{ 
		Path = filepath; 
		return m_Scene.Load(filepath, clear); 
	}

	void Copy(const Scene& FromWorld) { m_Scene.Copy(FromWorld); }
};