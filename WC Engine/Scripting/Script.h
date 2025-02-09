#pragma once

#include <luau/Common/include/Luau/Common.h>
#include <luau/Compiler/include/luacode.h>
#include <luau/VM/include/lualib.h>

#include <wc/Utils/Log.h>
#include <filesystem>
#include <string>

static int printWrapper(lua_State* L)
{
	int args = lua_gettop(L);

	for (int i = 1; i <= args; i++)
	{
		if (lua_isstring(L, -1))
		{
			WC_INFO(lua_tostring(L, -1))
		}
		else if (lua_isboolean(L, -1))
		{
			WC_INFO((bool)lua_toboolean(L, -1));
		}
		else
		{
			WC_WARN("Passed argument of unknown type.");
		}
	}

	return 0;
}

static const struct luaL_Reg blazeLib[]
{
	{"logWarn", printWrapper},
	{NULL, NULL}
};

namespace blaze
{
	inline std::string OpenFile(const std::string& filePath)
	{
		std::stringstream fileContents;
		{
			std::fstream fileHandle(filePath, std::ios::in);
			if (!fileHandle) WC_CORE_ERROR("Cant find {}", filePath);
			fileContents << fileHandle.rdbuf();
		}
		return fileContents.str();
	}

	struct Script
	{
	private:
		lua_State* L = nullptr;

		void Pop(int n = 1) { lua_pop(L, n); }
		void NewTable() { lua_newtable(L); }
		//void NewUserData(const std::string& s) { lua_newtable(L, s.c_str()); }

		auto IsString(int n = -1) { return lua_isstring(L, n); }
		auto IsNumber(int n = -1) { return lua_isnumber(L, n); }
		auto IsFunction(int n = -1) { return lua_isfunction(L, n); }
		auto IsTable(int n = -1) { return lua_istable(L, n); }
		auto IsLightUserData(int n = -1) { return lua_islightuserdata(L, n); }
		auto IsNil(int n = -1) { return lua_isnil(L, n); }
		auto IsBoolean(int n = -1) { return lua_isboolean(L, n); }
		auto IsVector(int n = -1) { return lua_isvector(L, n); }
		auto IsThread(int n = -1) { return lua_isthread(L, n); }
		auto IsBuffer(int n = -1) { return lua_isbuffer(L, n); }
		auto IsNone(int n = -1) { return lua_isnone(L, n); }
		auto IsNoneOrNil(int n = -1) { return lua_isnoneornil(L, n); }

		auto GetTop() { return lua_gettop(L); }

		//void PushLiteral(const std::string& s) { lua_pushliteral(L, s.c_str()); }
		//void PushCFunction(const std::string& s) { lua_pushliteral(L, s.c_str()); }
		//void PushCClosure(const std::string& s) { lua_pushliteral(L, s.c_str()); }
		void PushLightUserData(void* p) { lua_pushlightuserdata(L, p); }
		void PushBool(int b) { lua_pushboolean(L, b); }
		void PushInt(int n) { lua_pushinteger(L, n); }
		void PushString(const std::string& s) { lua_pushlstring(L, s.data(), s.size()); }
		void PushNil() { lua_pushnil(L); }
		void PushNumber(lua_Number n) { lua_pushnumber(L, n); }
		void PushValue(int n) { lua_pushvalue(L, n); }

		auto Next(int idx) { return lua_next(L, idx); }
		std::string ToTypeName(int i = -1) { return std::string(lua_typename(L, lua_type(L, i))); }

		std::string ToString(int i = -1) { return std::string(lua_tostring(L, i)); }
		int32_t ToInt(int i = -1) { return lua_tointeger(L, i); }
		uint32_t ToUInt(int i = -1) { return lua_tounsigned(L, i); }
		double ToNumber(int i = -1) { return lua_tonumber(L, i); }

		void SetGlobal(const std::string& s) { lua_setglobal(L, s.c_str()); }
		auto GetGlobal(const std::string& s) { return lua_getglobal(L, s.c_str()); }

		void Register(const std::string& libName, const luaL_Reg* l) { luaL_register(L, libName.c_str(), l); }

		auto PCall(int nargs, int nresults, int errfunc) { return lua_pcall(L, nargs, nresults, errfunc); }
		auto Call(int nargs, int nresults) { return lua_call(L, nargs, nresults); }

		template<typename T>
		void PushValue(T value)
		{
			if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>)
				PushInt(value);
			else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)
				PushNumber(value);
			else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, std::string>)
				PushString(value);
			else if constexpr (std::is_same_v<T, bool>)
				PushBool(value);
			else
				static_assert("Unsupported type for Lua");
		}

		void Convert(int idx)
		{

		}

	public:

		int Load(const char* byteCode, size_t byteCodeSize, const std::string& name)
		{
			L = luaL_newstate();
			luaL_openlibs(L);
			Register("blaze", blazeLib);

			if (luau_load(L, name.c_str(), byteCode, byteCodeSize, 0) != LUA_OK)
				return -1;

			PCall(0, LUA_MULTRET, 0); // This is really strange and has to be here.

			GetGlobal("_G");
			lua_pushnil(L);  /* first key */
			while (lua_next(L, -2) != 0)
			{
				/* uses 'key' (at index -2) and 'value' (at index -1) */
				std::string value;
				if (IsString())
				{
					value = ToString();
				}
				else if (IsNumber())
					value = std::format("{}", ToNumber());

				WC_CORE_INFO("{}: {}", value, ToString(-2));
				/* removes 'value'; keeps 'key' for next iteration */
				lua_pop(L, 1);
			}

			return LUA_OK;
		}

		void LoadFromFile(const std::string& filePath)
		{
			if (!std::filesystem::exists(filePath))
			{
				WC_CORE_ERROR("Could not find {}", filePath);
				return;
			}

			// needs lua.h and luacode.h
			size_t bytecodeSize = 0;
			std::string srcCode = OpenFile(filePath);
			char* bytecode = luau_compile(srcCode.c_str(), srcCode.size(), NULL, &bytecodeSize);

			std::string_view sv = std::string_view();

			if (Load(bytecode, bytecodeSize, filePath) != LUA_OK)
				WC_ERROR(std::string(bytecode, bytecodeSize));

			free(bytecode);
		}

		template<typename T>
		void SetVariable(const std::string& name, const T& value)
		{
			PushValue(value);
			SetGlobal(name);
		}

		template<typename... Args>
		void Execute(const std::string& functionName, Args&&... args)
		{
			GetGlobal(functionName);

			if (!IsFunction())
			{
				WC_ERROR("{} is not a function", functionName);
				return;
			}

			(PushValue(args), ...);

			constexpr int argCount = sizeof...(Args);
			WC_CORE_INFO(GetTop());
			if (PCall(argCount, 0, 0) != LUA_OK)
			{
				// Print error message if the call fails
				const char* error = lua_tostring(L, -1);
				WC_ERROR("{}", error);
				Pop(); // Remove error message from the stack
			}
		}
	};
}