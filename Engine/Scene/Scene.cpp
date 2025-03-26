#include "Scene.h"

#include "../Globals.h"
#include "../Utils/YAML.h"

namespace blaze
{
	void SavePhysicsMaterials(const std::string& filepath)
	{
		YAML::Node data;
		for (auto& [name, id] : PhysicsMaterialNames)
		{
			if (name == "Default") continue;

			YAML::Node material;
			auto& physicsMat = PhysicsMaterials[id];
			material["Density"] = physicsMat.Density;
			material["Friction"] = physicsMat.Friction;
			material["Restitution"] = physicsMat.Restitution;
			material["RollingResistance"] = physicsMat.RollingResistance;

			material["DebugColor"] = physicsMat.DebugColor;

			material["Sensor"] = physicsMat.Sensor;
			material["EnableContactEvents"] = physicsMat.EnableContactEvents;
			material["EnableHitEvents"] = physicsMat.EnableHitEvents;
			material["EnablePreSolveEvents"] = physicsMat.EnablePreSolveEvents;
			material["InvokeContactCreation"] = physicsMat.InvokeContactCreation;
			material["UpdateBodyMass"] = physicsMat.UpdateBodyMass;

			data[name] = material;
		}

		YAMLUtils::SaveFile(filepath, data);
	}

	void LoadPhysicsMaterials(const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("{} does not exist", filepath);
			return;
		}

