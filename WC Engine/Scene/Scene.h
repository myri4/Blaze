#pragma once

#include "flecs/flecs.h"
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
			worldDef.gravity = b2Vec2{ 0.0f, -10.0f };
			m_PhysicsWorld.Create(worldDef);

			m_World.each([this](PhysicsComponent& p, PositionComponent& pos) {
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

			m_World.each([this, &alpha](PhysicsComponent& p, PositionComponent& pc)
				{
					pc.position = glm::mix(p.prevPos, p.body.GetPosition(), alpha);
				});
		}

		void Update()
		{
			UpdatePhysics();
		}

		void Save(const std::string& filepath)
		{
			YAML::Node metaData;
			YAML::Node entitiesData;

			m_World.each([&](flecs::entity entity, EntityTag) {
				YAML::Node entityData;
				entityData["Name"] = std::string(entity.name().c_str());
				entityData["ID"] = entity.id();

				if (entity.has<PositionComponent>())
				{
					YAML::Node componentData;
					componentData["Position"] = entity.get<PositionComponent>()->position;
					entityData["PositionComponent"] = componentData;
				}

				if (entity.has<ScaleComponent>())
				{
					YAML::Node componentData;
					componentData["Scale"] = entity.get<ScaleComponent>()->scale;
					entityData["ScaleComponent"] = componentData;
				}

				if (entity.has<RotationComponent>())
				{
					YAML::Node componentData;
					componentData["Rotation"] = entity.get<RotationComponent>()->rotation;
					entityData["RotationComponent"] = componentData;
				}

				if (entity.has<SpriteRendererComponent>())
				{
					YAML::Node componentData;
					componentData["Color"] = entity.get<SpriteRendererComponent>()->Color;
					componentData["Texture"] = entity.get<SpriteRendererComponent>()->Texture; // @TODO: this should be in text form
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

				entitiesData.push_back(entityData);
			});

			metaData["Entities"] = entitiesData;

			YAMLUtils::SaveFile(filepath, metaData);
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
					std::string name = entity["Name"].as<std::string>();
					std::string id = entity["ID"].as<std::string>();

					WC_CORE_TRACE("Deserialized entity with name = {}, ID = {}", name, id);

					flecs::entity deserializedEntity = AddEntity(name);

					auto positionComponent = entity["PositionComponent"];
					if (positionComponent)
					{
						deserializedEntity.set<PositionComponent>({
							positionComponent["Position"].as<glm::vec2>()
							});
					}

					auto scaleComponent = entity["ScaleComponent"];
					if (scaleComponent)
					{
						deserializedEntity.set<ScaleComponent>({
							scaleComponent["Scale"].as<glm::vec2>()
							});
					}

					auto rotationComponent = entity["RotationComponent"];
					if (rotationComponent)
					{
						deserializedEntity.set<RotationComponent>({
							rotationComponent["Rotation"].as<float>()
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
				}
			}
		}
	};
}