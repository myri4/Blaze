#pragma once

#include "ScriptBase.h"

namespace blaze
{
	struct Script
	{
		std::string Name;
		ScriptState state;

		int Load(const ScriptBinary& script);

		void Unload();

		operator bool() { return state.L; }
	};
}