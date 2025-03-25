#pragma once

#include <glm/glm.hpp>
#include "Utils/Window.h"
#include "Utils/Time.h"
#include "Sound/SoundEngine.h"

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
		Audio::SoundContext SoundContext;

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