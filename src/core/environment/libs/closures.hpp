#pragma once

#include <Windows.h>
#include <lstate.h>
#include <lgc.h>

#include <shared.hpp>
#include <execution/execution.hpp>
#include <rbx/taskscheduler/scheduler.hpp>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>
#include <format>
#include <regex>
#include <unordered_set>
#include <ldo.h>
#include <lapi.h>
#include <lfunc.h>
#include <lgc.h>
#include <execution>

TValue* index2addr(lua_State* L, int idx);
#define lua_normalisestack(L, maxSize) { if (lua_gettop(L) > maxSize) lua_settop(L, maxSize); }
#define lua_pushrawclosure(L, rawClosure) \
setclvalue(L, L->top, rawClosure); \
incr_top(L)
#define lua_toclosure(L, i) (Closure*)lua_topointer(L, i)

namespace closures
{
    enum class ClosureType : uint32_t
    {
        None = 0,
        RobloxClosure,
        LuauClosure,
        ExecutorFunction,
        NewCClosure,
    };

    static std::unordered_map<Closure*, Closure*> s_Newcclosures = {};
    static std::unordered_map<Closure*, Closure*> s_HookedFunctions = {};
    static std::unordered_map<Closure*, Closure*> original_functions = {};
    static std::unordered_map<Closure*, lua_CFunction> s_ExecutorClosures = {};
    static std::unordered_set<Closure*> s_ExecutorFunctions = {};
    static std::unordered_map<Closure*, int> s_TrampolineEnvRefs = {};

    enum class FunctionKind { NewCClosure, CClosure, LuauClosure };

    Closure* FindSavedCClosure(Closure* closure)
    {
        auto it = s_Newcclosures.find(closure);
        return it != s_Newcclosures.end() ? it->second : nullptr;
    }

    // Handler functions
    static void handler_run(lua_State* L, void* ud)
    {
        luaD_call(L, (StkId)ud, LUA_MULTRET);
    }

