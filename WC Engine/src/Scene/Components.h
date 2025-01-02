#pragma once

#include <glm/glm.hpp>

namespace wc
{
	struct PositionComponent
	{
		glm::vec2 position;
	};

	struct VelocityComponent
	{
		glm::vec2 velocity;
	};

	struct ScaleComponent
	{
		glm::vec2 scale;
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = glm::vec4(1.f);
		uint32_t Texture = 0;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color = glm::vec4(1.f);
		float Thickness = 1.0f;
		float Fade = 0.005f;
	};
}