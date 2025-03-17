#pragma once

#include <Luau/Common.h>
#include <luacode.h>
#include <lualib.h>

#include <Luau/Parser.h>
#include <Luau/Scope.h>
#include <Luau/TypeInfer.h>
#include <Luau/TypeChecker2.h>
#include <Luau/Frontend.h>
#include <Luau/BuiltinDefinitions.h>
#include <Luau/Module.h>


#include <wc/Utils/Log.h>
#include <filesystem>
#include <string>

#include <magic_enum.hpp>

namespace blaze
{
	inline std::string OpenFile(const std::string& filePath)
	{
		std::stringstream fileContents;
		{
			std::fstream fileHandle(filePath, std::ios::in);
			if (!fileHandle)
			{
				WC_CORE_ERROR("Can't find {}", filePath);
				return "";
			}
			fileContents << fileHandle.rdbuf();
		}
		return fileContents.str();
	}

	struct ScriptBinary
	{
		std::vector<uint8_t> binary;
		std::vector<std::string> VariableNames;
		std::string Name;

		bool CompileScript(const std::string& filePath)
		{
			if (!std::filesystem::exists(filePath))
			{
				WC_CORE_ERROR("Could not find {}", filePath);
				return false;
			}

			Name = filePath;

			std::string srcCode = OpenFile(filePath);
			bool hasErrors = false;
			{ // analyze script
				Luau::FrontendOptions options;

				struct FileResolver : public Luau::FileResolver
				{
					FileResolver(const std::string& s) { src = s; }

					std::string src;

					std::optional<Luau::SourceCode> readSource(const Luau::ModuleName& name) override
					{
						Luau::SourceCode res;
						res.type = res.Module;
						res.source = src;
						return res;
					}
				} fileResolver(srcCode);

				struct ConfigResolver : public Luau::ConfigResolver
				{
					Luau::Config res = {};

					const Luau::Config& getConfig(const Luau::ModuleName& name) const override
					{
						return res;
					}
				} configResolver;


				Luau::Frontend frontend(&fileResolver, &configResolver, options);
				Luau::registerBuiltinGlobals(frontend, frontend.globals);
				{
					Luau::LoadDefinitionFileResult loadResult = frontend.loadDefinitionFile(frontend.globals, frontend.globals.globalScope, OpenFile("BuiltinDefinitions.luau"), "BuiltinDefinitions", false, false);
					if (!loadResult.success)
					{
						for (const auto& error : loadResult.parseResult.errors)
						{
							WC_ERROR("Builtin definitions: [{},{}]: {}", error.getLocation().begin.line + 1, 
								error.getLocation().begin.column + 1, error.getMessage());
						}
					}
				}
				Luau::freeze(frontend.globals.globalTypes);

				const char* name = filePath.c_str();
				Luau::CheckResult cr;

				if (frontend.isDirty(name))
					cr = frontend.check(name);

				if (!frontend.getSourceModule(name))
				{
					WC_CORE_ERROR("{} does not exist", name);
					return false;
				}

				for (auto& error : cr.errors)
				{
					std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(error.moduleName);
					const auto* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data);
					hasErrors = true;
					if (syntaxError)
					{
						WC_ERROR("{}[{},{}]: Syntax error: {}", humanReadableName, (error.location.begin.line + 1), (error.location.begin.column + 1), syntaxError->message);
					}
					else
					{
						WC_ERROR("{}[{},{}]: Type error: {}", humanReadableName, error.location.begin.line + 1, error.location.begin.column + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{ frontend.fileResolver }));
					}
				}
				for (auto& warning : cr.lintResult.warnings)
				{
					std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(filePath);
					WC_WARN("{}({},{}): {}: {}", humanReadableName, warning.location.begin.line + 1, warning.location.begin.column + 1, Luau::LintWarning::getName(warning.code), warning.text);
				}

				if (!hasErrors)
				{
					Luau::Allocator allocator;
					Luau::AstNameTable names(allocator);
					Luau::ParseOptions options;

					Luau::ParseResult parseResult = Luau::Parser::parse(srcCode.data(), srcCode.size(), names, allocator, options);

					// Visit all statements in the block
					for (auto* stat : parseResult.root->body)
					{
						/*if (auto local = stat->as<Luau::AstStatLocal>())
						{
							for (auto* var : local->vars)
								VariableNames.push_back(var->name.value);
						} else*/
						if (auto assign = stat->as<Luau::AstStatAssign>())
						{
							for (auto* expr : assign->vars)
								if (auto global = expr->as<Luau::AstExprGlobal>())
									VariableNames.push_back(global->name.value);
						}
					}

					size_t bytecodeSize = 0;
					char* bytecode = luau_compile(srcCode.c_str(), srcCode.size(), NULL, &bytecodeSize);
					binary.resize(bytecodeSize);
					memcpy(binary.data(), (void*)bytecode, bytecodeSize);

					free(bytecode);
				}

