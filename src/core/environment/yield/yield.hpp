#pragma once

#include <functional>
#include <memory>
#include <lua.h>
#include <thread>

using yielded = std::function<int(lua_State*)>;
namespace yield
{
	int YieldExecution(lua_State* L, const std::function<yielded()>& Function);
	void RunYield();
}