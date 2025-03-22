#pragma once
#include "ScriptBase.h"
#include "../Utils/Window.h"
#include "glm/glm.hpp"

static int lua_Log(lua_State* L, spdlog::level::level_enum level)
{
	int args = lua_gettop(L);

	for (int i = 1; i <= args; i++)
	{
		if (lua_isstring(L, -1))
		{
			WC_LOG(level, lua_tostring(L, -1));
		}
		else if (lua_isboolean(L, -1))
		{
			WC_LOG(level, (bool)lua_toboolean(L, -1));
		}
		else
		{
			WC_WARN("Passed argument of unknown type.");
		}
		lua_pop(L, 1);
	}

	return LUA_OK;
}

static void throwError(lua_State* L, const std::string& errorMessage)
{
	lua_pushstring(L, errorMessage.c_str());
	lua_error(L);
}

static glm::vec4 toVec4(lua_State* L, int index)
{
	glm::vec4 res = { NAN, NAN, NAN, NAN };

	// Vec4 could be x, y, z, w or r, g, b, a so try both
	lua_getfield(L, index, "x");
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		lua_getfield(L, index, "r");
		if (!lua_isnumber(L, -1))
		{
			throwError(L, "Vec4.x expected number. Instead got something else.");
			return res;
		}
	}
	res.x = (float)lua_tonumber(L, -1);

	lua_getfield(L, index, "y");
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		lua_getfield(L, index, "g");
		if (!lua_isnumber(L, -1))
		{
			throwError(L, "Vec4.y expected number. Instead got something else.");
			return res;
		}
	}
	res.y = (float)lua_tonumber(L, -1);

	lua_getfield(L, index, "z");
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		lua_getfield(L, index, "b");
		if (!lua_isnumber(L, -1))
		{
			throwError(L, "Vec4.z expected number. Instead got something else.");
			return res;
		}
	}
	res.z = (float)lua_tonumber(L, -1);

	lua_getfield(L, index, "w");
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		lua_getfield(L, index, "a");
		if (!lua_isnumber(L, -1))
		{
			throwError(L, "Vec.w expected number. Instead got something else.");
			return res;
		}
	}
	res.w = (float)lua_tonumber(L, -1);

	return res;
}

static int lua_IsKeyPressed(lua_State* L)
{
	int Key = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushboolean(L, wc::Key::GetKey(Key));
	return 1;
}

static int lua_Print(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::trace); }
static int lua_Info(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::info); }
static int lua_Warn(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::warn); }
static int lua_Error(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::err); }
static int lua_Critical(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::critical); }

static const struct luaL_Reg logFuncs[]
{
	{"info", lua_Info},
	{"warn", lua_Warn},
	{"error", lua_Error},
	{"critical", lua_Critical},
	{"keypress", lua_IsKeyPressed},
	{NULL, NULL}
};

namespace blaze
{
	struct Script
	{
		std::string Name;
		ScriptState state;

		int Load(const ScriptBinary& script)
		{
			auto res = state.Load(script);
			if (res == LUA_OK)
			{
				Name = script.Name;

				enum FrozenTable
				{
					Red = 1,
					Green,
					Blue,
				};

				state.PushCFunction(lua_Print, "print");
				state.SetGlobal("print");
				state.RegisterEnumType<FrozenTable>("FrozenTable");
				luaL_register(state.L, "log", logFuncs);
			}

			return res;
		}

		void Unload()
		{
			Name.clear();
			state.Close();
		}

		operator bool() { return state.L; }
	};
}