				return hasErrors;
			}
		}
	};

	struct ScriptState
	{
		lua_State* L = nullptr;

		void Pop(int n = 1) { lua_pop(L, n); }
		void NewTable() { lua_newtable(L); }
		void CreateTable(int narr, int nrec) { lua_createtable(L, narr, nrec); }
		//void NewUserData(const std::string& s) { lua_newtable(L, s.c_str()); }

		auto IsString(int n = -1) { return lua_isstring(L, n); }
		auto IsNumber(int n = -1) { return lua_isnumber(L, n); }
		auto IsFunction(int n = -1) { return lua_isfunction(L, n); }
		auto IsTable(int n = -1) { return lua_istable(L, n); }
		auto IsLightUserData(int n = -1) { return lua_islightuserdata(L, n); }
		auto IsNil(int n = -1) { return lua_isnil(L, n); }
		auto IsBool(int n = -1) { return lua_isboolean(L, n); }
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
		void PushCFunction(lua_CFunction fn, const std::string& name) { lua_pushcfunction(L, fn, name.c_str()); }

		template<typename T>
		void Push(T value)
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

		auto Next(int idx) { return lua_next(L, idx); }
		std::string ToTypeName(int i = -1) { return std::string(lua_typename(L, lua_type(L, i))); }

		std::string ToString(int i = -1) { return std::string(lua_tostring(L, i)); }
		int32_t ToInt(int i = -1) { return lua_tointeger(L, i); }
		uint32_t ToUInt(int i = -1) { return lua_tounsigned(L, i); }
		double ToNumber(int i = -1) { return lua_tonumber(L, i); }
		double ToBool(int i = -1) { return lua_toboolean(L, i); }

		void SetField(const char* k, int i = -1) { lua_setfield(L, i, k); }
		void SetGlobal(const std::string& s) { lua_setglobal(L, s.c_str()); }
		auto GetGlobal(const std::string& s) { return lua_getglobal(L, s.c_str()); }

		void Register(const std::string& libName, const luaL_Reg* l) { luaL_register(L, libName.c_str(), l); }

		auto PCall(int nargs, int nresults, int errfunc) { return lua_pcall(L, nargs, nresults, errfunc); }
		auto Call(int nargs, int nresults) { return lua_call(L, nargs, nresults); }

		template<typename T>
		void SetVariable(const std::string& name, const T& value)
		{
			Push(value);
			SetGlobal(name);
		}

		template<typename T>
		T To()
		{
			if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>)					 return ToInt();
			else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)			 return ToNumber();
			else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, std::string>) return ToString();
			else if constexpr (std::is_same_v<T, bool>)									  		 return ToBool();
			else static_assert("Unsupported type for Lua");
		}

		template<typename T>
		T GetVariable(const std::string& name)
		{
			GetGlobal(name);

			auto val = To<T>();
			Pop();
			return val;
		}

		template<typename... Args>
		void Execute(const std::string& functionName, Args&&... args)
		{
			GetGlobal(functionName);

			if (!IsFunction())
			{
				//WC_ERROR("{} is not a function", functionName);
				return;
			}

			(PushValue(args), ...);

			constexpr int argCount = sizeof...(Args);
			//WC_CORE_INFO(GetTop());
			if (PCall(argCount, 0, 0) != LUA_OK)
			{
				// Print error message if the call fails
				const char* error = lua_tostring(L, -1);
				WC_ERROR("{}", error);
				Pop(); // Remove error message from the stack
			}
		}

		template<typename T>
		void RegisterEnumType(const std::string& tableName = "")
		{
			static_assert(magic_enum::is_scoped_enum<T>() || magic_enum::is_unscoped_enum<T>(), "Type is not an enum type");

			// Determine table name
			std::string actualTableName = tableName.empty() ? std::string(magic_enum::enum_type_name<T>()) : tableName;

			size_t enumCount = magic_enum::enum_count<T>();
			lua_createtable(L, 0, enumCount * 2);

			// Populate the table
			for (const auto [val, name] : magic_enum::enum_entries<T>())
			{
				lua_pushinteger(L, static_cast<lua_Integer>(val));
				lua_setfield(L, -2, std::string(name).c_str());

				lua_pushstring(L, std::string(name).c_str());
				lua_pushinteger(L, static_cast<lua_Integer>(val));
				lua_settable(L, -3);
			}

			// Create metatable
			lua_newtable(L);

			// Prevent adding or modifying keys
			static const luaL_Reg newIndexHandler = {
				"enum_freeze_error",
				[](lua_State* L) -> int {
					lua_pushstring(L, "attempt to modify a frozen enum table");
					return 1;
				}
			};

			// Add __index metamethod to enable table read operations
			lua_pushstring(L, "__index");
			lua_pushvalue(L, -3); // Push a copy of the actual table
			lua_settable(L, -3);

			// Add __newindex to prevent modifications
			lua_pushstring(L, "__newindex");
			lua_pushcclosure(L, newIndexHandler.func, "enum_freeze_error", 0);
			lua_settable(L, -3);

			// Protect the metatable itself
			lua_pushstring(L, "__metatable");
			lua_pushboolean(L, false);
			lua_settable(L, -3);

			// Apply the metatable
			lua_setmetatable(L, -2);

			// Register as global
			lua_setglobal(L, actualTableName.c_str());
		}

		int Load(const char* byteCode, size_t byteCodeSize, const std::string& name)
		{
			L = luaL_newstate();
			luaL_openlibs(L);			

			if (luau_load(L, name.c_str(), byteCode, byteCodeSize, 0) != LUA_OK)
			{
				WC_ERROR(std::string(byteCode, byteCodeSize));
				return LUA_ERRERR;
			}

			if (lua_pcall(L, 0, 0, 0) != LUA_OK)
			{
				WC_ERROR(std::string(byteCode, byteCodeSize));
				return LUA_ERRERR;
			}

			return LUA_OK;
		}

		int Load(const ScriptBinary& script)
		{
			if (script.binary.size() > 0)
				return Load((char*)script.binary.data(), script.binary.size(), script.Name);
		
			WC_CORE_ERROR("Script binary is empty");
			return LUA_ERRERR;
		}

		void Close() { lua_close(L); }		
	};
}