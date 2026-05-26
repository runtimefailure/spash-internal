#include <shared.hpp>
#include <execution/execution.hpp>
#include <environment/yield/yield.hpp>
#include <rbx/taskscheduler/scheduler.hpp>

#include <rbx/update/offsets.hpp>

class BytecodeEncoder : public Luau::BytecodeEncoder
{
	inline void encode(uint32_t* data, size_t count) override
	{
		for (auto i = 0; i < count;)
		{
			uint8_t Opcode = LUAU_INSN_OP(data[i]);
			const auto LookupTable = reinterpret_cast<BYTE*>(Structs::LuauOpcode);
			uint8_t FinalOpcode = Opcode * 227;
			FinalOpcode = LookupTable[FinalOpcode];

			data[i] = (FinalOpcode) | (data[i] & ~0xFF);
			i += Luau::getOpLength(static_cast<LuauOpcode>(Opcode));
		}
	}
};

std::string execution::compile(std::string Source)
{
	auto BytecodeEncoding = BytecodeEncoder();
	static const char* CommonGlobals[] = { "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV", nullptr };

	Luau::CompileOptions Options;
	Options.debugLevel = 1;
	Options.optimizationLevel = 1;
	Options.mutableGlobals = CommonGlobals;
	Options.vectorLib = "Vector3";
	Options.vectorCtor = "new";
	Options.vectorType = "Vector3";

	return Luau::compile(Source, Options, {}, &BytecodeEncoding);
}

void execution::execute(lua_State* L, std::string Script)
{
	if (Script.empty())
		return;

	int OriginalTop = lua_gettop(L);
	lua_State* executionThread = lua_newthread(L);
	lua_pop(L, 1);

	luaL_sandboxthread(executionThread);
	taskscheduler::set_thread(executionThread, 8, 8);

	std::string Bytecode = execution::compile(Script);
	lua_pushcclosure(executionThread, Roblox::TaskDefer, "task.defer", 0);
	if (luau_load(executionThread, "", Bytecode.c_str(), Bytecode.length(), 0) != LUA_OK)
	{
		std::string Error = lua_tostring(executionThread, -1);
		Roblox::Print(3, "%s", Error.c_str());
		lua_pop(executionThread, 1);
		return;
	}

	Closure* Closure = clvalue(luaA_toobject(executionThread, -1));
	taskscheduler::set_proto(Closure->l.p, reinterpret_cast<uintptr_t*>(MaxCapabilities));

	if (lua_pcall(executionThread, 1, NULL, NULL) != LUA_OK)
	{
		std::string Error = lua_tostring(executionThread, -1);
		Roblox::Print(3, "%s", Error.c_str());
		lua_pop(executionThread, 1);
		return;
	}

	lua_settop(executionThread, 0);
	lua_settop(L, OriginalTop);
}