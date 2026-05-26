#include "core/execution/execution.hpp"
#include "taskscheduler/scheduler.hpp"
#include "rbx/update/offsets.hpp"
#include "misc/shared/shared.hpp"

#include <lua.h>
#include <lstate.h>
#include <lapi.h>
#include <lualib.h>
#include "tphandler.hpp"

int loadstring(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char* script = lua_tostring(L, 1);
	const char* chunkname = luaL_optstring(L, 2, "Base-LoadString");
	std::string bytecode = execution::compile(script);
	if (bytecode[0] == '\0' || bytecode.empty()) {
		const char* err = bytecode.c_str() + 1;
		lua_pushnil(L);
		lua_pushstring(L, err);
		return 2;
	}

	if (luau_load(L, chunkname, bytecode.c_str(), bytecode.length(), 0) != LUA_OK) {
		lua_pushnil(L);
		lua_pushvalue(L, -2);
		return 2;
	}

	Closure* function = clvalue(luaA_toobject(L, -1));
	if (function && function->l.p)
		taskscheduler::set_proto(function->l.p, &MaxCapabilities);

	lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
	return 1;
}

void tphandler::initenv(uintptr_t datamodel)
{
	if (!datamodel)
		return;

	if (taskscheduler::get_placeid(datamodel) == 0)
		return;

	while (taskscheduler::get_gameloaded(datamodel) == 15) {
		Sleep(100);
	}
	
	uintptr_t context = taskscheduler::get_scriptcontext(datamodel);
	if (!context)
		return;

	uintptr_t identity = 0, script = 0;
	uintptr_t main = taskscheduler::get_globalstate(context, &identity, &script);
	if (!main)
		return;

	lua_State* rbx_state = reinterpret_cast<lua_State*>(main);
	if (!rbx_state)
		return;

	lua_State* ExecutorState = lua_newthread(rbx_state);
	//RBX::ExecutorState = ExecutorState;
	taskscheduler::set_thread(ExecutorState, 9, true);
	luaL_sandboxthread(ExecutorState);

	lua_pushcfunction(ExecutorState, loadstring, 0);
	lua_setglobal(ExecutorState, "loadstring");

	lua_getglobal(ExecutorState, "print");
	lua_pushstring(ExecutorState, "injected");
	lua_call(ExecutorState, 1, 0);

	taskscheduler::request("print('diegosploit ud')");
}

void tphandler::reset()
{}
void tphandler::initialize()
{}