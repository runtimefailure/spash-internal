#include <shared.hpp>
#include "yield.hpp"

#include <rbx/update/offsets.hpp>

void yield::RunYield()
{
	if (!shared::YieldQueue.empty())
	{
		std::function<void()> yieldRequest = shared::YieldQueue.front();
		shared::YieldQueue.pop();
		yieldRequest();
	}
}

int yield::YieldExecution(lua_State* L, const std::function<yielded()>& yieldClosure)
{
    lua_pushthread(L);
    int yieldedThreadRef = lua_ref(L, -1);
    lua_pop(L, 1);

    yielded resume = yieldClosure();
    
    shared::YieldQueue.emplace([L, yieldedThreadRef, resume]() -> void
    {
        WeakThreadRef threadRef = { L };
        lua_pushthread(L);
        threadRef.thread_ref = yieldedThreadRef;
        lua_pop(L, 1);

        WeakThreadRef* pWeakThreadRef = &threadRef;
        DebuggerResult_T debuggerResult = { 0 };

        auto* ud = reinterpret_cast<RobloxExtraSpace*>(L->userdata);
        auto scriptContextAddr = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(ud) + Offsets::ScriptContext);
        
        Roblox::SCResume(scriptContextAddr, &debuggerResult, &pWeakThreadRef, resume(L), 0, nullptr);

        lua_unref(L, yieldedThreadRef);
    });

    return lua_yield(L, 0);
}