		YAML::Node data = YAML::LoadFile(filepath);
		if (data)
		{
			for (const auto& materialPair : data)
			{
				std::string materialName = materialPair.first.as<std::string>();
				const auto& materialData = materialPair.second;
				PhysicsMaterial material;
				material.Density = materialData["Density"].as<float>();
				material.Friction = materialData["Friction"].as<float>();
				material.Restitution = materialData["Restitution"].as<float>();
				material.RollingResistance = materialData["RollingResistance"].as<float>();

				material.DebugColor = materialData["DebugColor"].as<glm::vec4>();

				material.Sensor = materialData["Sensor"].as<bool>();
				material.EnableContactEvents = materialData["EnableContactEvents"].as<bool>();
				material.EnableHitEvents = materialData["EnableHitEvents"].as<bool>();
				material.EnablePreSolveEvents = materialData["EnablePreSolveEvents"].as<bool>();
				material.InvokeContactCreation = materialData["InvokeContactCreation"].as<bool>();
				material.UpdateBodyMass = materialData["UpdateBodyMass"].as<bool>();
				PhysicsMaterials.push_back(material);
				PhysicsMaterialNames[materialName] = PhysicsMaterials.size() - 1;
			}
		}
	}

	uint32_t LoadScriptBinary(const std::string& filepath, bool reload)
	{
		if (ScriptBinaryCache.find(filepath) != ScriptBinaryCache.end())
		{
			auto scriptID = ScriptBinaryCache[filepath];
			if (reload)
			{
				auto& script = ScriptBinaries[scriptID];
				script.VariableNames.clear();
				script.CompileScript(filepath);
			}

			return scriptID;
		}

		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("{} does not exist", filepath);
			return 0;
		}

		auto& binary = ScriptBinaries.emplace_back();
		binary.CompileScript(filepath);

		ScriptBinaryCache[filepath] = ScriptBinaries.size() - 1;

		return ScriptBinaries.size() - 1;
	}

	void Scene::Create()
	{
		PhysicsWorld.Create(b2DefaultWorldDef());

		/*EntityWorld.observer<BoxCollider2DComponent, RigidBodyComponent, TransformComponent>()
			.event(flecs::Monitor).each([&](flecs::iter& it, size_t i, BoxCollider2DComponent& collider, RigidBodyComponent& rigidBody, TransformComponent& transform) {
				if (it.event() == flecs::OnAdd)
				{
					b2BodyDef bodyDef = rigidBody.GetBodyDef();

					bodyDef.position = b2Vec2(transform.Translation.x + collider.Offset.x, transform.Translation.y + collider.Offset.y);
					bodyDef.rotation = b2MakeRot(transform.Rotation);
					rigidBody.body.Create(PhysicsWorld, bodyDef);
					collider.Shape.CreatePolygonShape(rigidBody.body, PhysicsMaterials[collider.MaterialID].GetShapeDef(), b2MakeBox(collider.Size.x * 0.5f, collider.Size.y * 0.5f));
				}
				else if (it.event() == flecs::OnSet)
				{
					rigidBody.body.SetTransform(glm::vec2(transform.Translation) + collider.Offset, b2MakeRot(transform.Rotation));
				}
				else if (it.event() == flecs::OnRemove)
				{
					rigidBody.body.Destroy();
				}
			});

		EntityWorld.observer<CircleCollider2DComponent, RigidBodyComponent, TransformComponent>()
			.event(flecs::Monitor).each([&](flecs::iter& it, size_t i, CircleCollider2DComponent& collider, RigidBodyComponent& rigidBody, TransformComponent& transform) {
				if (it.event() == flecs::OnAdd)
				{
					b2BodyDef bodyDef = rigidBody.GetBodyDef();

					bodyDef.position = b2Vec2(transform.Translation.x + collider.Offset.x, transform.Translation.y + collider.Offset.y);
					bodyDef.rotation = b2MakeRot(transform.Rotation);
					rigidBody.body.Create(PhysicsWorld, bodyDef);
					collider.Shape.CreateCircleShape(rigidBody.body, PhysicsMaterials[collider.MaterialID].GetShapeDef(), { {0.f, 0.f}, collider.Radius });
				}
				else if (it.event() == flecs::OnSet)
				{
					rigidBody.body.SetTransform(glm::vec2(transform.Translation) + collider.Offset, b2MakeRot(transform.Rotation));
				}
				else if (it.event() == flecs::OnRemove)
				{
					rigidBody.body.Destroy();
				}
			});*/
	}

	void Scene::CreatePhysicsWorld()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { PhysicsWorldData.Gravity.x, PhysicsWorldData.Gravity.y };
		PhysicsWorld.Create(worldDef);
		EntityWorld.each([this](flecs::entity entt, RigidBodyComponent& p, TransformComponent& pos) {
			if (p.body.IsValid()) return;
			b2BodyDef bodyDef = p.GetBodyDef();

			if (entt.has<BoxCollider2DComponent>())
			{
				auto collDef = entt.get_ref<BoxCollider2DComponent>();
				bodyDef.position = b2Vec2(pos.Translation.x + collDef->Offset.x, pos.Translation.y + collDef->Offset.y);
				bodyDef.rotation = b2MakeRot(pos.Rotation.z);
				p.body.Create(PhysicsWorld, bodyDef);
				collDef->Shape.CreatePolygonShape(p.body, PhysicsMaterials[collDef->MaterialID].GetShapeDef(), b2MakeBox(collDef->Size.x * 0.5f, collDef->Size.y * 0.5f));
			}

			if (entt.has<CircleCollider2DComponent>())
			{
				auto collDef = entt.get_ref<CircleCollider2DComponent>();
				bodyDef.position = b2Vec2(pos.Translation.x + collDef->Offset.x, pos.Translation.y + collDef->Offset.y);
				bodyDef.rotation = b2MakeRot(pos.Rotation.z);
				p.body.Create(PhysicsWorld, bodyDef);
				collDef->Shape.CreateCircleShape(p.body, PhysicsMaterials[collDef->MaterialID].GetShapeDef(), { {0.f, 0.f}, collDef->Radius });
			}
			p.prevPos = { bodyDef.position.x, bodyDef.position.y };
			p.previousRotation = pos.Rotation.z;
			});
	}

	void Scene::Destroy()
	{
		DeleteAllEntities();
		if (PhysicsWorld) PhysicsWorld.Destroy();
	}

	flecs::entity Scene::AddEntity() const { return EntityWorld.entity().add<EntityTag>(); }

	flecs::entity Scene::AddEntity(const std::string& name)
	{
		EntityOrder.push_back(name);
		return EntityWorld.entity(name.c_str()).add<EntityTag>();
	}

	void Scene::SetEntityName(const flecs::entity& entity, const std::string& name)
	{
		EntityOrder.push_back(name);
		entity.set_name(name.c_str());
	}

	void Scene::CopyEntity(const flecs::entity& ent, const flecs::entity& ent2) { ecs_clone(EntityWorld, ent2, ent, true); }
	flecs::entity Scene::CopyEntity(const flecs::entity& ent) { return flecs::entity(EntityWorld, ecs_clone(EntityWorld, 0, ent, true)); }

	void Scene::KillEntity(const flecs::entity& ent)
	{
		auto it = std::remove(EntityOrder.begin(), EntityOrder.end(), ent.name().c_str());
		if (it != EntityOrder.end())
			EntityOrder.erase(it, EntityOrder.end());

		auto parent = ent.parent();
		if (parent != flecs::entity::null() && parent.has<EntityOrderComponent>())
		{
			auto& order = parent.get_ref<EntityOrderComponent>()->EntityOrder;
			auto childIt = std::remove(order.begin(), order.end(), ent.name().c_str());
			if (childIt != order.end())
			{
				order.erase(childIt, order.end());

				if (order.empty())
					parent.remove<EntityOrderComponent>();
			}
		}

		ent.destruct();
	}

	void Scene::RemoveChild(const flecs::entity& child, bool changeChildTransform)
	{
		if (child.parent() == flecs::entity::null())
			return;

		auto parent = child.parent();

		if (changeChildTransform)
			if (parent.has<TransformComponent>() && child.has<TransformComponent>())
			{
				auto parentTransform = parent.get_ref<TransformComponent>();
				auto& childTransform = *child.get_mut<TransformComponent>();
				childTransform.Translation += parentTransform->Translation;
				childTransform.Scale *= parentTransform->Scale;
			}

		child.remove(flecs::ChildOf, parent);

		auto orderData = parent.get_ref<EntityOrderComponent>();

		if (orderData)
		{
			auto& order = orderData->EntityOrder;
			auto it = std::remove(order.begin(), order.end(), child.name().c_str());
			if (it != order.end())
			{
				order.erase(it, order.end());

				if (order.empty())
					parent.remove<EntityOrderComponent>();
			}
		}
		EntityOrder.push_back(std::string(child.name().c_str()));
	}

	void Scene::SetChild(const flecs::entity& parent, const flecs::entity& child, bool changeChildTransform)
	{
		if (child.parent() != flecs::entity::null())
			RemoveChild(child);

		parent.add<EntityOrderComponent>();
		parent.get_ref<EntityOrderComponent>()->EntityOrder.push_back(child.name().c_str());
		child.add(flecs::ChildOf, parent);

		if (changeChildTransform)
			if (parent.has<TransformComponent>() && child.has<TransformComponent>())
			{
				auto parentTransform = parent.get_ref<TransformComponent>();
				auto& childTransform = *child.get_mut<TransformComponent>();
				childTransform.Translation -= parentTransform->Translation;
				childTransform.Scale /= parentTransform->Scale;
			}

		EntityOrder.erase(std::remove(EntityOrder.begin(), EntityOrder.end(), child.name().c_str()), EntityOrder.end());
	}

	void Scene::DeleteAllEntities()
	{
		EntityWorld.reset();
		EntityOrder.clear();
	}

	void Scene::UpdatePhysics()
	{
		PhysicsWorld.SetGravity(PhysicsWorldData.Gravity);

		AccumulatedTime += wc::Globals.deltaTime;

		EntityWorld.each([](RigidBodyComponent& p, TransformComponent& pc) { // @TODO: Maybe optimize this?
			if (!p.body.IsValid())
			{
				WC_CORE_ERROR("Body is invalid!");
				return;
			}

			p.prevPos = p.body.GetPosition();
			p.previousRotation = p.body.GetAngle();
			});

		while (AccumulatedTime >= SimulationTime)
		{
			PhysicsWorld.Step(SimulationTime, 4);

			AccumulatedTime -= SimulationTime;
		}

		float alpha = AccumulatedTime / SimulationTime;

		EntityWorld.each([&alpha](RigidBodyComponent& p, TransformComponent& pc)
			{
				if (!p.body.IsValid())
				{
					WC_CORE_ERROR("Body is invalid!");
					return;
				}
				pc.Translation = { glm::mix(p.prevPos, p.body.GetPosition(), alpha), pc.Translation.z };

				float start = p.previousRotation;
				float end = p.body.GetAngle();
				//auto diff = end - start;

				pc.Rotation.z = glm::mix(start, end, alpha);
			});
	}

	void Scene::Update()
		{
			EntityWorld.each([](ScriptComponent& script)
				{
					if (script.ScriptInstance)
						script.ScriptInstance.state.Execute("Update");
				});

			UpdatePhysics();
		}
}