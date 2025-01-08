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
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

			m_World.each([&](flecs::entity entity, EntityTag) {
				out << YAML::BeginMap; // Entity
				out << YAML::Key << "Name" << YAML::Value << entity.name();
				out << YAML::Key << "ID" << YAML::Value << entity.id();

				/*
				if (entity.has<EntityTag>())
				{
					out << YAML::Key << "TagComponent";
					out << YAML::BeginMap; // TagComponent

					auto tag = entity.get<EntityTag>();
					out << YAML::Key << "Tag" << YAML::Value << tag;

					out << YAML::EndMap; // TagComponent
				}
				*/

				if (entity.has<PositionComponent>())
				{
					out << YAML::Key << "PositionComponent";
					out << YAML::BeginMap; // PositionComponent

					auto pos = entity.get<PositionComponent>()->position;
					out << YAML::Key << "Position" << YAML::Flow;
					out << YAML::BeginSeq << pos.x << pos.y << YAML::EndSeq; // glm::vec2 support

					out << YAML::EndMap; // PositionComponent
				}

				if (entity.has<ScaleComponent>())
				{
					out << YAML::Key << "ScaleComponent";
					out << YAML::BeginMap; // ScaleComponent

					auto sc = entity.get<ScaleComponent>()->scale;
					out << YAML::Key << "Scale" << YAML::Flow;
					out << YAML::BeginSeq << sc.x << sc.y << YAML::EndSeq; // glm::vec2 support

					out << YAML::EndMap; // ScaleComponent
				}

				if (entity.has<RotationComponent>())
				{
					out << YAML::Key << "RotationComponent";
					out << YAML::BeginMap; // RotationComponent

					auto rot = entity.get<RotationComponent>()->rotation;
					out << YAML::Key << "Rotation" << YAML::Value << rot;

					out << YAML::EndMap; // RotationComponent
				}

				if (entity.has<SpriteRendererComponent>())
				{
					out << YAML::Key << "SpriteRendererComponent";
					out << YAML::BeginMap; // SpriteRendererComponent

					auto col = entity.get<SpriteRendererComponent>()->Color;
					out << YAML::Key << "Color" << YAML::Flow;
					out << YAML::BeginSeq << col.r << col.g << col.b << col.a << YAML::EndSeq; // glm::vec4 support

					auto tex = entity.get<SpriteRendererComponent>()->Texture;
					out << YAML::Key << "Texture" << YAML::Value << tex;

					out << YAML::EndMap; // SpriteRendererComponent
				}

				if (entity.has<CircleRendererComponent>())
				{
					out << YAML::Key << "CircleRendererComponent";
					out << YAML::BeginMap; // CircleRendererComponent

					auto col = entity.get<CircleRendererComponent>()->Color;
					out << YAML::Key << "Color" << YAML::Flow;
					out << YAML::BeginSeq << col.r << col.g << col.b << col.a << YAML::EndSeq; // glm::vec4 support

					auto thicc = entity.get<CircleRendererComponent>()->Thickness;
					out << YAML::Key << "Thickness" << YAML::Value << thicc;

					auto fade = entity.get<CircleRendererComponent>()->Fade;
					out << YAML::Key << "Fade" << YAML::Value << fade;

					out << YAML::EndMap; // CircleRendererComponent
				}

				out << YAML::EndMap; // Entity
				});

			out << YAML::EndSeq;
			out << YAML::EndMap;

			YAMLUtils::SaveFile(filepath, out);
		}

		bool Load(const std::string& filepath)
		{
			std::ifstream stream(filepath);
			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());
			if (!data["Scene"]) return false;

			auto entities = data["Entities"];
			if (entities)
			{
				for (auto entity : entities)
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

					auto spriteRendererComponent = entity["RotationComponent"];
					if (spriteRendererComponent)
					{
						deserializedEntity.set<SpriteRendererComponent>({
							spriteRendererComponent["Color"].as<glm::vec4>(),
							spriteRendererComponent["Texture"].as<uint32_t>()
							});
					}

					auto circleRendererComponent = entity["RotationComponent"];
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