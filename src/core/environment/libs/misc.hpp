#pragma once

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <Windows.h>
#include <lstate.h>
#include <lgc.h>

#include <shared.hpp>
#include <lapi.h>
#include <ltable.h>
#include <lgc.h>
#include <lmem.h>
#include <iosfwd>
#include <filesystem>
#include <thread>
#include <string>
#include <fstream>
#include <lualib.h>
#include "lstate.h"
#include "lgc.h"
#include <bitset>

#include <rbx/update/offsets.hpp>
#include <rbx/taskscheduler/scheduler.hpp>
#include <rbx/update/globals.hpp>

__forceinline bool CheckMemory(uintptr_t address) {
    if (address < 0x10000 || address > 0x7FFFFFFFFFFF) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
        return false;
    }

    if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT) {
        return false;
    }

    return true;
}

std::string read_bytecode_misc(uintptr_t addr) {
    auto str = addr + 0x10;
    auto len = *(size_t*)(str + 0x10);
    auto data = *(size_t*)(str + 0x18) > 0xf ? *(uintptr_t*)(str) : str;
    return std::string((char*)(data), len);
}

static void setidentity(lua_State* L, int identity)
{
    auto* ud = reinterpret_cast<RobloxExtraSpace*>(L->userdata);
    ud->Identity = identity;
    ud->Capabilities = ToCapabilities(identity);

    alignas(16) std::int64_t Ignore[128] = { 0 };
    void* table_ptr = *reinterpret_cast<void**>(Structs::IdentityPtr);
    void* src = Roblox::GetIdentityStruct(table_ptr);
    constexpr uint64_t kMask = 0xFFFFFFFFFFFFFF00ull;
    Roblox::Impersonator(Ignore, src, &identity, kMask, 0ull);
}

int getthreadidentity(lua_State* L) {
    auto extra_space = reinterpret_cast<RobloxExtraSpace*>(reinterpret_cast<char*>(L) - sizeof(RobloxExtraSpace));
    lua_pushnumber(L, static_cast<double>(extra_space->Identity));

    return 1;
}
int setthreadidentity(lua_State* L)
{
    luaL_trimstack(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);

    setidentity(L, (int)lua_tonumber(L, 1));

    return 0;
};

int identifyexecutor(lua_State* L)
{
    lua_pushstring(L, exploit::name);
    lua_pushstring(L, exploit::build);
    return 2;
}
int getexecutorname(lua_State* L)
{
    lua_pushstring(L, exploit::name);
    return 1;
}
int getgenv(lua_State* L) {
    lua_pushvalue(L, LUA_ENVIRONINDEX);
    return 1;
}
int getrenv(lua_State* L) {
    lua_State* RobloxState = shared::LocalState;
    LuaTable* clone = luaH_clone(L, RobloxState->gt);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(RobloxState);

    L->top->value.gc = (GCObject*)clone;
    L->top->tt = LUA_TTABLE;
    L->top++;

    lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
    lua_setfield(L, -2, "_G");
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    lua_setfield(L, -2, "shared");
    return 1;
}
int getreg(lua_State* L) {

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
};

