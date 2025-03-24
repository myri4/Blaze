#pragma once
#include "ScriptBase.h"
#include "../Utils/Window.h"
#include "glm/glm.hpp"

static int lua_Log(lua_State* L, spdlog::level::level_enum level)
{
	int args = lua_gettop(L);

	for (int i = 1; i <= args; i++)
	{
		if (lua_isboolean(L, -1))
		{
			WC_LOG(level, (bool)lua_toboolean(L, -1));
		}
		else if (lua_isstring(L, -1))
		{
			WC_LOG(level, lua_tostring(L, -1));
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
	int Key = lua_tonumber(L, 1);
	lua_pushboolean(L, wc::Key::GetKey(Key));
	return 1;
}

static bool isVec2(lua_State* L, int index)
{
	bool res = true;

	// TODO: Code

	return res;
}

static bool isVec3(lua_State* L, int index)
{
	bool res = true;

	// TODO: Code

	return res;
}

static bool isVec4(lua_State* L, int index)
{
	blaze::ScriptState state(L);

	if (!state.IsTable(index)) return false;

	if (!state.GetMetatable(index)) return false;

	// Get the name of the metatable
	state.GetField(index, "__name");
	if (state.IsString())
	{
		auto name = state.ToString();
		if (name != "vec4") return false;
	}
	else 
	{
		WC_ERROR("__name is not a string!")
	}
	// Check for required numeric fields: x, y, z, w
	/*const char* fields[] = {"x", "y", "z", "w"};
	for (const char* field : fields) {
		state.GetField(index, field);
		if (!state.IsNumber(-1)) {
			return false;
		}
	}*/

	return true;
}

// Get a vec4 from the stack
static void pushVec4(lua_State* L, const glm::vec4& value)
{
	blaze::ScriptState state(L);
	state.CreateTable(0, 4);

	state.RegisterField("x", value.x);
	state.RegisterField("y", value.y);
	state.RegisterField("z", value.z);
	state.RegisterField("w", value.w);

	// Set r,g,b,a aliases too for color access
	state.RegisterField("r", value.r);
	state.RegisterField("g", value.g);
	state.RegisterField("b", value.b);
	state.RegisterField("a", value.a);

	// Set metatable
	state.GetMetatable("vec4");
	state.SetMetatable(-2);
}

static glm::vec4 toVec4(lua_State* L, int index)
{
	blaze::ScriptState state(L);
	luaL_checktype(L, index, LUA_TTABLE);

	float x, y, z, w;
	bool success = true;

	// Try x,y,z,w first
	state.GetField(index, "x");
	if (state.IsNumber()) 
	{
		x = (float)state.ToNumber();
		state.Pop();
	}
	else 
	{
		state.Pop();
		// Try r,g,b,a
		state.GetField(index, "r");
		if (state.IsNumber()) x = (float)state.ToNumber();
		else success = false;

		state.Pop();
	}

	state.GetField(index, "y");
	if (state.IsNumber())
	{
		y = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b,a
		state.GetField(index, "g");
		if (state.IsNumber()) y = (float)state.ToNumber();
		else success = false;

		state.Pop();
	}

	state.GetField(index, "z");
	if (state.IsNumber())
	{
		z = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b,a
		state.GetField(index, "b");
		if (state.IsNumber()) z = (float)state.ToNumber();
		else success = false;

		state.Pop();
	}

	state.GetField(index, "w");
	if (state.IsNumber())
	{
		w = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b,a
		state.GetField(index, "a");
		if (state.IsNumber()) w = (float)state.ToNumber();
		else success = false;

		state.Pop();
	}

	if (!success) 
	{
		state.RaiseError("Invalid vec4 table format");
		return glm::vec4(0.f);
	}

	return glm::vec4(x, y, z, w);
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
	glm::vec4 b = lua_isnumber(L, 2) ? glm::vec4(lua_tonumber(L, 2)) : toVec4(L, 2);

	pushVec4(L, a + b);
	return 1;
}

static int vec4_sub(lua_State* L)
{
	glm::vec4 a = toVec4(L, 1);
	glm::vec4 b = lua_isnumber(L, 2) ? glm::vec4(lua_tonumber(L, 2)) : toVec4(L, 2);

	pushVec4(L, a - b);
	return 1;
}

static int vec4_mul(lua_State* L)
{
	glm::vec4 a = toVec4(L, 1);
	glm::vec4 b = lua_isnumber(L, 2) ? glm::vec4(lua_tonumber(L, 2)) : toVec4(L, 2);

	pushVec4(L, a * b);
	return 1;
}

static int vec4_div(lua_State* L)
{
	glm::vec4 a = toVec4(L, 1);
	glm::vec4 b = lua_isnumber(L, 2) ? glm::vec4(lua_tonumber(L, 2)) : toVec4(L, 2);

	pushVec4(L, a / b);
	return 1;
}

static int vec4_eq(lua_State* L)
{
	glm::vec4 a = toVec4(L, 1);
	glm::vec4 b = toVec4(L, 2);

	lua_pushboolean(L, a == b);
	return 1;
}

static int vec4_unm(lua_State* L)
{
	glm::vec4 a = -toVec4(L, 1);

	pushVec4(L, a);
	return 1;
}

// Get a vec3 from the stack
static void pushVec3(lua_State* L, const glm::vec3& value)
{
	blaze::ScriptState state(L);
	state.CreateTable(0, 3);

	state.RegisterField("x", value.x);
	state.RegisterField("y", value.y);
	state.RegisterField("z", value.z);

	// Set r,g,b aliases too for color access
	state.RegisterField("r", value.r);
	state.RegisterField("g", value.g);
	state.RegisterField("b", value.b);

	// Set metatable
	state.GetMetatable("vec3");
	state.SetMetatable(-2);
}

static glm::vec3 toVec3(lua_State* L, int index)
{
	blaze::ScriptState state(L);
	luaL_checktype(L, index, LUA_TTABLE);

	float x, y, z;
	bool success = true;

	// Try x,y,z first
	state.GetField(index, "x");
	if (state.IsNumber())
	{
		x = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b
		state.GetField(index, "r");
		if (state.IsNumber())
		{
			x = (float)state.ToNumber();
			state.Pop();
		}
		else
		{
			success = false;
			state.Pop();
		}
	}

	state.GetField(index, "y");
	if (state.IsNumber())
	{
		y = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b
		state.GetField(index, "g");
		if (state.IsNumber())
		{
			y = (float)state.ToNumber();
			state.Pop();
		}
		else
		{
			success = false;
			state.Pop();
		}
	}

	state.GetField(index, "z");
	if (state.IsNumber())
	{
		z = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g,b
		state.GetField(index, "b");
		if (state.IsNumber())
		{
			z = (float)state.ToNumber();
			state.Pop();
		}
		else
		{
			success = false;
			state.Pop();
		}
	}

	if (!success)
	{
		state.RaiseError("Invalid vec3 table format");
		return glm::vec3(0.f);
	}

	return glm::vec3(x, y, z);
}

static int construct_vec3(lua_State* L)
{
	int nargs = lua_gettop(L);

	if (nargs == 0)
		pushVec3(L, glm::vec3(0.f));
	else if (nargs == 1 && lua_isnumber(L, 1))
	{
		float val = lua_tonumber(L, 1);
		pushVec3(L, glm::vec3(val));
	}
	else
	{
		float x = luaL_optnumber(L, 1, 0.f);
		float y = luaL_optnumber(L, 2, 0.f);
		float z = luaL_optnumber(L, 3, 0.f);
		pushVec3(L, glm::vec3(x, y, z));
	}

	return 1;
}

static int vec3_index(lua_State* L)
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

static int vec3_add(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = lua_isnumber(L, 2) ? glm::vec3(lua_tonumber(L, 2)) : toVec3(L, 2);

	pushVec3(L, a + b);
	return 1;
}

static int vec3_sub(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = lua_isnumber(L, 2) ? glm::vec3(lua_tonumber(L, 2)) : toVec3(L, 2);

	pushVec3(L, a - b);
	return 1;
}

static int vec3_mul(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = lua_isnumber(L, 2) ? glm::vec3(lua_tonumber(L, 2)) : toVec3(L, 2);

	pushVec3(L, a * b);
	return 1;
}

static int vec3_div(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = lua_isnumber(L, 2) ? glm::vec3(lua_tonumber(L, 2)) : toVec3(L, 2);

	pushVec3(L, a / b);
	return 1;
}

static int vec3_eq(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = toVec3(L, 2);

	lua_pushboolean(L, a == b);
	return 1;
}

static int vec3_unm(lua_State* L)
{
	glm::vec3 a = -toVec3(L, 1);

	pushVec3(L, a);
	return 1;
}

// Get a vec2 from the stack
static void pushVec2(lua_State* L, const glm::vec2& value)
{
	blaze::ScriptState state(L);
	state.CreateTable(0, 2);

	state.RegisterField("x", value.x);
	state.RegisterField("y", value.y);

	// Set r,g aliases too for color access
	state.RegisterField("r", value.r);
	state.RegisterField("g", value.g);

	// Set metatable
	state.GetMetatable("vec2");
	state.SetMetatable(-2);
}

static glm::vec2 toVec2(lua_State* L, int index)
{
	blaze::ScriptState state(L);
	luaL_checktype(L, index, LUA_TTABLE);

	float x, y;
	bool success = true;

	// Try x,y first
	state.GetField(index, "x");
	if (state.IsNumber())
	{
		x = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g
		state.GetField(index, "r");
		if (state.IsNumber())
		{
			x = (float)state.ToNumber();
			state.Pop();
		}
		else
		{
			success = false;
			state.Pop();
		}
	}

	state.GetField(index, "y");
	if (state.IsNumber())
	{
		y = (float)state.ToNumber();
		state.Pop();
	}
	else
	{
		state.Pop();
		// Try r,g
		state.GetField(index, "g");
		if (state.IsNumber())
		{
			y = (float)state.ToNumber();
			state.Pop();
		}
		else
		{
			success = false;
			state.Pop();
		}
	}

	if (!success)
	{
		state.RaiseError("Invalid vec2 table format");
		return glm::vec2(0.f);
	}

	return glm::vec2(x, y);
}

static int construct_vec2(lua_State* L)
{
	int nargs = lua_gettop(L);

	if (nargs == 0)
		pushVec2(L, glm::vec2(0.f));
	else if (nargs == 1 && lua_isnumber(L, 1))
	{
		float val = lua_tonumber(L, 1);
		pushVec2(L, glm::vec2(val));
	}
	else
	{
		float x = luaL_optnumber(L, 1, 0.f);
		float y = luaL_optnumber(L, 2, 0.f);
		pushVec2(L, glm::vec2(x, y));
	}

	return 1;
}

static int vec2_index(lua_State* L)
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

static int vec2_add(lua_State* L)
{
	glm::vec2 a = toVec2(L, 1);
	glm::vec2 b = lua_isnumber(L, 2) ? glm::vec2(lua_tonumber(L, 2)) : toVec2(L, 2);

	pushVec2(L, a + b);
	return 1;
}

static int vec2_sub(lua_State* L)
{
	glm::vec2 a = toVec2(L, 1);
	glm::vec2 b = lua_isnumber(L, 2) ? glm::vec2(lua_tonumber(L, 2)) : toVec2(L, 2);

	pushVec2(L, a - b);
	return 1;
}

static int vec2_mul(lua_State* L)
{
	glm::vec2 a = toVec2(L, 1);
	glm::vec2 b = lua_isnumber(L, 2) ? glm::vec2(lua_tonumber(L, 2)) : toVec2(L, 2);

	pushVec2(L, a * b);
	return 1;
}

static int vec2_div(lua_State* L)
{
	glm::vec2 a = toVec2(L, 1);
	glm::vec2 b = lua_isnumber(L, 2) ? glm::vec2(lua_tonumber(L, 2)) : toVec2(L, 2);

	pushVec2(L, a / b);
	return 1;
}

static int vec2_eq(lua_State* L)
{
	glm::vec2 a = toVec2(L, 1);
	glm::vec2 b = toVec2(L, 2);

	lua_pushboolean(L, a == b);
	return 1;
}

static int vec2_unm(lua_State* L)
{
	glm::vec2 a = -toVec2(L, 1);

	pushVec2(L, a);
	return 1;
}

static int lua_normalize(lua_State* L)
{
	WC_INFO("{}", isVec4(L, 1))
	/*if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);

		pushVec2(L, glm::normalize(a));
	}

	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);

		pushVec3(L, glm::normalize(a));
	}

	else
	{
		glm::vec4 a = toVec4(L, 1);

		pushVec4(L, glm::normalize(a));
	}*/
	
	return 1;
}

static int min(lua_State* L)
{
	if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);
		glm::vec2 b = toVec2(L, 2);

		pushVec2(L, glm::min(a, b));
	}
	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);
		glm::vec3 b = toVec3(L, 2);

		pushVec3(L, glm::min(a, b));
	}
	else
	{
		glm::vec4 a = toVec4(L, 1);
		glm::vec4 b = toVec4(L, 2);

		pushVec4(L, glm::min(a, b));
	}
	
	return 1;
}