    std::string ErrorMessage(const std::string& message) {
        static const std::regex callstack_regex(
            R"(.*"\]:(\d)*: )",
            std::regex_constants::optimize | std::regex_constants::icase);
        if (std::regex_search(message.begin(), message.end(), callstack_regex)) {
            const auto fixed = std::regex_replace(message, callstack_regex, "");
            return fixed;
        }

        return message;
    }

    static int ClosuresHandler(lua_State* L)
    {
        auto found = s_ExecutorClosures.find(curr_func(L));
        if (found != s_ExecutorClosures.end()) {
            return found->second(L);
        }
        return 0;
    }

    int NewCClosureContinuation(lua_State* L, std::int32_t status) {
        if (status != LUA_OK) {
            std::size_t error_len;
            const char* errmsg = luaL_checklstring(L, -1, &error_len);
            lua_pop(L, 1);
            std::string error(errmsg);

            if (error == std::string(("attempt to yield across metamethod/C-call boundary")))
                return lua_yield(L, LUA_MULTRET);

            std::string fixedError = ErrorMessage(error);
            const std::regex pattern(R"([^:]+:\d+:\s?)");

            std::smatch match;
            if (std::regex_search(fixedError, match, pattern)) {
                fixedError.erase(match.position(), match.length());
            }

            lua_pushlstring(L, fixedError.data(), fixedError.size());
            lua_error(L);
            return 0;
        }

        return lua_gettop(L);
    };

    int NewCClosureStub(lua_State* L) {
        int nArgs = lua_gettop(L);

        Closure* cl = clvalue(L->ci->func);
        if (!cl) {
            luaL_error(L, "Invalid closure (NewCClosureStub 1)");
        }

        Closure* originalClosure = FindSavedCClosure(cl);
        if (!originalClosure) {
            luaL_error(L, "Invalid closure (NewCClosureStub 2)");
        }

        setclvalue(L, L->top, originalClosure);
        L->top++;

        lua_insert(L, 1);

        StkId func = L->base;
        L->ci->flags |= LUA_CALLINFO_HANDLE;

        L->baseCcalls++;
        int status = luaD_pcall(L, handler_run, func, savestack(L, func), 0);
        L->baseCcalls--;

        if (status == LUA_ERRRUN) {
            size_t error_len;
            const char* errmsg = luaL_checklstring(L, -1, &error_len);
            lua_pop(L, 1);
            std::string error(errmsg);

            if (error == std::string(("attempt to yield across metamethod/C-call boundary")))
                return lua_yield(L, LUA_MULTRET);

            std::string fixedError = ErrorMessage(error);
            std::regex pattern(R"([^:]+:\d+:\s?)");

            std::smatch match;
            if (std::regex_search(fixedError, match, pattern)) {
                fixedError.erase(match.position(), match.length());
            }

            lua_pushlstring(L, fixedError.data(), fixedError.size());
            lua_error(L);
            return 0;
        }

        expandstacklimit(L, L->top);

        if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
            return -1;

        return lua_gettop(L);
    }

    int NonYieldNewCClosureStub(lua_State* L) {
        const auto nArgs = lua_gettop(L);

        Closure* cl = clvalue(L->ci->func);
        if (!cl)
            luaL_error(L, ("Invalid closure (NonYieldNewCClosureStub 1)"));

        const auto originalClosure = FindSavedCClosure(cl);
        if (!originalClosure)
            luaL_error(L, ("Invalid closure (NonYieldNewCClosureStub 2)"));

        setclvalue(L, L->top, originalClosure);
        L->top++;

        lua_insert(L, 1);

        StkId func = L->base;
        L->ci->flags |= LUA_CALLINFO_HANDLE;

        L->baseCcalls++;
        int status = luaD_pcall(L, handler_run, func, savestack(L, func), 0);
        L->baseCcalls--;

        if (status == LUA_ERRRUN) {
            std::size_t error_len;
            const char* errmsg = luaL_checklstring(L, -1, &error_len);
            lua_pop(L, 1);
            std::string error(errmsg);

            if (error == std::string(("attempt to yield across metamethod/C-call boundary")))
                return lua_yield(L, 0);

            std::string fixedError = ErrorMessage(error);
            std::regex pattern(R"([^:]+:\d+:\s?)");

            std::smatch match;
            if (std::regex_search(fixedError, match, pattern)) {
                fixedError.erase(match.position(), match.length());
            }

            lua_pushlstring(L, fixedError.data(), fixedError.size());
            lua_error(L);
            return 0;
        }

        expandstacklimit(L, L->top);

        if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
            return -1;

        return lua_gettop(L);
    }

    static ClosureType GetClosureType(Closure* closure)
    {
        if (!closure->isC) {
            return ClosureType::LuauClosure;
        }

        // Treat executor-managed cclosures created via either ClosuresHandler or
        // the newer handler() trampoline as non-Roblox closures
        if (closure->c.f != ClosuresHandler) {
            return ClosureType::RobloxClosure;
        }

        if (auto it = s_ExecutorClosures.find(closure); it != s_ExecutorClosures.end()) {
            if (it->second == NewCClosureStub || it->second == NonYieldNewCClosureStub)
                return ClosureType::NewCClosure;
            return ClosureType::ExecutorFunction;
        }

        // If we reached here, treat as executor function (defensive)
        return ClosureType::ExecutorFunction;
    }

    static void WrapClosure(lua_State* L, int idx, const char* debugname = nullptr)
    {

        Closure* oldClosure = clvalue(index2addr(L, idx));
        if (GetClosureType(oldClosure) == ClosureType::NewCClosure) {
            lua_pushvalue(L, idx);
            return;
        }

        lua_ref(L, idx);
        lua_pushcclosurek(L, ClosuresHandler, debugname, 0, NewCClosureContinuation);

        Closure* newClosure = clvalue(index2addr(L, -1));
        s_ExecutorClosures[newClosure] = NewCClosureStub;
        s_ExecutorFunctions.insert(newClosure);

        newClosure->isC = 1;
        newClosure->env = oldClosure->env;

        lua_ref(L, -1);

        s_Newcclosures[newClosure] = oldClosure;
    }

    // Public API functions
    static int iscclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* closure = clvalue(luaA_toobject(L, 1));
        lua_pushboolean(L, closure && closure->isC);
        return 1;
    }

    static int islclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* closure = clvalue(luaA_toobject(L, 1));
        lua_pushboolean(L, closure && !closure->isC);
        return 1;
    }

    int isexecutorclosure(lua_State* rl) {
        if (lua_type(rl, 1) != LUA_TFUNCTION) { lua_pushboolean(rl, false); return 1; }
        Closure* closure = clvalue(luaA_toobject(rl, 1));
        bool value = false;

        if (lua_isLfunction(rl, 1)) {
            value = closure->l.p->linedefined;
        }
        else {
            value = s_ExecutorFunctions.find(closure) != s_ExecutorFunctions.end();
        }

        lua_pushboolean(rl, value);
        return 1;
    }

    static int isnewcclosure(lua_State* L)
    {
        if (lua_type(L, 1) != LUA_TFUNCTION) { lua_pushboolean(L, false); return 1; }
        Closure* cl = clvalue(luaA_toobject(L, 1));
        lua_pushboolean(L, s_Newcclosures.contains(cl));
        return 1;
    }

    int newcclosure(lua_State* L)
    {
        lua_normalisestack(L, 2);
        luaL_checktype(L, 1, LUA_TFUNCTION);

        Closure* closure = clvalue(index2addr(L, 1));
        if (closure->isC) {
            lua_pushvalue(L, 1);
            return 1;
        }

        const char* debugname = lua_isstring(L, 2) ? lua_tostring(L, 2) : nullptr;
        WrapClosure(L, 1, debugname);
        return 1;
    }

    int checkcaller(lua_State* L)
    {
        const auto ud_base = reinterpret_cast<std::uintptr_t>(L->userdata);
        if (!ud_base) {
            lua_pushboolean(L, false);
            return 1;
        }
        const auto script_ptr = *reinterpret_cast<std::uintptr_t const*>(ud_base + 0x50);
        lua_pushboolean(L, script_ptr == 0);
        return 1;
    }

    int clonefunction(lua_State* L)
    {
        lua_normalisestack(L, 1);
        luaL_checktype(L, 1, LUA_TFUNCTION);

        Closure* closure = clvalue(index2addr(L, 1));
        if (!closure) {
            luaL_error(L, "Invalid closure provided in clonefunction");
        }

        switch (GetClosureType(closure))
        {
        case ClosureType::ExecutorFunction:
            lua_clonefunction(L, 1);
            s_ExecutorClosures[clvalue(index2addr(L, -1))] = s_ExecutorClosures.at(closure);
            break;
        case ClosureType::RobloxClosure:
            lua_clonefunction(L, 1);
            break;
        case ClosureType::LuauClosure:
            lua_clonefunction(L, 1);
            break;
        case ClosureType::None:
            luaL_argerror(L, 1, "Type of closure unsupported");
        }

        return 1;
    }

    static bool IsWrappedClosure(Closure* cl)
    {
        return cl->isC && s_Newcclosures.contains(cl);
    }

    static Closure* get_backing_lclosure(Closure* nc)
    {
        if (!nc) return nullptr;
        // Prefer s_Newcclosures when present
        auto it = s_Newcclosures.find(nc);
        if (it != s_Newcclosures.end() && it->second)
            return it->second;
        return nullptr;
    }

    int hookfunction(lua_State* L)
    {
        lua_normalisestack(L, 2);
        luaL_checktype(L, 1, LUA_TFUNCTION);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        Closure* hookWhat = clvalue(index2addr(L, 1));
        Closure* hookWith = clvalue(index2addr(L, 2));

        if (!hookWhat || !hookWith) {
            luaL_error(L, "Invalid closures");
        }

        ClosureType hookWhatType = GetClosureType(hookWhat);
        ClosureType hookWithType = GetClosureType(hookWith);

        // Save the original function if it hasn't been saved already
        if (!s_HookedFunctions.contains(hookWhat)) {
            // Avoid dependency on clonefunction reliability: always duplicate via raw push
            if (lua_iscfunction(L, 1)) lua_clonefunction(L, 1); else lua_clonefunction(L, 1);

            Closure* originalClone = clvalue(index2addr(L, -1));
            s_HookedFunctions[hookWhat] = originalClone;
            original_functions[hookWhat] = originalClone; // Save to original_functions for restore
            lua_pop(L, 1);
        }
        //INFO("What: {}", (int)hookWhatType);
        //INFO("With: {}", (int)hookWithType);

        if (!s_HookedFunctions.contains(hookWhat)) {
            // Avoid dependency on clonefunction reliability: always duplicate via raw push
            if (lua_iscfunction(L, 1)) lua_clonefunction(L, 1); else lua_clonefunction(L, 1);

            Closure* originalClone = clvalue(index2addr(L, -1));
            s_HookedFunctions[hookWhat] = originalClone;
            lua_pop(L, 1);
        }

        if (hookWhatType == ClosureType::RobloxClosure && hookWithType == ClosureType::RobloxClosure) { // Works
            // debug: C->C hooking

            // Clone the "hookWhat" to be returned
            lua_clonefunction(L, 1);
            if (auto it = s_Newcclosures.find(hookWhat); it != s_Newcclosures.end()) {
                s_Newcclosures[clvalue(index2addr(L, -1))] = it->second;
            }

            hookWhat->c.f = hookWith->c.f;
            hookWhat->c.cont = hookWith->c.cont;
            hookWhat->env = hookWith->env;
            hookWhat->stacksize = hookWith->stacksize;
            hookWhat->preload = hookWith->preload;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            if (auto it = s_Newcclosures.find(hookWith); it != s_Newcclosures.end()) {
                s_Newcclosures[hookWhat] = it->second;
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::LuauClosure && hookWithType == ClosureType::LuauClosure) { // Works
            // debug: L->L hooking

            if (hookWhat->nupvalues < hookWith->nupvalues) {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            lua_clonefunction(L, 1);
            // Do not lua_ref here; we'll return and not keep a registry ref for the original clone

            hookWhat->l.p = hookWith->l.p;
            //OldClosure->nupvalues = HookClosure->nupvalues;
            hookWhat->stacksize = hookWith->stacksize;
            //OldClosure->preload = HookClosure->preload;
            //OldClosure->memcat = HookClosure->memcat;
            //OldClosure->marked = HookClosure->marked;

            //OldClosure->env = (LuaTable*)HookClosure->env;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->l.uprefs[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj(L, &hookWhat->l.uprefs[i], &hookWith->l.uprefs[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::NewCClosure && hookWithType == ClosureType::NewCClosure) {
            // debug: NC->NC hooking

            lua_clonefunction(L, 1);

            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;

            if (auto itn = s_Newcclosures.find(hookWhat); itn != s_Newcclosures.end())
                s_Newcclosures[clvalue(index2addr(L, -1))] = itn->second;
            if (auto itnw = s_Newcclosures.find(hookWith); itnw != s_Newcclosures.end())
                s_Newcclosures[hookWhat] = itnw->second;

            hookWhat->stacksize = hookWith->stacksize;
            hookWhat->env = hookWith->env;

            return 1;
        }
        else if (hookWhatType == ClosureType::ExecutorFunction && hookWithType == ClosureType::ExecutorFunction) {
            // debug: EX->EX hooking

            if (hookWhat->nupvalues < hookWith->nupvalues) {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            lua_clonefunction(L, 1);
            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;

            //Handler::SetClosure(hookWhat, Handler::GetClosure(hookWith));
            if (auto it = s_ExecutorClosures.find(hookWith); it != s_ExecutorClosures.end())
                s_ExecutorClosures[hookWhat] = it->second;
            hookWhat->c.cont = (lua_Continuation)hookWith->c.cont;
            hookWhat->env = (LuaTable*)hookWith->env;

            // TODO: Maybe this?
            //for (int i = 0; i < hookWhat->nupvalues; i++) {
            //    setobj2n(L, &hookWhat->l.uprefs[i], luaO_nilobject);
            //}

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::ExecutorFunction && hookWithType == ClosureType::RobloxClosure) {
            // debug: EX->C hooking

            if (hookWhat->nupvalues < hookWith->nupvalues)
            {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            lua_clonefunction(L, 1);

            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;

            hookWhat->env = hookWith->env;
            hookWhat->c.f = hookWith->c.f;
            hookWhat->c.cont = hookWith->c.cont;
            hookWhat->stacksize = hookWith->stacksize;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::ExecutorFunction && hookWithType == ClosureType::LuauClosure) {
            // EX->L hooking: wrap L closure into a handler trampoline (NC behavior)
            lua_clonefunction(L, 1); // return original C function
            s_ExecutorClosures[clvalue(index2addr(L, -1))] = s_ExecutorClosures.at(hookWhat);

            s_ExecutorClosures[hookWhat] = NonYieldNewCClosureStub;
            s_Newcclosures[hookWhat] = hookWith;
            return 1;
        }
        else if (hookWhatType == ClosureType::ExecutorFunction && hookWithType == ClosureType::NewCClosure) {
            // debug: EX->NC hooking

            lua_clonefunction(L, 1);
            //Handler::SetClosure(clvalue(index2addr(L, -1)), Handler::GetClosure(hookWhat));
            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;

            //Handler::SetClosure(hookWhat, NonYieldNewCClosureStub);
            s_ExecutorClosures[hookWhat] = NonYieldNewCClosureStub;
            //Handler::Wraps::SetClosure(hookWhat, hookWith);
            s_Newcclosures[hookWhat] = hookWith;

            return 1;
        }
        else if (hookWhatType == ClosureType::NewCClosure && hookWithType == ClosureType::LuauClosure) {
            // debug: NC->L hooking

            // Return original C clone
            lua_clonefunction(L, 1);
            Closure* ret = clvalue(index2addr(L, -1));
            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[ret] = it->second;
            if (auto it2 = s_Newcclosures.find(hookWhat); it2 != s_Newcclosures.end())
                s_Newcclosures[ret] = it2->second;

            // ClosuresHandler/NC stub-based: use NonYield stub with s_Newcclosures lookup
            s_ExecutorClosures[hookWhat] = NonYieldNewCClosureStub;
            s_Newcclosures[hookWhat] = hookWith;
            return 1;
        }
        else if (hookWhatType == ClosureType::NewCClosure && hookWithType == ClosureType::RobloxClosure) {
            // debug: NC->C hooking

            if (hookWhat->nupvalues < hookWith->nupvalues) {
                luaL_error(L, "First closure has less upvalues than second closure");
            }

            lua_clonefunction(L, 1);
            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;
            if (auto itn = s_Newcclosures.find(hookWhat); itn != s_Newcclosures.end())
                s_Newcclosures[clvalue(index2addr(L, -1))] = itn->second;

            hookWhat->env = hookWith->env;
            hookWhat->c.f = hookWith->c.f;
            hookWhat->c.cont = hookWith->c.cont;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::NewCClosure && hookWithType == ClosureType::ExecutorFunction) {
            // debug: NC->EX hooking

            if (hookWhat->nupvalues < hookWith->nupvalues)
            {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            lua_clonefunction(L, 1);
            //Handler::SetClosure(clvalue(index2addr(L, -1)), Handler::GetClosure(hookWhat));
            if (auto it = s_ExecutorClosures.find(hookWhat); it != s_ExecutorClosures.end())
                s_ExecutorClosures[clvalue(index2addr(L, -1))] = it->second;
            if (auto itn = s_Newcclosures.find(hookWhat); itn != s_Newcclosures.end())
                s_Newcclosures[clvalue(index2addr(L, -1))] = itn->second;

            //Handler::SetClosure(hookWhat, Handler::GetClosure(hookWith));
            if (auto it = s_ExecutorClosures.find(hookWith); it != s_ExecutorClosures.end())
                s_ExecutorClosures[hookWhat] = it->second;

            hookWhat->env = (LuaTable*)hookWith->env;
            hookWhat->c.cont = hookWith->c.cont;

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::RobloxClosure && hookWithType == ClosureType::LuauClosure) { // Works
            // debug: C->L hooking

            // Build an NC wrapper over hookWith and assign to C function
            WrapClosure(L, 2);
            s_Newcclosures[hookWhat] = clvalue(index2addr(L, -1));

            // Clone the "hookWhat" to be returned
            lua_clonefunction(L, 1);

            hookWhat->c.f = NewCClosureStub;
            hookWhat->c.cont = NewCClosureContinuation;

            return 1;
        }
        else if (hookWhatType == ClosureType::RobloxClosure && hookWithType == ClosureType::ExecutorFunction) {
            // debug: C->EX hooking

            if (hookWhat->nupvalues < hookWith->nupvalues) {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            lua_clonefunction(L, 1);

            //Handler::SetClosure(hookWhat, Handler::GetClosure(hookWith));
            if (auto it = s_ExecutorClosures.find(hookWith); it != s_ExecutorClosures.end())
                s_ExecutorClosures[hookWhat] = it->second;

            hookWhat->c.f = hookWith->c.f;
            hookWhat->c.cont = hookWith->c.cont;
            hookWhat->env = (LuaTable*)hookWith->env;
            hookWhat->stacksize = hookWith->stacksize;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::RobloxClosure && hookWithType == ClosureType::NewCClosure) { // Works
            // debug: C->NC hooking

            lua_clonefunction(L, 1);

            //Handler::SetClosure(hookWhat, Handler::GetClosure(hookWith));
            if (auto it = s_ExecutorClosures.find(hookWith); it != s_ExecutorClosures.end())
                s_ExecutorClosures[hookWhat] = it->second;
            if (auto itn = s_Newcclosures.find(hookWith); itn != s_Newcclosures.end())
                s_Newcclosures[hookWhat] = itn->second;

            hookWhat->c.f = hookWith->c.f;
            hookWhat->c.cont = hookWith->c.cont;
            hookWhat->env = (LuaTable*)hookWith->env;
            hookWhat->stacksize = hookWith->stacksize;

            return 1;
        }
        else if (hookWhatType == ClosureType::LuauClosure && hookWithType == ClosureType::RobloxClosure) {
            // L -> C for sUNC: return a clone of the original Lua function, and make it call the C closure
            lua_clonefunction(L, 1);

            // Build env with C closure under key 'asshole'
            lua_newtable(L);
            lua_newtable(L);
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            lua_setfield(L, -2, "__index");
            lua_setmetatable(L, -2);
            lua_pushrawclosure(L, hookWith);
            lua_setfield(L, -2, "asshole");

            const char* src = "return asshole(...)";
            const std::string& bc = execution::compile(src);
            if (luau_load(L, "@Exec_LtoC", bc.data(), bc.size(), 0) != LUA_OK)
            {
                // Clean env
                lua_pop(L, 1);
                return 1; // still return original clone
            }
            Closure* thunk = lua_toclosure(L, -1);
            if (thunk) {
                thunk->env = hvalue(luaA_toobject(L, -2));
                luaC_threadbarrier(L);
                if (thunk->l.p) taskscheduler::set_proto(thunk->l.p, &MaxCapabilities);

                hookWhat->l.p = thunk->l.p;
                hookWhat->env = thunk->env;
                luaC_threadbarrier(L);
                for (int i = 0; i < hookWhat->nupvalues; i++) {
                    setobj2n(L, &hookWhat->l.uprefs[i], luaO_nilobject);
                }
            }
            // pop thunk and env
            lua_pop(L, 2);
            return 1;
        }
        else if (hookWhatType == ClosureType::LuauClosure && hookWithType == ClosureType::NewCClosure) {
            // L->NC hooking: resolve the backing Luau closure for the NC
            const Closure* backing = get_backing_lclosure(hookWith);
            if (!backing) { luaL_error(L, "Failed to find closure"); return 0; }

            if (hookWhat->nupvalues < backing->nupvalues)
            {
                luaL_error(L, "First closure has less upvalues than second closure");
                return 0;
            }

            // Create a fresh Luau wrapper that forwards to the NC via _ENV.asshole
            // Build env with reference to hookWith and set as env of a compiled thunk
            lua_newtable(L);
            lua_newtable(L);
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            lua_setfield(L, -2, "__index");
            lua_setmetatable(L, -2);
            // env.asshole = hookWith
            lua_pushrawclosure(L, (Closure*)hookWith);
            lua_setfield(L, -2, "asshole");

            const char* src = "return asshole(...)";
            const std::string& bc = execution::compile(src);
            if (luau_load(L, "@Exec_LtoNC", bc.data(), bc.size(), 0) != LUA_OK)
            {
                lua_settop(L, 0);
                lua_pushnil(L);
                return 0;
            }
            Closure* thunk = lua_toclosure(L, -1);
            if (!thunk) { lua_pop(L, 1); /* env */ lua_pushnil(L); return 0; }
            thunk->env = hvalue(luaA_toobject(L, -2));
            luaC_threadbarrier(L);
            taskscheduler::set_proto(thunk->l.p, &MaxCapabilities);

            // Clone prototype to avoid sharing code with different envs
            // Replace target Luau function's proto/env with the thunk's
            hookWhat->l.p = thunk->l.p;
            hookWhat->env = (LuaTable*)thunk->env;
            luaC_threadbarrier(L);

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->l.uprefs[i], luaO_nilobject);
            }
            for (int i = 0; i < backing->nupvalues; i++) {
                setobj2n(L, &hookWhat->l.uprefs[i], &backing->l.uprefs[i]);
            }
            // Clean stack: pop thunk and env
            lua_pop(L, 2);
            return 1;
        }
        else if (hookWhatType == ClosureType::LuauClosure && (hookWithType == ClosureType::ExecutorFunction)) // L -> EX only; L->C handled above
        {
            // debug: L->C/L->EX hooking
            lua_clonefunction(L, 1);

            lua_pop(L, 1);

            int hookRef = lua_ref(L, 2);

            // Prepare a dedicated environment table that delegates to globals and
            // stores the target callable at key "asshole" without using setfenv.
            lua_newtable(L);                 // [ ... env ]
            lua_newtable(L);                 // [ ... env mt ]
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            lua_setfield(L, -2, "__index"); // mt.__index = _G
            lua_setmetatable(L, -2);         // setmetatable(env, mt)
            lua_getref(L, hookRef);
            lua_setfield(L, -2, "asshole"); // env.asshole = hookWith

            // Build trampoline that calls env.asshole(...)
            {
                const char* src = "return asshole(...)";
                const std::string& bc = execution::compile(src);
                if (luau_load(L, "@Exec_hook", bc.data(), bc.size(), 0) != LUA_OK)
                {
                    lua_pushnil(L);
                    return 0;
                }
            }

            // new closure on top, env below; set closure env pointer directly and
            // clone closure so we don't mutate shared prototypes that other code may inspect
            lua_clonefunction(L, -1); // duplicate closure as a fresh L closure
            lua_remove(L, -2);        // remove the original to keep stack predictable
            // stack: ... [env] [closure]
            Closure* newLClosure = lua_toclosure(L, -1);
            LuaTable* newEnv = hvalue(luaA_toobject(L, -2));
            if (newLClosure) {
                newLClosure->env = newEnv;
                luaC_threadbarrier(L);
            }
            if (newLClosure && newLClosure->l.p)
            {
                taskscheduler::set_proto(newLClosure->l.p, &MaxCapabilities);
            }
            // Anchor the trampoline closure and its environment to prevent GC
            int trampRef = lua_ref(L, -1);
            (void)trampRef;
            // Duplicate env and store a registry ref for it
            lua_pushvalue(L, -2);
            int envRef = lua_ref(L, -1);
            if (newLClosure) s_TrampolineEnvRefs[newLClosure] = envRef;
            // Clean up stack: remove original env left on stack
            if (lua_gettop(L) >= 1 && lua_istable(L, -1)) lua_pop(L, 1);
            lua_ref(L, -1);

            lua_clonefunction(L, 1);
            lua_ref(L, -1);

            hookWhat->l.p = (Proto*)newLClosure->l.p;
            hookWhat->env = (LuaTable*)newLClosure->env;

            for (int i = 0; i < hookWhat->nupvalues; i++) {
                setobj2n(L, &hookWhat->l.uprefs[i], luaO_nilobject);
            }

            for (int i = 0; i < hookWith->nupvalues; i++) {
                setobj2n(L, &hookWhat->l.uprefs[i], &hookWith->l.uprefs[i]);
            }

            return 1;
        }
        else if (hookWhatType == ClosureType::ExecutorFunction && hookWithType == ClosureType::LuauClosure)
        {
            // debug: EX->L hooking
        }
        else
        {
            luaL_error(L, "First: %d, Second: %d", hookWhatType, hookWithType);
        }

        return 0;
    }

    int restorefunction(lua_State* L) {
        if (!lua_isfunction(L, 1)) {
            lua_pushstring(L, "Argument must be a function");
            lua_error(L);
            return 0;
        }

        Closure* Function = clvalue(luaA_toobject(L, 1));
        if (!Function) {
            lua_pushstring(L, "Invalid function argument");
            lua_error(L);
            return 0;
        }

        auto it = original_functions.find(Function);
        if (it == original_functions.end()) {
            lua_pushstring(L, "No original function saved for this function");
            lua_error(L);
            return 0;
        }

        Closure* original = it->second;

        if (Function->isC) {
            Function->nupvalues = original->nupvalues;
            Function->c.f = original->c.f;
            for (int i = 0; i < original->nupvalues; i++) {
                Function->c.upvals[i] = original->c.upvals[i];
            }
        }
        else {
            Function->env = original->env;
            Function->stacksize = original->stacksize;
            Function->preload = original->preload;
            for (int i = 0; i < original->nupvalues; i++) {
                setobj2n(L, &Function->l.uprefs[i], &original->l.uprefs[i]);
            }
            Function->nupvalues = original->nupvalues;
            Function->l.p = original->l.p;
        }

        s_HookedFunctions.erase(Function);
        original_functions.erase(it);

        return 0;
    }

    int getnamecallmethod(lua_State* L)
    {
        const char* namecall = lua_namecallatom(L, nullptr);

        if (!namecall)
        {
            lua_pushnil(L);
            return 1;
        }

        lua_pushstring(L, namecall);
        return 1;
    }

    static int hookmetamethod(lua_State* L)
    {
        luaL_checkany(L, 1);

        const char* MetatableName = luaL_checkstring(L, 2);
        luaL_checktype(L, 3, LUA_TFUNCTION);

        // Fetch metatable
        lua_pushvalue(L, 1);
        if (!lua_getmetatable(L, -1))
            luaL_argerror(L, 1, "object has no metatable");
        if (lua_getfield(L, -1, MetatableName) == LUA_TNIL)
        {
            std::string msg = std::format("'{}' is not a valid member of the given object's metatable.", MetatableName);
            luaL_argerror(L, 2, msg.c_str());
        }
        if (!lua_isfunction(L, -1))
            luaL_argerror(L, 2, "metamethod is not a function");

        // old metamethod at stack top now
        int oldidx = lua_gettop(L);
        lua_setreadonly(L, -2, false);

        // Call hookfunction(old, new) safely
        lua_getglobal(L, "hookfunction");
        lua_pushvalue(L, oldidx);  // old
        lua_pushvalue(L, 3);       // new
        if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            lua_setreadonly(L, -2, true);
            luaL_error(L, "hookfunction failed: %s", err ? err : "?");
        }

        // hookfunction mutates in place and returns original; we don't need to set field.
        // Restore readonly and clean stack: [..., object, metatable, old, original]
        lua_setreadonly(L, -3, true);
        // Return the value returned by hookfunction (original)
        // Remove object, metatable, and old from beneath
        lua_replace(L, 1);   // move returned original to stack index 1
        lua_settop(L, 1);

        return 1;
    }
    int loadstring(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);

        const char* source = lua_tostring(L, 1);
        const char* chunk_name = luaL_optstring(L, 2, "YuB_X");

        const std::string& bytecode = execution::compile(source);

        if (luau_load(L, chunk_name, bytecode.data(), bytecode.size(), 0) != LUA_OK)
        {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        if (Closure* func = lua_toclosure(L, -1))
        {
            if (func->l.p)
                taskscheduler::set_proto(func->l.p, &MaxCapabilities);
        }

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    }

    void initialize(lua_State* L)
    {
        Function(L, "loadstring", 			closures::loadstring);
        Function(L, "clonefunction", 		closures::clonefunction);
        Function(L, "newcclosure", 			closures::newcclosure);
        Function(L, "isnewcclosure", 		closures::isnewcclosure);
        Function(L, "isexecutorclosure", 	closures::isexecutorclosure);
        Function(L, "islclosure", 			closures::islclosure);
        Function(L, "iscclosure", 			closures::iscclosure);
    }
}