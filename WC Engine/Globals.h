#pragma once

#include <glm/glm.hpp>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>

namespace wc
{
    struct AppFont
    {
        ImFont* Regular = nullptr;
        ImFont* Bold = nullptr;
        ImFont* Italic = nullptr;
        ImFont* Thin = nullptr;
        ImFont* Big = nullptr;
        ImFont* Small = nullptr;
        ImFont* Menu = nullptr;
        // ADD OTHER IF NEEDED
    };

	struct GlobalVariables
	{
		Window window; // @TODO: maybe rename to main window

		// Change size if needed
        AppFont f_Display = {};
	    AppFont f_Default = {};

		Clock deltaTimer;
		float deltaTime = 0.f;

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}
	};

	inline GlobalVariables Globals;
}