int getsenv(lua_State* s) {
    luaL_checktype(s, 1, LUA_TUSERDATA);

    std::string type = luaL_typename(s, 1);
    if (type != "Instance")
        luaL_typeerrorL(s, 1, "Instance");

    const auto script = *reinterpret_cast<uintptr_t*>(lua_touserdata(s, 1));

    lua_getfield(s, 1, "ClassName");
    std::string class_name = std::string(lua_tolstring(s, -1, nullptr));
    lua_pop(s, 1);

    if (class_name == "LocalScript" || class_name == "Script") {
        auto node = *reinterpret_cast<uintptr_t*>(script + 0x1E0);
        auto weakref = *reinterpret_cast<uintptr_t*>(node + 0x8);
        auto liveref = *reinterpret_cast<uintptr_t*>(weakref + 0x10);
        lua_State* liverefthread = *reinterpret_cast<lua_State**>(liveref + 0x70);
        if (!liverefthread) {
            lua_pushnil(s);
            return 1;
        }

        if (liverefthread->global->mainthread != s->global->mainthread)
            luaL_error(s, "thread is on a different vm");

        luaC_threadbarrier(s);
        luaC_threadbarrier(liverefthread);

        lua_pushvalue(liverefthread, LUA_GLOBALSINDEX);
        lua_xmove(liverefthread, s, 1);
        return 1;
    }
    else if (class_name == "ModuleScript") {
        lua_pushvalue(s, LUA_REGISTRYINDEX);
        lua_pushnil(s);
        while (lua_next(s, -2) != 0) {
            if (lua_type(s, -1) == LUA_TTHREAD) {
                lua_State* thread = lua_tothread(s, -1);

                if (thread->userdata && !static_cast<RobloxExtraSpace*>(thread->userdata)->Script.expired()) {
                    if ((uintptr_t)(static_cast<RobloxExtraSpace*>(thread->userdata)->Script.lock().get()) == script) {
                        luaC_threadbarrier(s);
                        luaC_threadbarrier(thread);

                        lua_pushvalue(thread, LUA_GLOBALSINDEX);
                        lua_xmove(thread, s, 1);
                        lua_pop(s, 2);
                        return 1;
                    }
                }
            }
            lua_pop(s, 1);
        }
        lua_pop(s, 1);

        lua_pushnil(s);
        return 1;
    }
    else {
        luaL_argerrorL(s, 1, "Invalid script type");
    }
    return 0;
}
static int gethui(lua_State* L)
{
    lua_getglobal(L, "game");
    lua_getfield(L, -1, "GetService");
    lua_pushvalue(L, -2);
    lua_pushstring(L, "CoreGui");
    if (lua_pcall(L, 2, 1, 0) == LUA_OK && lua_isuserdata(L, -1))
    {
        lua_remove(L, -2);
        return 1;
    }
    lua_settop(L, 0);

    lua_getglobal(L, "game");
    lua_getfield(L, -1, "GetService");
    lua_pushvalue(L, -2);
    lua_pushstring(L, "Players");
    if (lua_pcall(L, 2, 1, 0) != LUA_OK)
    {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, "LocalPlayer");
    if (!lua_isuserdata(L, -1))
    {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, "PlayerGui");
    lua_remove(L, -4);
    lua_remove(L, -3);
    lua_remove(L, -2);
    return 1;
}
int getscriptbytecode(lua_State* L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    void* userdata_block = lua_touserdata(L, 1);
    uintptr_t script_pointer = userdata_block ? *(uintptr_t*)userdata_block : 0;

    if (!script_pointer) {
        lua_pushnil(L);
        return 1;
    }

    lua_getfield(L, 1, "ClassName");
    std::string className = lua_tostring(L, -1);
    lua_pop(L, 1);

    uintptr_t protectedString = 0;
    if (className == "ModuleScript") {
        protectedString = *reinterpret_cast<uintptr_t*>(script_pointer + Offsets::ModuleScriptBytecode);
    }
    else {
        protectedString = *reinterpret_cast<uintptr_t*>(script_pointer + Offsets::LocalScriptBytecode);
    }

    if (!protectedString) {
        lua_pushnil(L);
        return 1;
    }

    auto decompressed = shared::DecompressBytecode(read_bytecode_misc(protectedString));
    if (decompressed.empty()) { lua_pushnil(L); return 1; }
    lua_pushlstring(L, decompressed.data(), decompressed.size());
    return 1;
}
int getrunningscripts(lua_State* L) {

    int threadCount = 0;
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);
    while (lua_next(L, -2) && threadCount < 1000) {
        if (lua_isthread(L, -1)) {
            threadCount++;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    std::unordered_map<uintptr_t, bool> map;
    map.reserve(threadCount > 0 ? threadCount * 2 : 64);

    lua_pushvalue(L, LUA_REGISTRYINDEX);

    lua_newtable(L);

    lua_pushnil(L);

    unsigned int c = 0u;
    while (lua_next(L, -3))
    {
        if (lua_isthread(L, -1))
        {
            const auto thread = lua_tothread(L, -1);

            if (thread)
            {
                if (const auto script_ptr = reinterpret_cast<std::uintptr_t>(thread->userdata) + 0x50; *reinterpret_cast<std::uintptr_t*>(script_ptr))
                {
                    if (map.find(*(uintptr_t*)script_ptr) == map.end())
                    {
                        map.insert({ *(uintptr_t*)script_ptr, true });

                        Roblox::PushInstance(L, *reinterpret_cast<uintptr_t*>(script_ptr));

                        lua_rawseti(L, -4, ++c);
                    }
                }
            }
        }

        lua_pop(L, 1);
    }

    return 1;
}
int getscripts(lua_State* L) {
    struct instancecontext {
        lua_State* L;
        __int64 n;
    };

    instancecontext Context = { L, 0 };

    lua_createtable(L, 0, 0);

    const auto originalGCThreshold = L->global->GCthreshold;
    L->global->GCthreshold = SIZE_MAX;

    luaM_visitgco(L, &Context, [](void* ctx, lua_Page* page, GCObject* gco) -> bool {
        auto context = static_cast<instancecontext*>(ctx);
        lua_State* L = context->L;

        if (isdead(L->global, gco))
            return false;

        if (gco->gch.tt == LUA_TUSERDATA) {
            TValue* top = L->top;
            top->value.gc = gco;
            top->tt = LUA_TUSERDATA;
            L->top++;

            if (strcmp(luaL_typename(L, -1), "Instance") == 0) {
                lua_getfield(L, -1, "ClassName");
                const char* className = lua_tolstring(L, -1, 0);

                if (className && (
                    strcmp(className, "LocalScript") == 0 ||
                    strcmp(className, "ModuleScript") == 0 ||
                    strcmp(className, "Script") == 0))
                {
                    lua_pop(L, 1);

                    lua_getfield(L, -1, "Parent");
                    if (lua_isnil(L, -1)) {
                        lua_pop(L, 1);

                        context->n++;
                        lua_rawseti(L, -2, (int)context->n);
                    }
                    else {
                        lua_pop(L, 2);
                    }
                }
                else {
                    lua_pop(L, 2);
                }
            }
            else {
                lua_pop(L, 1);
            }
        }

        return true;
        });

    L->global->GCthreshold = originalGCThreshold;

    return 1;
}
int getgc(lua_State* L) {
    bool include_tables = luaL_optboolean(L, 1, false);

    lua_newtable(L);

    struct GCContext {
        lua_State* L;
        bool include_tables;
        int i;
    } ctx{ L, include_tables, 0 };

    const auto oldThreshold = L->global->GCthreshold;
    L->global->GCthreshold = SIZE_MAX;

    luaM_visitgco(L, &ctx, [](void* ud, lua_Page* /*page*/, GCObject* gco) -> bool {
        auto* c = static_cast<GCContext*>(ud);
        lua_State* L = c->L;

        if (isdead(L->global, gco))
            return false;

        if (gco->gch.tt == LUA_TFUNCTION || gco->gch.tt == LUA_TUSERDATA ||
            (c->include_tables && gco->gch.tt == LUA_TTABLE))
        {
            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);

            L->top->value.gc = gco;
            L->top->tt = gco->gch.tt;
            L->top++;

            lua_rawseti(L, -2, ++c->i);
        }
        return true;
        });

    L->global->GCthreshold = oldThreshold;

    return 1;
};

