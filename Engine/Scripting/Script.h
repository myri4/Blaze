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

static int lua_IsKeyPressed(lua_State* L)
{
	int Key = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushboolean(L, wc::Key::GetKey(Key));
	return 1;
}

// Get a vec4 from the stack
static glm::vec4 toVec4(lua_State* L, int index)
{
	blaze::ScriptState state(L);
	luaL_checktype(L, index, LUA_TTABLE);

	float x, y, z, w;
	bool success = true;

	// Try x,y,z,w first
	lua_getfield(L, index, "x");
	if (lua_isnumber(L, -1)) 
	{
		x = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else 
	{
		lua_pop(L, 1);
		// Try r,g,b,a
		lua_getfield(L, index, "r");
		if (lua_isnumber(L, -1)) 
		{
			x = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		else 
		{
			success = false;
			lua_pop(L, 1);
		}
	}

	lua_getfield(L, index, "y");
	if (lua_isnumber(L, -1))
	{
		y = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
		// Try r,g,b,a
		lua_getfield(L, index, "g");
		if (lua_isnumber(L, -1))
		{
			y = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		else
		{
			success = false;
			lua_pop(L, 1);
		}
	}

	lua_getfield(L, index, "z");
	if (lua_isnumber(L, -1))
	{
		z = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
		// Try r,g,b,a
		lua_getfield(L, index, "b");
		if (lua_isnumber(L, -1))
		{
			z = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		else
		{
			success = false;
			lua_pop(L, 1);
		}
	}

	lua_getfield(L, index, "w");
	if (lua_isnumber(L, -1))
	{
		w = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
		// Try r,g,b,a
		lua_getfield(L, index, "a");
		if (lua_isnumber(L, -1))
		{
			w = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		else
		{
			success = false;
			lua_pop(L, 1);
		}
	}

	if (!success) 
	{
		state.RaiseError("Invalid vec4 table format");
		return glm::vec4(0.f);
	}

	return glm::vec4(x, y, z, w);
}

static void pushVec4(lua_State* L, const glm::vec4& value)
{
	lua_createtable(L, 0, 4);

	lua_pushnumber(L, value.x);
	lua_setfield(L, -2, "x");

	lua_pushnumber(L, value.y);
	lua_setfield(L, -2, "y");

	lua_pushnumber(L, value.z);
	lua_setfield(L, -2, "z");

	lua_pushnumber(L, value.w);
	lua_setfield(L, -2, "w");

	// Set r,g,b,a aliases too for color access
	lua_pushnumber(L, value.x);
	lua_setfield(L, -2, "r");

	lua_pushnumber(L, value.y);
	lua_setfield(L, -2, "g");

	lua_pushnumber(L, value.z);
	lua_setfield(L, -2, "b");

	lua_pushnumber(L, value.w);
	lua_setfield(L, -2, "a");

	// Set metatable
	luaL_getmetatable(L, "vec4");
	lua_setmetatable(L, -2);
}

static int construct_vec4(lua_State* L)
{
	int nargs = lua_gettop(L);

	if (nargs == 0) 
		pushVec4(L, glm::vec4(0.f));
	else if (nargs == 1 && lua_isnumber(L, 1)) 
	{
		float val = lua_tonumber(L, 1);
		pushVec4(L, glm::vec4(val));
	}
	else 
	{
		float x = luaL_optnumber(L, 1, 0.f);
		float y = luaL_optnumber(L, 2, 0.f);
		float z = luaL_optnumber(L, 3, 0.f);
		float w = luaL_optnumber(L, 4, 0.f);
		pushVec4(L, glm::vec4(x, y, z, w));
	}

	return 1;
}

static int vec4_index(lua_State* L)
{
	const char* key = luaL_checkstring(L, 2);

	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if (!lua_isnil(L, -1)) 
		return 1;

	lua_pop(L, 2);
	lua_pushnil(L);
	return 1;
}

static int vec4_add(lua_State* L)
{
	glm::vec4 a = toVec4(L, 1);
	glm::vec4 b = toVec4(L, 2);

	pushVec4(L, a + b);
	return 1;
}

static int lua_Print(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::trace); }
static int lua_Info(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::info); }
static int lua_Warn(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::warn); }
static int lua_Error(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::err); }
static int lua_Critical(lua_State* L) { return lua_Log(L, spdlog::level::level_enum::critical); }

static const luaL_Reg logFuncs[] =
{
	{"info", lua_Info},
	{"warn", lua_Warn},
	{"error", lua_Error},
	{"critical", lua_Critical},
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

				state.Register("print", lua_Print);
				state.Register("IsKeyPressed", lua_IsKeyPressed);

				luaL_newmetatable(state.L, "vec4");

				// Setup metatable
				lua_pushcfunction(state.L, vec4_index, "__index");
				lua_setfield(state.L, -2, "__index");

				lua_pushcfunction(state.L, vec4_add, "__add");
				lua_setfield(state.L, -2, "__add");
				state.Register("vec4", construct_vec4);
				
				state.Register("log", logFuncs);
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