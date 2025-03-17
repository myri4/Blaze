#pragma once
#include "ScriptBase.h"

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

	return 0;
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