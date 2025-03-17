#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "box2d.h"

#include "../Rendering/Font.h"

#include "../Scripting/Script.h"

namespace blaze
{
	struct EntityTag
	{
	    bool showEntity = true;
	};

	struct EntityOrderComponent
	{
		std::vector<std::string> EntityOrder;
	};

	struct TransformComponent
	{
		glm::vec3 Translation = glm::vec3(0.f);
		glm::vec3 Scale = glm::vec3(1.f);
		glm::vec3 Rotation = glm::vec3(0.f);

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	// Graphics
	enum class CameraType { Orthographic, Perspective };
	struct CameraComponent
	{
		bool FixedAspectRatio = false;
		CameraType type = CameraType::Orthographic;
		// uint32_t target
		// Effects UI...
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

	struct TextRendererComponent
	{
		std::string Text;
		uint32_t FontID = UINT32_MAX;
		glm::vec4 Color = glm::vec4(1.f);
		float Kerning = 0.f;
		float LineSpacing = 0.f;
	};

	// Physics
	enum class BodyType { Static = 0, Dynamic, Kinematic };

	b2BodyType BodyTypeToBox2D(BodyType type)
	{
		if (type == BodyType::Static) return b2_staticBody;
		if (type == BodyType::Dynamic) return b2_dynamicBody;
		if (type == BodyType::Kinematic) return b2_kinematicBody;
	}

	struct RigidBodyComponent
	{
		// Don't expose in the ui
		b2::Body body;
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
			bodyDef.type = BodyTypeToBox2D(Type);
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

		glm::vec4 DebugColor = glm::vec4(1.f);

		bool Sensor = false;
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
			shapeDef.customColor = glm::packUnorm4x8(DebugColor); // @NOTE: maybe not working
			shapeDef.isSensor = Sensor;
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
		glm::vec2 Offset = glm::vec2(0.f);
		glm::vec2 Size = glm::vec2(1.f);

		uint32_t MaterialID = 0; // @NOTE: This may need to be PhysMatID or something like that

		b2::Shape Shape;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = glm::vec2(0.f);
		float Radius = 0.5f; // @TODO: if this is set to -1 derive the radius from other components

		uint32_t MaterialID = 0;

		b2::Shape Shape;
	};

	// Scripting

	struct ScriptComponent
	{
		Script ScriptInstance;
	};
}