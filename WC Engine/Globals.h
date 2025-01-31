#pragma once

#include <glm/glm.hpp>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>

namespace wc 
{
	struct GlobalVariables
	{
		Window window; // @TODO: maybe rename to main window

		Clock deltaTimer;
		float deltaTime = 0.f;

		void HandleWindowState()
		{
			window.SetMaximized(!isWindowMaximized);
			isWindowMaximized = !isWindowMaximized;
		}

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}

	private:
		bool isWindowMaximized = window.IsMaximized();
	};

	inline GlobalVariables Globals;
}