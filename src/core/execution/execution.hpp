#pragma once

#include <Luau/BytecodeBuilder.h>
#include <Luau/BytecodeUtils.h>
#include <Luau/Compiler.h>
#include <windows.h>
#include <lstate.h>
#include <lualib.h>
#include <string>
#include <lapi.h>

namespace execution
{
	std::string compile(std::string Source);
	void execute(lua_State* L, std::string Script);
}