#pragma once

#include <glm/glm.hpp>
#include "box2d.h"
#include "../Rendering/Font.h"

namespace wc
{
	struct EntityTag {};

	// Transforms

	struct TransformComponent
	{
		glm::vec2 position = { 0, 0 };
		glm::vec2 scale = { 1, 1 };
		float rotation = { 0 };
	};

	// Graphics

	struct TextRendererComponent
	{
		std::string Text;
		Font Font;
		glm::vec4 Color = glm::vec4(1.f);
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

	struct PhysicsComponent
	{
		b2Body body;
		glm::vec2 prevPos; // Don't expose in the ui
	};
}