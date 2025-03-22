#pragma once

#include "flecs.h"
#include "Components.h"
#include "../Rendering/AssetManager.h"

namespace blaze
{
	struct PhysicsWorldData
	{
		glm::vec2 Gravity = { 0.f, -9.8f };
		float TimeStep = 1.f / 60.f;
	};

	using Cache = std::unordered_map<std::string, uint32_t>;

	template<typename T>
	using Storage = std::vector<T>;

	inline AssetManager assetManager;

	inline Storage<PhysicsMaterial> PhysicsMaterials;
	inline Cache PhysicsMaterialNames;

	inline Storage<ScriptBinary> ScriptBinaries;
	inline Cache ScriptBinaryCache;

	void SavePhysicsMaterials(const std::string& filepath);

	void LoadPhysicsMaterials(const std::string& filepath);

	uint32_t LoadScriptBinary(const std::string& filepath, bool reload = false);

	struct Scene
	{
		flecs::world EntityWorld;
		b2::World PhysicsWorld;
		PhysicsWorldData PhysicsWorldData;

		std::vector<std::string> EntityOrder; // only parents

		float AccumulatedTime = 0.f;
		const float SimulationTime = 1.f / 60.f;

		void Create();

		void CreatePhysicsWorld();

		void Destroy();

		flecs::entity AddEntity() const;

		flecs::entity AddEntity(const std::string& name);

		void SetEntityName(const flecs::entity& entity, const std::string& name);

		void CopyEntity(const flecs::entity& ent, const flecs::entity& ent2);
		flecs::entity CopyEntity(const flecs::entity& ent);

		void KillEntity(const flecs::entity& ent);

		void RemoveChild(const flecs::entity& child, bool changeChildTransform = true);

		void SetChild(const flecs::entity& parent, const flecs::entity& child, bool changeChildTransform = true);

		void DeleteAllEntities();

		void UpdatePhysics();

		void Update();
	};
}