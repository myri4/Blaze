#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "box2d.h"
#include "../Rendering/Font.h"

namespace wc
{
	struct EntityTag {};

	struct TransformComponent
	{
		glm::vec2 Translation = glm::vec2(0.f);
		glm::vec2 Scale = glm::vec2(1.f);
		float Rotation = 0.f;

		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.f), glm::vec3(Translation, 0.f)) * glm::rotate(glm::mat4(1.f), glm::radians(Rotation), { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.0f), glm::vec3(Scale, 1.f));
		}
	};

	// Graphics

	struct TextRendererComponent
	{
		std::string Text;
		uint32_t FontID = 0;
		glm::vec4 Color = glm::vec4(1.f);
		float Kerning = 0.f;
		float LineSpacing = 0.f;
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = glm::vec4(1.f);
		uint32_t Texture = 0;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color = glm::vec4(1.f);
		float Thickness = 1.f;
		float Fade = 0.005f;
	};

	// Physics
	enum class BodyType { Static = 0, Dynamic, Kinematic };

	struct RigidBodyComponent
	{
		// Don't expose in the ui
		b2Body body;
		glm::vec2 prevPos;
		float previousRotation = 0.f;

		BodyType Type = BodyType::Static;
		bool FixedRotation = false;
		bool Bullet = false;
		bool FastRotation = false;
		float GravityScale = 1.f;
		float LinearDamping = 0.f;
		float AngularDamping = 0.f;

		b2BodyDef GetBodyDef() const
		{
			b2BodyDef bodyDef = b2DefaultBodyDef();
			bodyDef.type = (b2BodyType)Type;
			bodyDef.fixedRotation = FixedRotation;
			bodyDef.isBullet = Bullet;
			bodyDef.allowFastRotation = FastRotation;
			bodyDef.gravityScale = GravityScale;
			bodyDef.linearDamping = LinearDamping;
			bodyDef.angularDamping = AngularDamping;

			return bodyDef;
		}
	};

	struct PhysicsMaterial
	{
		float Density = 1.f;
		float Friction = 0.6f;
		float Restitution = 0.0f;
		float RollingResistance = 0.5f;

		float AllowedClipFraction = 0.1f;
		glm::vec4 DebugColor = glm::vec4(1.f);

		bool EnableSensorEvents = true;
		bool EnableContactEvents = false;
		bool EnableHitEvents = false;
		bool EnablePreSolveEvents = false;
		bool InvokeContactCreation = false;
		bool UpdateBodyMass = true;

		b2ShapeDef GetShapeDef() const
		{
			b2ShapeDef shapeDef = b2DefaultShapeDef();
			shapeDef.density = Density;
			shapeDef.friction = Friction;
			shapeDef.restitution = Restitution;
			shapeDef.rollingResistance = RollingResistance;
			shapeDef.allowedClipFraction = AllowedClipFraction;
			shapeDef.customColor = glm::packUnorm4x8(DebugColor); // @NOTE: maybe not working
			shapeDef.enableSensorEvents = EnableSensorEvents;
			shapeDef.enableContactEvents = EnableContactEvents;
			shapeDef.enableHitEvents = EnableHitEvents;
			shapeDef.enablePreSolveEvents = EnablePreSolveEvents;
			shapeDef.invokeContactCreation = InvokeContactCreation;
			shapeDef.updateBodyMass = UpdateBodyMass;

			return shapeDef;
		}
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.f, 0.f };
		glm::vec2 Size = { 0.5f, 0.5f };

		PhysicsMaterial Material;

		b2Shape Shape;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.f, 0.f };
		float Radius = 0.5f; // @TODO: if this is set to -1 derive the radius from other components

		PhysicsMaterial Material;

		b2Shape Shape;
	};
}