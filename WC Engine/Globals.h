#pragma once

#include <glm/glm.hpp>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>

namespace wc
{
	struct GlobalVariables
	{
		Window window; // @TODO: maybe rename to main window

		// Change size if needed
		ImFont* fontDefault = nullptr;
		ImFont* fontBig = nullptr;
		//ImFont* fontSmall = nullptr;
		//ImFont* fontBold = nullptr;

		Clock deltaTimer;
		float deltaTime = 0.f;

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}
	};

	inline GlobalVariables Globals;
}