namespace misc {
    int getobjects(lua_State* L)
    {
        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "InsertService");
        lua_pcall(L, 2, 1, 0);

        lua_getfield(L, -1, "LoadLocalAsset");
        lua_pushvalue(L, -2);
        lua_pushstring(L, lua_tostring(L, 2));
        lua_pcall(L, 2, 1, 0);

        lua_newtable(L);
        lua_pushvalue(L, -2);
        lua_rawseti(L, -2, 1);
        return 1;
    }
    inline auto fireproximityprompt(lua_State* rl) -> int
    {
        luaL_checktype(rl, 1, LUA_TUSERDATA);

        lua_getglobal(rl, ("typeof"));
        lua_pushvalue(rl, 1);
        lua_pcall(rl, 1, 1, 0);
        const bool isInstance = (strcmp(lua_tolstring(rl, -1, 0), ("Instance")) == 0);
        lua_pop(rl, 1);

        if (!isInstance)
            luaL_argerror(rl, 1, ("Expected Instance"));

        lua_getfield(rl, 1, ("IsA"));
        lua_pushvalue(rl, 1);
        lua_pushstring(rl, ("ProximityPrompt"));
        lua_pcall(rl, 2, 1, 0);
        const bool isExpectedClass = lua_toboolean(rl, -1);
        lua_pop(rl, 1);

        if (!isExpectedClass)
            luaL_argerror(rl, 1, ("Expected an ProximityPrompt"));


        reinterpret_cast<int(__thiscall*)(std::uintptr_t)>(REBASE(0x2686F80))(*reinterpret_cast<std::uintptr_t*>(lua_touserdata(rl, 1)));
        return 0;
    }

    inline __forceinline int fireclickdetector(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        std::string clickOption = lua_isstring(L, 3) ? lua_tostring(L, 3) : "";

        if (strcmp(luaL_typename(L, 1), "Instance") != 0)
        {
            luaL_typeerror(L, 1, "Instance");
            return 0;
        }

        const auto clickdetector = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));

        float distance = 0.0f;
        if (lua_isnumber(L, 2))
            distance = (float)lua_tonumber(L, 2);

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_insert(L, -2);
        lua_pushstring(L, "Players");
        lua_pcall(L, 2, 1, 0);

        lua_getfield(L, -1, "LocalPlayer");
        const auto localPlayer = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, -1));
        lua_pop(L, 2);

        std::transform(clickOption.begin(), clickOption.end(), clickOption.begin(), ::tolower);

        if (clickOption == "rightmouseclick")
            reinterpret_cast<void(*)(__int64, float, __int64)>(REBASE(0x25F1980))((__int64)clickdetector, distance, (__int64)localPlayer);
        else if (clickOption == "mousehoverenter")
            reinterpret_cast<void(*)(__int64, __int64)>(REBASE(0x25F2D80))((__int64)clickdetector, (__int64)localPlayer);
        else if (clickOption == "mousehoverleave")
            reinterpret_cast<void(*)(__int64, __int64)>(REBASE(0x25F2F20))((__int64)clickdetector, (__int64)localPlayer);
        else
            reinterpret_cast<void(*)(__int64, float, __int64)>(REBASE(0x25F17E0))((__int64)clickdetector, distance, (__int64)localPlayer);

        return 0;
    }
    int getloadedmodules(lua_State* L)
    {

        lua_newtable(L);

        typedef struct {
            lua_State* pLua;
            int itemsFound;
            std::map< uintptr_t, bool > map;
        } GCOContext;

        auto gcCtx = GCOContext{ L, 0 };

        const auto ullOldThreshold = L->global->GCthreshold;
        L->global->GCthreshold = SIZE_MAX;

        luaM_visitgco(L, &gcCtx, [](void* ctx, lua_Page* pPage, GCObject* pGcObj) -> bool {
            const auto pCtx = static_cast<GCOContext*>(ctx);
            const auto ctxL = pCtx->pLua;

            if (isdead(ctxL->global, pGcObj))
                return false;

            if (const auto gcObjType = pGcObj->gch.tt;
                gcObjType == LUA_TFUNCTION) {
                ctxL->top->value.gc = pGcObj;
                ctxL->top->tt = gcObjType;
                ctxL->top++;

                lua_getfenv(ctxL, -1);

                if (!lua_isnil(ctxL, -1)) {
                    lua_getfield(ctxL, -1, "script");

                    if (!lua_isnil(ctxL, -1)) {
                        uintptr_t script_addr = *(uintptr_t*)lua_touserdata(ctxL, -1);
                        if (!CheckMemory(script_addr))
                            return false;

                        uintptr_t classDescriptor = *(uintptr_t*)(script_addr + Offsets::ClassDescriptor);
                        if (!CheckMemory(classDescriptor))
                            return false;

                        std::string class_name = **(std::string**)(classDescriptor + Offsets::DescriptorName);

                        if (pCtx->map.find(script_addr) == pCtx->map.end() && class_name == "ModuleScript") {
                            pCtx->map.insert({ script_addr, true });
                            lua_rawseti(ctxL, -4, ++pCtx->itemsFound);
                        }
                        else {
                            lua_pop(ctxL, 1);
                        }
                    }
                    else {
                        lua_pop(ctxL, 1);
                    }
                }

                lua_pop(ctxL, 2);
            }
            return false;
            });

        L->global->GCthreshold = ullOldThreshold;

        return 1;
    }

    void initialize(lua_State* L)
    {
        Function(L, "setthreadidentity", 	setthreadidentity);
        Function(L, "getscriptbytecode", 	getscriptbytecode);
        Function(L, "getthreadidentity", 	getthreadidentity);
        Function(L, "getsenv", 				getsenv);
        Function(L, "getgenv", 				getgenv);
        Function(L, "getrenv", 				getrenv);
        Function(L, "getreg", 				getreg);
        Function(L, "gethui", 				gethui);
        Function(L, "fireproximityprompt", 	fireproximityprompt);
        Function(L, "fireclickdetector", 	fireclickdetector);
        Function(L, "getloadedmodules", 	getloadedmodules);
    }
}