#pragma once

#include "flecs.h"
#include "Components.h"
#include "../Globals.h"
#include <wc/Utils/YAML.h>

namespace wc
{
	struct PhysicsWorldData
	{
		glm::vec2 Gravity = { 0.f, -9.8f };
		float TimeStep = 1.f / 60.f;
	};

	inline std::unordered_map<std::string, uint32_t> PhysicsMaterialNames = { { "Default", 0 } };
	inline std::vector<PhysicsMaterial> PhysicsMaterials;
	inline AssetManager assetManager;

	inline void SavePhysicsMaterials(const std::string& filepath)
	{
		YAML::Node data;
		for (auto& [name, id] : PhysicsMaterialNames)
		{
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

	inline void LoadPhysicsMaterials(const std::string& filepath)
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

	struct Scene
	{
		flecs::world EntityWorld;
		b2World PhysicsWorld;
		PhysicsWorldData PhysicsWorldData;

		std::vector<std::string> EntityOrder; // only parents

		float AccumulatedTime = 0.f;
		const float SimulationTime = 1.f / 60.f; // @NOTE: maybe should expose this as an option


		auto AddEntity() { return EntityWorld.entity().add<EntityTag>(); }

		auto AddEntity(const std::string& name)
		{
			EntityOrder.push_back(name);
			return EntityWorld.entity(name.c_str()).add<EntityTag>();
		}

		void CloneEntity(flecs::entity& ent, flecs::entity& ent2) { ecs_clone(EntityWorld, ent2, ent, true); }

		void KillEntity(flecs::entity& ent)
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

		void RemoveChild(flecs::entity& child)
		{
			if (child.parent() == flecs::entity::null())
				return;

			auto parent = child.parent();

			{
				if (parent.has<TransformComponent>() && child.has<TransformComponent>())
				{
					auto parentTransform = parent.get_ref<TransformComponent>();
					auto& childTransform = *child.get_mut<TransformComponent>();
					childTransform.Translation += parentTransform->Translation;
					childTransform.Scale *= parentTransform->Scale;
				}
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

			EntityOrder.push_back(child.name().c_str());
		}

		void SetChild(flecs::entity& parent, flecs::entity& child)
		{
			if (child.parent() != flecs::entity::null())
				RemoveChild(child);

			parent.add<EntityOrderComponent>();
			parent.get_ref<EntityOrderComponent>()->EntityOrder.push_back(child.name().c_str());
			child.add(flecs::ChildOf, parent);

			if (parent.has<TransformComponent>() && child.has<TransformComponent>())
			{
				auto parentTransform = parent.get_ref<TransformComponent>();
				auto& childTransform = *child.get_mut<TransformComponent>();
				childTransform.Translation -= parentTransform->Translation;
				childTransform.Scale /= parentTransform->Scale;
			}

			EntityOrder.erase(std::remove(EntityOrder.begin(), EntityOrder.end(), child.name().c_str()), EntityOrder.end());
		}

		void CreatePhysicsWorld()
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
					bodyDef.rotation = b2MakeRot(pos.Rotation);
					p.body.Create(PhysicsWorld, bodyDef);
					collDef->Shape.CreatePolygonShape(p.body, PhysicsMaterials[collDef->MaterialID].GetShapeDef(), b2MakeBox(collDef->Size.x * 0.5f, collDef->Size.y * 0.5f));
				}

				if (entt.has<CircleCollider2DComponent>())
				{
					auto collDef = entt.get_ref<CircleCollider2DComponent>();
					bodyDef.position = b2Vec2(pos.Translation.x + collDef->Offset.x, pos.Translation.y + collDef->Offset.y);
					bodyDef.rotation = b2MakeRot(pos.Rotation);
					p.body.Create(PhysicsWorld, bodyDef);
					collDef->Shape.CreateCircleShape(p.body, PhysicsMaterials[collDef->MaterialID].GetShapeDef(), { {0.f, 0.f}, collDef->Radius });
				}
				p.prevPos = { bodyDef.position.x, bodyDef.position.y };
				p.previousRotation = pos.Rotation;
				});
		}

		void Destroy()
		{
			if (PhysicsWorld) PhysicsWorld.Destroy();
			DeleteAllEntities();
		}

		void UpdatePhysics()
		{
			PhysicsWorld.SetGravity(PhysicsWorldData.Gravity);

			AccumulatedTime += Globals.deltaTime;

			while (AccumulatedTime >= SimulationTime)
			{
				EntityWorld.each([this](RigidBodyComponent& p, TransformComponent& pc) { // @TODO: Maybe optimize this?
					p.prevPos = p.body.GetPosition();
					p.previousRotation = p.body.GetAngle();
					});

				PhysicsWorld.Step(SimulationTime, 4);

				AccumulatedTime -= SimulationTime;
			}

			float alpha = AccumulatedTime / SimulationTime;

			EntityWorld.each([this, &alpha](RigidBodyComponent& p, TransformComponent& pc)
				{
					pc.Translation = { glm::mix(p.prevPos, p.body.GetPosition(), alpha), pc.Translation.z };

					float start = p.previousRotation;
					float end = p.body.GetAngle();
					auto diff = end - start;

					pc.Rotation = glm::mix(start, end, alpha);
				});
		}

