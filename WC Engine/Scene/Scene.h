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

	struct Scene
	{
	private:
		flecs::world m_World;
		b2World m_PhysicsWorld;
		PhysicsWorldData m_PhysicsWorldData;

		std::vector<std::string> m_ParentEntityNames; // only parents

		float AccumulatedTime = 0.f;
		const float SimulationTime = 1.f / 60.f; // @NOTE: maybe should expose this as an option

	public:
	    std::unordered_map<std::string, PhysicsMaterial> Materials = {{ "Default", {} }};
		auto GetWorld()	{ return m_World; }
		auto GetPhysicsWorld() { return m_PhysicsWorld; }
		auto& GetPhysicsWorldData() { return m_PhysicsWorldData; }
		auto& GetParentEntityNames() { return m_ParentEntityNames; }

		auto AddEntity() {	return m_World.entity().add<EntityTag>();	}

		auto AddEntity(const std::string& name) { m_ParentEntityNames.push_back(name); return m_World.entity(name.c_str()).add<EntityTag>(); }
		
		void RemoveChild(flecs::entity& child)
		{
		    if (child.parent() == flecs::entity::null()) return;

		    auto parent = child.parent();
			child.remove(flecs::ChildOf, parent);

			auto names = parent.get_ref<ChildNamesComponent>();

			if (names)
			{
				auto& childNames = names->childNames;
				auto it = std::remove(childNames.begin(), childNames.end(), child.name().c_str());
				if (it != childNames.end())
				{
					childNames.erase(it, childNames.end());

					// If it was the last child, remove the ChildNamesComponent from the parent
					if (childNames.empty())
					{
						parent.remove<ChildNamesComponent>();
					}
				}
			}

			// Add the child's name back to m_EntityNames
			m_ParentEntityNames.push_back(child.name().c_str());
		}

		void SetChild(flecs::entity& parent, flecs::entity& child)
		{
			// Check if the child already has a parent
			if (child.parent() != flecs::entity::null())
			{
				// Remove the child from the current parent
				RemoveChild(child);
			}

			parent.add<ChildNamesComponent>();
			parent.get_ref<ChildNamesComponent>()->childNames.push_back(child.name().c_str());
			child.add(flecs::ChildOf, parent);

			// Remove the child's name from m_ParentEntityNames
			m_ParentEntityNames.erase(std::remove(m_ParentEntityNames.begin(), m_ParentEntityNames.end(), child.name().c_str()), m_ParentEntityNames.end());
		}

		void CloneEntity(flecs::entity& ent, flecs::entity& ent2) { ecs_clone(m_World, ent2, ent, true); }

		void KillEntity(flecs::entity& ent)
		{
			// Remove the entity's name from m_EntityNames
			auto it = std::remove(m_ParentEntityNames.begin(), m_ParentEntityNames.end(), ent.name().c_str());
			if (it != m_ParentEntityNames.end()) {
				m_ParentEntityNames.erase(it, m_ParentEntityNames.end());
			}

			// Check if the entity has a parent
			auto parent = ent.parent();
			if (parent != flecs::entity::null() && parent.has<ChildNamesComponent>())
			{
				// Remove the child's name from the parent's ChildNamesComponent
				auto& childNames = parent.get_ref<ChildNamesComponent>()->childNames;
				auto childIt = std::remove(childNames.begin(), childNames.end(), ent.name().c_str());
				if (childIt != childNames.end()) 
				{
					childNames.erase(childIt, childNames.end());

					// If it was the last child, remove the ChildNamesComponent from the parent
					if (childNames.empty()) 
						parent.remove<ChildNamesComponent>();					
				}
			}

			// Destruct the entity
			ent.destruct();
		}

		void CreatePhysicsWorld()
		{
			b2WorldDef worldDef = b2DefaultWorldDef();
			worldDef.gravity = { m_PhysicsWorldData.Gravity.x, m_PhysicsWorldData.Gravity.y };
			m_PhysicsWorld.Create(worldDef);
			m_World.each([this](flecs::entity entt, RigidBodyComponent& p, TransformComponent& pos) {
				if (p.body.IsValid()) return;
				b2BodyDef bodyDef = p.GetBodyDef();

				if (entt.has<BoxCollider2DComponent>())
				{
					auto collDef = entt.get_ref<BoxCollider2DComponent>();
					bodyDef.position = b2Vec2(pos.Translation.x + collDef->Offset.x, pos.Translation.y + collDef->Offset.y);
					bodyDef.rotation = b2MakeRot(pos.Rotation);
					p.body.Create(m_PhysicsWorld, bodyDef);
					collDef->Shape.CreatePolygonShape(p.body, collDef->Material.GetShapeDef(), b2MakeBox(collDef->Size.x * 0.5f, collDef->Size.y * 0.5f));
				}

				if (entt.has<CircleCollider2DComponent>())
				{
					auto collDef = entt.get_ref<CircleCollider2DComponent>();
					bodyDef.position = b2Vec2(pos.Translation.x + collDef->Offset.x, pos.Translation.y + collDef->Offset.y);
					bodyDef.rotation = b2MakeRot(pos.Rotation);
					p.body.Create(m_PhysicsWorld, bodyDef);
					collDef->Shape.CreateCircleShape(p.body, collDef->Material.GetShapeDef(), { {0.f, 0.f}, collDef->Radius });
				}
				p.prevPos = { bodyDef.position.x, bodyDef.position.y };
				p.previousRotation = pos.Rotation;
				});
		}

		void DestroyPhysicsWorld()
		{
			m_PhysicsWorld.Destroy();
		}

		void UpdatePhysics()
		{
		    m_PhysicsWorld.SetGravity(m_PhysicsWorldData.Gravity);

			AccumulatedTime += Globals.deltaTime;

			while (AccumulatedTime >= SimulationTime)
			{
				m_World.each([this](RigidBodyComponent& p, TransformComponent& pc) { // @TODO: Maybe optimize this?
					p.prevPos = p.body.GetPosition();
					p.previousRotation = p.body.GetAngle();
					});

				m_PhysicsWorld.Step(SimulationTime, 4);

				AccumulatedTime -= SimulationTime;
			}

			float alpha = AccumulatedTime / SimulationTime;

			m_World.each([this, &alpha](RigidBodyComponent& p, TransformComponent& pc)
				{
					pc.Translation = {glm::mix(p.prevPos, p.body.GetPosition(), alpha), pc.Translation.z};
					pc.Rotation = glm::mix(p.previousRotation, p.body.GetAngle(), alpha);
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
				auto component = entity.get<TransformComponent>();
				YAML::Node componentData;
				componentData["Translation"] = component->Translation;
				componentData["Scale"] = component->Scale;
				componentData["Rotation"] = glm::degrees(component->Rotation);
				entityData["TransformComponent"] = componentData;
			}

			if (entity.has<TextRendererComponent>())
			{
				auto component = entity.get<TextRendererComponent>();
				YAML::Node componentData;
				componentData["Text"] = component->Text;
				//componentData["Font"] = component->Font;		// @TODO: add support for serializing Font type
				componentData["Color"] = component->Color;
				componentData["Kerning"] = component->Kerning;
				componentData["LineSpacing"] = component->LineSpacing;
				entityData["TextRendererComponent"] = componentData;
			}

			if (entity.has<SpriteRendererComponent>())
			{
				auto component = entity.get<SpriteRendererComponent>();
				YAML::Node componentData;
				componentData["Color"] = component->Color;
				componentData["Texture"] = component->Texture;  // @TODO: this should be in text form
				entityData["SpriteRendererComponent"] = componentData;
			}

			if (entity.has<CircleRendererComponent>())
			{
				auto component = entity.get<CircleRendererComponent>();
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
				auto component = entity.get<BoxCollider2DComponent>();
				YAML::Node componentData;
				componentData["Offset"] = component->Offset;
				componentData["Size"] = component->Size;

				componentData["Density"] = component->Material.Density;
				componentData["Friction"] = component->Material.Friction;
				componentData["Restitution"] = component->Material.Restitution;
				componentData["RollingResistance"] = component->Material.RollingResistance;

				componentData["DebugColor"] = component->Material.DebugColor;

				componentData["Sensor"] = component->Material.Sensor;
				componentData["EnableContactEvents"] = component->Material.EnableContactEvents;
				componentData["EnableHitEvents"] = component->Material.EnableHitEvents;
				componentData["EnablePreSolveEvents"] = component->Material.EnablePreSolveEvents;
				componentData["InvokeContactCreation"] = component->Material.InvokeContactCreation;
				componentData["UpdateBodyMass"] = component->Material.UpdateBodyMass;

				entityData["BoxCollider2DComponent"] = componentData;
			}

			if (entity.has<CircleCollider2DComponent>())
			{
				auto component = entity.get<CircleCollider2DComponent>();
				YAML::Node componentData;
				componentData["Offset"] = component->Offset;
				componentData["Radius"] = component->Radius;

				componentData["Density"] = component->Material.Density;
				componentData["Friction"] = component->Material.Friction;
				componentData["Restitution"] = component->Material.Restitution;
				componentData["RollingResistance"] = component->Material.RollingResistance;

				componentData["DebugColor"] = component->Material.DebugColor;

				componentData["Sensor"] = component->Material.Sensor;
				componentData["EnableContactEvents"] = component->Material.EnableContactEvents;
				componentData["EnableHitEvents"] = component->Material.EnableHitEvents;
				componentData["EnablePreSolveEvents"] = component->Material.EnablePreSolveEvents;
				componentData["InvokeContactCreation"] = component->Material.InvokeContactCreation;
				componentData["UpdateBodyMass"] = component->Material.UpdateBodyMass;

				entityData["CircleCollider2DComponent"] = componentData;
			}
		}

		void SerializeChildEntity(flecs::entity& parent, YAML::Node& entityData) const
		{
			YAML::Node childrenData;

			m_World.query_builder<EntityTag>()
				.with(flecs::ChildOf, parent)
				.each([&](flecs::entity child, EntityTag) {
				YAML::Node childData;
				
				SerializeEntity(child, childData);

				SerializeChildEntity(child, childData);

				childrenData.push_back(childData);
			});

			if (childrenData.size() != 0) 
				entityData["Children"] = childrenData;
		}

		YAML::Node toYAML() const
		{
			YAML::Node metaData;
			YAML::Node entitiesData;

			m_World.each([&](flecs::entity entity, EntityTag) {
				if (entity.parent() != 0) return;

				YAML::Node entityData;

				SerializeEntity(entity, entityData);

				SerializeChildEntity(entity, entityData);

				entitiesData.push_back(entityData);
				});

			if (m_PhysicsWorld.IsValid())
			{
				metaData["Gravity"] = m_PhysicsWorld.GetGravity();
			}
			metaData["Entities"] = entitiesData;
			return metaData;
		}

		void Save(const std::string& filepath) { YAMLUtils::SaveFile(filepath, toYAML()); }

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

		void DeserializeEntity(flecs::entity& deserializedEntity, const YAML::Node& entity)
		{
			std::string name = entity["Name"].as<std::string>();
			std::string id = entity["ID"].as<std::string>();

			WC_CORE_INFO("Deserialized entity with name = {}, ID = {}", name, id);

			if (name == "null[5ws78@!12]") deserializedEntity = AddEntity("null");
			else if (name != "null") deserializedEntity = AddEntity(name);
			else deserializedEntity = AddEntity();

			auto transformComponent = entity["TransformComponent"];
			if (transformComponent)
			{
				deserializedEntity.set<TransformComponent>({
					transformComponent["Translation"].as<glm::vec3>(),
					transformComponent["Scale"].as<glm::vec2>(),
					glm::radians(transformComponent["Rotation"].as<float>())
					});
			}
			
			auto textRendererComponent = entity["TextRendererComponent"];
			if (textRendererComponent)
			{
				deserializedEntity.set<TextRendererComponent>({
					textRendererComponent["Text"].as<std::string>(),
					//textRendererComponent["Font"].as<Font>(),					// @TODO: add support for deserializing Font type
					//textRendererComponent["Color"].as<glm::vec4>()
					});
			}

			auto spriteRendererComponent = entity["SpriteRendererComponent"];
			if (spriteRendererComponent)
			{
				deserializedEntity.set<SpriteRendererComponent>({
					spriteRendererComponent["Color"].as<glm::vec4>(),
					spriteRendererComponent["Texture"].as<uint32_t>()
					});
			}

			auto circleRendererComponent = entity["CircleRendererComponent"];
			if (circleRendererComponent)
			{
				deserializedEntity.set<CircleRendererComponent>({
					circleRendererComponent["Color"].as<glm::vec4>(),
					circleRendererComponent["Thickness"].as<float>(),
					circleRendererComponent["Fade"].as<float>()
					});
			}
			
			auto rigidBodyComponent = entity["RigidBodyComponent"];
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
				deserializedEntity.set<RigidBodyComponent>(component);
			}
			
			{
				auto componentData = entity["BoxCollider2DComponent"];
				if (componentData)
				{
					BoxCollider2DComponent component;
					component.Offset = componentData["Offset"].as<glm::vec2>();
					component.Size = componentData["Size"].as<glm::vec2>();

					component.Material.Density = componentData["Density"].as<float>();
					component.Material.Friction = componentData["Friction"].as<float>();
					component.Material.Restitution = componentData["Restitution"].as<float>();
					component.Material.RollingResistance = componentData["RollingResistance"].as<float>();

					component.Material.DebugColor = componentData["DebugColor"].as<glm::vec4>();

					component.Material.Sensor = componentData["Sensor"].as<bool>();
					component.Material.EnableContactEvents = componentData["EnableContactEvents"].as<bool>();
					component.Material.EnableHitEvents = componentData["EnableHitEvents"].as<bool>();
					component.Material.EnablePreSolveEvents = componentData["EnablePreSolveEvents"].as<bool>();
					component.Material.InvokeContactCreation = componentData["InvokeContactCreation"].as<bool>();
					component.Material.UpdateBodyMass = componentData["UpdateBodyMass"].as<bool>();

					deserializedEntity.set<BoxCollider2DComponent>(component);
				}
			}

			{
				auto componentData = entity["CircleCollider2DComponent"];
				if (componentData)
				{
					CircleCollider2DComponent component;
					component.Offset = componentData["Offset"].as<glm::vec2>();
					component.Radius = componentData["Radius"].as<float>();

					component.Material.Density = componentData["Density"].as<float>();
					component.Material.Friction = componentData["Friction"].as<float>();
					component.Material.Restitution = componentData["Restitution"].as<float>();
					component.Material.RollingResistance = componentData["RollingResistance"].as<float>();

					component.Material.DebugColor = componentData["DebugColor"].as<glm::vec4>();

					component.Material.Sensor = componentData["Sensor"].as<bool>();
					component.Material.EnableContactEvents = componentData["EnableContactEvents"].as<bool>();
					component.Material.EnableHitEvents = componentData["EnableHitEvents"].as<bool>();
					component.Material.EnablePreSolveEvents = componentData["EnablePreSolveEvents"].as<bool>();
					component.Material.InvokeContactCreation = componentData["InvokeContactCreation"].as<bool>();
					component.Material.UpdateBodyMass = componentData["UpdateBodyMass"].as<bool>();

					deserializedEntity.set<CircleCollider2DComponent>(component);
				}
			}

			auto childEntities = entity["Children"];
			if (childEntities)
			{
				for (const auto& child : childEntities)
				{
					flecs::entity deserializedChildEntity;

					DeserializeEntity(deserializedChildEntity, child);
					
					SetChild(deserializedEntity, deserializedChildEntity);
				}
			}
		}

	    void DeleteAllEntities()
		{
		    m_World.each([](flecs::entity entity, EntityTag) {
                entity.destruct();
                });
		    m_ParentEntityNames.clear();
		}

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