static int max(lua_State* L)
{
	if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);
		glm::vec2 b = toVec2(L, 2);

		pushVec2(L, glm::max(a, b));
	}
	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);
		glm::vec3 b = toVec3(L, 2);

		pushVec3(L, glm::max(a, b));
	}
	else
	{
		glm::vec4 a = toVec4(L, 1);
		glm::vec4 b = toVec4(L, 2);

		pushVec4(L, glm::max(a, b));
	}
	
	return 1;
}

static int length(lua_State* L)
{
	if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);

		lua_pushnumber(L, glm::length(a));
	}
	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);

		lua_pushnumber(L, glm::length(a));
	}
	else
	{
		glm::vec4 a = toVec4(L, 1);

		lua_pushnumber(L, glm::length(a));
	}
	
	return 1;
}

static int distance(lua_State* L)
{
	if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);
		glm::vec2 b = toVec2(L, 2);

		lua_pushnumber(L, glm::distance(a, b));
	}
	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);
		glm::vec3 b = toVec3(L, 2);

		lua_pushnumber(L, glm::distance(a, b));
	}
	else
	{
		glm::vec4 a = toVec4(L, 1);
		glm::vec4 b = toVec4(L, 2);

		lua_pushnumber(L, glm::distance(a, b));
	}
	
	return 1;
}