		void Update()
		{
			UpdatePhysics();
		}

		void SerializeEntity(flecs::entity& entity, YAML::Node& entityData) const
		{
			if (entity.name() != "null") entityData["Name"] = std::string(entity.name().c_str());
			else entityData["Name"] = std::string("null[5ws78@!12]");
			entityData["ID"] = entity.id();

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
					WC_CORE_INFO(name);
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

				for (const auto& [name, matID] : PhysicsMaterialNames) // ew, but there's no other way ig
				{
					if (matID == component->MaterialID)
					{
						componentData["PhysicsMaterial"] = name;
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

				for (const auto& [name, matID] : PhysicsMaterialNames) // ew, but there's no other way ig
				{
					if (matID == component->MaterialID)
					{
						componentData["PhysicsMaterial"] = name;
						break;
					}
				}

				entityData["CircleCollider2DComponent"] = componentData;
			}
		}

		void SerializeChildEntity(flecs::entity& parent, YAML::Node& entityData) const
		{
			if (!parent.has<EntityOrderComponent>()) return;
			YAML::Node childrenData;

			/*EntityWorld.query_builder<EntityTag>()
			.with(flecs::ChildOf, parent)
			.each([&](flecs::entity child, EntityTag) {
				YAML::Node childData;

				SerializeEntity(child, childData);

				SerializeChildEntity(child, childData);

				childrenData.push_back(childData);
			});*/

			for (const auto& name : parent.get_ref<EntityOrderComponent>()->EntityOrder)
			{
				auto child = EntityWorld.lookup(name.c_str());

				YAML::Node childData;

				SerializeEntity(child, childData);

				SerializeChildEntity(child, childData);

				childrenData.push_back(childData);
			}			

			if (childrenData.size() != 0)
				entityData["Children"] = childrenData;
		}

		YAML::Node toYAML() const
		{
			YAML::Node metaData;
			YAML::Node entitiesData;

			for (auto& name : EntityOrder)
			{
				auto entity = EntityWorld.lookup(name.c_str());

				YAML::Node entityData;

				SerializeEntity(entity, entityData);

				SerializeChildEntity(entity, entityData);

				entitiesData.push_back(entityData);
			}

			if (PhysicsWorld.IsValid())
			{
				metaData["Gravity"] = PhysicsWorld.GetGravity();
			}

			if (entitiesData) 
				metaData["Entities"] = entitiesData;

			return metaData;
		}

		void fromYAML(const YAML::Node& data)
		{
			auto entities = data["Entities"];
			if (entities)
			{
				for (const auto& entity : entities)
				{
					flecs::entity deserializedEntity;

					DeserializeEntity(deserializedEntity, entity);
				}
			}
		}

		void DeserializeEntity(flecs::entity& entity, const YAML::Node& entityData)
		{
			std::string name = entityData["Name"].as<std::string>();
			std::string id = entityData["ID"].as<std::string>();

			WC_CORE_INFO("Deserialized entity with name = {}, ID = {}", name, id);

			if (name == "null[5ws78@!12]") entity = AddEntity("null");
			else if (name != "null") entity = AddEntity(name);
			else entity = AddEntity();

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
					component.MaterialID = PhysicsMaterialNames[componentData["PhysicsMaterial"].as<std::string>()];

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
					component.MaterialID = PhysicsMaterialNames[componentData["PhysicsMaterial"].as<std::string>()];					

					entity.set<CircleCollider2DComponent>(component);
				}
			}

			auto childEntities = entityData["Children"];
			if (childEntities)
			{
				for (const auto& child : childEntities)
				{
					flecs::entity deserializedChildEntity;

					DeserializeEntity(deserializedChildEntity, child);

					SetChild(entity, deserializedChildEntity);
				}
			}
		}

		void DeleteAllEntities()
		{
			EntityWorld.reset();
			EntityOrder.clear();
		}

		void Save(const std::string& filepath) { YAMLUtils::SaveFile(filepath, toYAML()); }

		bool Load(const std::string& filepath, const bool clear = true)
		{
			if (clear) DeleteAllEntities();
			if (!std::filesystem::exists(filepath))
			{
				WC_CORE_ERROR("{} does not exist.", filepath);
				return false;
			}

			fromYAML(YAML::LoadFile(filepath));
			return true;
		}

		void Copy(const Scene& FromWorld)
		{
			DeleteAllEntities();
			fromYAML(FromWorld.toYAML());
		}
	};
}