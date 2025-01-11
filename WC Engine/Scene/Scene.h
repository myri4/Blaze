#pragma once

#include "flecs.h"
#include "Components.h"
#include "../Globals.h"
#include <wc/Utils/YAML.h>

namespace wc
{
	struct Scene
	{
	private:
		flecs::world m_World;
		b2World m_PhysicsWorld;

		float AccumulatedTime = 0.f;
		const float SimulationTime = 1.f / 60.f; // @NOTE: maybe should expose this as an option

	public:
		auto GetWorld()	{ return m_World; }

		auto AddEntity() {	return m_World.entity().add<EntityTag>();	}

		auto AddEntity(const std::string& name) { return m_World.entity(name.c_str()).add<EntityTag>(); }

		void KillEntity(flecs::entity& ent)	{ ent.destruct(); }

		void CreatePhysicsWorld()
		{
			b2WorldDef worldDef = b2DefaultWorldDef();
			worldDef.gravity = b2Vec2{ 0.0f, -9.8f };
			m_PhysicsWorld.Create(worldDef);

			m_World.each([this](PhysicsComponent& p, TransformComponent& pos) {
				b2BodyDef bodyDef = b2DefaultBodyDef();
				bodyDef.type = b2_dynamicBody;
				bodyDef.position = b2Vec2{ pos.position.x, pos.position.y };
				p.body.Create(m_PhysicsWorld, bodyDef);

				b2Polygon dynamicBox = b2MakeBox(0.5f, 0.5f);

				b2ShapeDef shapeDef = b2DefaultShapeDef();
				shapeDef.density = 1.0f;
				shapeDef.friction = 0.3f;

				b2Shape shape;
				shape.CreatePolygonShape(p.body, shapeDef, dynamicBox);
			});
		}

		void UpdatePhysics()
		{
			AccumulatedTime += Globals.deltaTime;

			while (AccumulatedTime >= SimulationTime)
			{
				m_World.each([this](PhysicsComponent& p) {
					p.prevPos = p.body.GetPosition();
					});

				m_PhysicsWorld.Step(SimulationTime, 4);

				AccumulatedTime -= SimulationTime;
			}

			float alpha = AccumulatedTime / SimulationTime;

			m_World.each([this, &alpha](PhysicsComponent& p, TransformComponent& pc)
				{
					pc.position = glm::mix(p.prevPos, p.body.GetPosition(), alpha);
				});
		}

		void Update()
		{
			UpdatePhysics();
		}

		void SerializeEntity(flecs::entity& entity, YAML::Node& entityData)
		{
			if (entity.name() != "null") entityData["Name"] = std::string(entity.name().c_str());
			else entityData["Name"] = std::string("null[5ws78@!12]");
			entityData["ID"] = entity.id();

			if (entity.has<TransformComponent>())
			{
				YAML::Node componentData;
				componentData["Position"] = entity.get<TransformComponent>()->position;
				componentData["Scale"] = entity.get<TransformComponent>()->scale;
				componentData["Rotation"] = entity.get<TransformComponent>()->rotation;
				entityData["TransformComponent"] = componentData;
			}

			if (entity.has<TextRendererComponent>())
			{
				YAML::Node componentData;
				componentData["Text"] = entity.get<TextRendererComponent>()->Text;
				//componentData["Font"] = entity.get<TextRendererComponent>()->Font;		// @TODO: add support for serializing Font type
				componentData["Color"] = entity.get<TextRendererComponent>()->Color;
				entityData["TextRendererComponent"] = componentData;
			}

			if (entity.has<SpriteRendererComponent>())
			{
				YAML::Node componentData;
				componentData["Color"] = entity.get<SpriteRendererComponent>()->Color;
				componentData["Texture"] = entity.get<SpriteRendererComponent>()->Texture;  // @TODO: this should be in text form
				entityData["SpriteRendererComponent"] = componentData;
			}

			if (entity.has<CircleRendererComponent>())
			{
				YAML::Node componentData;
				componentData["Color"] = entity.get<CircleRendererComponent>()->Color;
				componentData["Thickness"] = entity.get<CircleRendererComponent>()->Thickness;
				componentData["Fade"] = entity.get<CircleRendererComponent>()->Fade;
				entityData["CircleRendererComponent"] = componentData;
			}

			if (entity.has<PhysicsComponent>())
			{
				YAML::Node componentData;
				//componentData["Body"] = entity.get<PhysicsComponent>()->body;				// @TODO: add support for serializing b2Body type
				componentData["PreviousPosition"] = entity.get<PhysicsComponent>()->prevPos;
				entityData["PhysicsComponent"] = componentData;
			}
		}

		void SerializeChildEntity(flecs::entity& parent, YAML::Node& entityData)
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

			if (childrenData.size() != 0) entityData["Children"] = childrenData;
		}

		void Save(const std::string& filepath)
		{
			YAML::Node metaData;
			YAML::Node entitiesData;

			m_World.each([&](flecs::entity entity, EntityTag) {
				if (entity.parent() == 0)
				{
					YAML::Node entityData;

					SerializeEntity(entity, entityData);

					SerializeChildEntity(entity, entityData);

					entitiesData.push_back(entityData);
				}
			});

			metaData["Entities"] = entitiesData;

			YAMLUtils::SaveFile(filepath, metaData);
		}

		void DeserializeEntity(flecs::entity& deserializedEntity, const YAML::detail::iterator_value& entity)
		{
			std::string name = entity["Name"].as<std::string>();
			std::string id = entity["ID"].as<std::string>();

			WC_CORE_TRACE("Deserialized entity with name = {}, ID = {}", name, id);

			if (name == "null[5ws78@!12]") deserializedEntity = AddEntity("null");
			else if (name != "null") deserializedEntity = AddEntity(name);
			else deserializedEntity = AddEntity();

			auto transformComponent = entity["TransformComponent"];
			if (transformComponent)
			{
				deserializedEntity.set<TransformComponent>({
					transformComponent["Position"].as<glm::vec2>(),
					transformComponent["Scale"].as<glm::vec2>(),
					transformComponent["Rotation"].as<float>()
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

			auto physicsComponent = entity["PhysicsComponent"];
			if (physicsComponent)
			{
				deserializedEntity.set<PhysicsComponent>({
					//physicsComponent["Body"].as<b2Body>(),					// @TODO: add support for deserializing b2Body type
					//physicsComponent["PreviousPosition"].as<glm::vec2>()
					});
			}

			auto childEntities = entity["Children"];
			if (childEntities)
			{
				for (const auto& child : childEntities)
				{
					flecs::entity deserializedChildEntity;

					DeserializeEntity(deserializedChildEntity, child);

					deserializedChildEntity.child_of(deserializedEntity);
				}
			}
		}

		bool Load(const std::string& filepath)
		{
			if (!std::filesystem::exists(filepath))
			{
				WC_CORE_ERROR("{} does not exist.", filepath);
				return false;
			}

			YAML::Node data = YAML::LoadFile(filepath);

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
	};
}