static int cross(lua_State* L)
{
	glm::vec3 a = toVec3(L, 1);
	glm::vec3 b = toVec3(L, 2);

	pushVec3(L, glm::cross(a, b));
	return 1;
}

static int dot(lua_State* L)
{
	if (isVec2(L, 1) && isVec2(L, 2))
	{
		glm::vec2 a = toVec2(L, 1);
		glm::vec2 b = toVec2(L, 2);

		lua_pushnumber(L, glm::dot(a, b));
	}
	else if (isVec3(L, 1) && isVec3(L, 2))
	{
		glm::vec3 a = toVec3(L, 1);
		glm::vec3 b = toVec3(L, 2);

		lua_pushnumber(L, glm::dot(a, b));
	}
	else
	{
		glm::vec4 a = toVec4(L, 1);
		glm::vec4 b = toVec4(L, 2);

		lua_pushnumber(L, glm::dot(a, b));
	}

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

				enum FrozenTable
				{
					Red,
					Green,
					Blue,
				};

				state.RegisterEnumType<FrozenTable>("FrozenTable");

				state.Register("print", lua_Print);
				state.Register("IsKeyPressed", lua_IsKeyPressed);

				state.Register("normalize", lua_normalize);
				state.Register("min", min);
				state.Register("max", max);
				state.Register("length", length);
				state.Register("distance", distance);
				state.Register("cross", cross);
				state.Register("dot", dot);

				// Setup metatable
				state.NewMetatable("vec4");
				state.RegisterField("__name", "vec4");
				state.RegisterField("__index", vec4_index);
				state.RegisterField("__add", vec4_add);
				state.RegisterField("__sub", vec4_sub);
				state.RegisterField("__mul", vec4_mul);
				state.RegisterField("__div", vec4_div);
				state.RegisterField("__eq", vec4_eq);
				state.RegisterField("__unm", vec4_unm);
				state.Pop();
				
				state.Register("vec4", construct_vec4);
				
				// Setup metatable
				state.NewMetatable("vec3");
				state.RegisterField("__name", "vec3");
				state.RegisterField("__index", vec3_index);
				state.RegisterField("__add", vec3_add);
				state.RegisterField("__sub", vec3_sub);
				state.RegisterField("__mul", vec3_mul);
				state.RegisterField("__div", vec3_div);
				state.RegisterField("__eq", vec3_eq);
				state.RegisterField("__unm", vec3_unm);
				state.Pop();
				
				state.Register("vec3", construct_vec3);
				
				// Setup metatable
				state.NewMetatable("vec2");
				state.RegisterField("__name", "vec2");
				state.RegisterField("__index", vec2_index);
				state.RegisterField("__add", vec2_add);
				state.RegisterField("__sub", vec2_sub);
				state.RegisterField("__mul", vec2_mul);
				state.RegisterField("__div", vec2_div);
				state.RegisterField("__eq", vec2_eq);
				state.RegisterField("__unm", vec2_unm);
				state.Pop();
				
				state.Register("vec2", construct_vec2);
				
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