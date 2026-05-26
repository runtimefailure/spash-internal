#pragma once

#include <windows.h>
#include <string>
#include <lstate.h>
#include <lualib.h>

namespace taskscheduler
{
	void request(std::string script);
	void set_proto(Proto* proton, uintptr_t* comp);
	void set_thread(lua_State* L, int level, uintptr_t comp);
	uintptr_t set_identity(lua_State* thread, int identity, bool isinstance);

	uintptr_t get_placeid(uintptr_t datamodel);
	uintptr_t get_gameloaded(uintptr_t datamodel);
	uintptr_t get_globalstate(uintptr_t scriptcontext, uintptr_t* identity, uintptr_t* script);
	uintptr_t get_jobbyname(std::string jobname);
	uintptr_t get_specificjobbyname(std::string jobname, uintptr_t offset, uintptr_t datamodel);
	uintptr_t get_capabilities(int identity);

	uintptr_t get_datamodel();
	uintptr_t get_scriptcontext(uintptr_t datamodel);
	lua_State* decrypt(uintptr_t scriptcontext);
}