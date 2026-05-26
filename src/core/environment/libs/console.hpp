#include "../env.hpp"
#include <lgc.h>
#include <lmem.h>
#include <lualib.h>
#include <winternl.h>
#include <config.hpp>
#include <iostream>

std::uintptr_t old_handle = 0x0;

namespace console {

	int rconsolecreate(lua_State* L)
	{
		lua_normalisestack(L, 0);

		if (old_handle != 0x0)
			return 0;

		AllocConsole();

		auto peb = reinterpret_cast<std::uintptr_t>(NtCurrentTeb()->ProcessEnvironmentBlock);
		std::uintptr_t process_params = * reinterpret_cast<std::uintptr_t *>(peb + 0x20);
		old_handle = * reinterpret_cast<std::uintptr_t *>(process_params + 0x10);

		FILE* fDummy;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = 0;

		return 0;
	}

	int rconsoledestroy(lua_State* L)
	{
		lua_normalisestack(L, 0);

		auto peb = reinterpret_cast<std::uintptr_t>(NtCurrentTeb()->ProcessEnvironmentBlock);
		std::uintptr_t process_params = * reinterpret_cast<std::uintptr_t *>(peb + 0x20);

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = old_handle;

		if (const auto hWnd = GetConsoleWindow(); hWnd && hWnd != INVALID_HANDLE_VALUE) {
			ShowWindow(hWnd, 0);
			FreeConsole();
		}

		old_handle = 0x0;

		return 0;
	}

	int rconsoleprint(lua_State* L)
	{
		lua_normalisestack(L, 1);
		luaL_checktype(L, 1, LUA_TSTRING);

		size_t len;
		const char* content = lua_tolstring(L, 1, &len);

		auto peb = reinterpret_cast<std::uintptr_t>(NtCurrentTeb()->ProcessEnvironmentBlock);
		std::uintptr_t process_params = * reinterpret_cast<std::uintptr_t *>(peb + 0x20);

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = old_handle;

		std::cout.write(content, len);
		std::cout.flush();

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = 0;

		return 0;
	}

	int rconsolesettitle(lua_State* L)
	{
		lua_normalisestack(L, 1);
		luaL_checktype(L, 1, LUA_TSTRING);

		size_t len;
		const char* title = lua_tolstring(L, 1, &len);

		auto peb = reinterpret_cast<std::uintptr_t>(NtCurrentTeb()->ProcessEnvironmentBlock);
		std::uintptr_t process_params = * reinterpret_cast<std::uintptr_t *>(peb + 0x20);

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = old_handle;

		SetConsoleTitleA(title);

		* reinterpret_cast<std::uintptr_t *>(process_params + 0x10) = 0;

		return 0;
	}

	int empty(lua_State* L)
	{
		return 0;
	}

	void initialize(lua_State* L)
	{
		Function(L, "rconsolecreate", 	console::rconsolecreate);
		Function(L, "rconsoledestroy", 	console::rconsoledestroy);
		Function(L, "rconsolesettitle", console::rconsolesettitle);
		Function(L, "rconsoleprint", 	console::rconsoleprint);
		Function(L, "rconsoleinput", 	console::empty);

		Function(L, "consolecreate", 	console::rconsolecreate);
		Function(L, "consoledestroy", 	console::rconsoledestroy);
		Function(L, "rconsolename", 	console::rconsolesettitle);
		Function(L, "consolesettitle", 	console::rconsolesettitle);
		Function(L, "consoleprint", 	console::rconsoleprint);
		Function(L, "rconsolewarn", 	console::rconsoleprint);
		Function(L, "consolewarn", 		console::rconsoleprint);
		Function(L, "rconsoleerror", 	console::rconsoleprint);
		Function(L, "consoleerror", 	console::rconsoleprint);
		Function(L, "rconsoleerr", 		console::rconsoleprint);
		Function(L, "consoleinput", 	console::empty);
		Function(L, "rconsoleclear", 	console::empty);
		Function(L, "setfflag", 		console::empty);
		Function(L, "getfflag", 		console::empty);
		Function(L, "consoleclear", 	console::empty);
		Function(L, "rconsolehide", 	console::empty);
		Function(L, "consolehide", 		console::empty);
		Function(L, "rconsoleshow", 	console::empty);
		Function(L, "consoleshow", 		console::empty);
		Function(L, "rconsoleinfo", 	console::empty);
		Function(L, "consoleinfo", 		console::empty);

		lua_newtable(L);
		TableFunction(L, "create", 		console::rconsolecreate);
		TableFunction(L, "destroy", 	console::rconsoledestroy);
		TableFunction(L, "settitle", 	console::rconsolesettitle);
		TableFunction(L, "name", 		console::rconsolesettitle);
		TableFunction(L, "print", 		console::rconsoleprint);
		TableFunction(L, "warn", 		console::rconsoleprint);
		TableFunction(L, "error", 		console::rconsoleprint);
		TableFunction(L, "err", 		console::rconsoleprint);
		TableFunction(L, "input", 		console::empty);
		TableFunction(L, "clear", 		console::empty);
		TableFunction(L, "hide", 		console::empty);
		TableFunction(L, "show", 		console::empty);
		TableFunction(L, "info", 		console::empty);
		lua_setfield(L, LUA_GLOBALSINDEX, "console");
	}
}