#include <Windows.h>
#include <lstate.h>
#include <lgc.h>

#include <shared.hpp>
#include <execution/Execution.hpp>
#include <rbx/taskscheduler/scheduler.hpp>


namespace cache {
    namespace HelpFuncs {
        static void IsInstance(lua_State* L, int idx) {
            std::string typeoff = luaL_typename(L, idx);
            if (typeoff != "Instance")
                luaL_typeerrorL(L, 1, "Instance");
        };
    }

    int invalidate(lua_State* L) {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        if (strcmp(luaL_typename(L, 1), "Instance") != LUA_OK)
        {
            luaL_typeerror(L, 1, "Instance");
            return 0;
        }

        const auto Instance = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, 1));
        lua_pop(L, lua_gettop(L));

        lua_pushlightuserdata(L, (void*)Roblox::PushInstance);
        lua_gettable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, (void*)Instance);
        lua_pushnil(L);
        lua_settable(L, -3);

        return 0;
    }

    int replace(lua_State* LS) {
        luaL_checktype(LS, 1, LUA_TUSERDATA);
        luaL_checktype(LS, 2, LUA_TUSERDATA);

        HelpFuncs::IsInstance(LS, 1);
        HelpFuncs::IsInstance(LS, 2);

        const auto Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));

        lua_pushlightuserdata(LS, (void*)Roblox::PushInstance);
        lua_gettable(LS, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(LS, (void*)Instance);
        lua_pushvalue(LS, 2);
        lua_settable(LS, -3);
        return 0;
    };

    int iscached(lua_State* LS) {
        luaL_checktype(LS, 1, LUA_TUSERDATA);
        HelpFuncs::IsInstance(LS, 1);
        const auto Instance = *static_cast<void**>(lua_touserdata(LS, 1));

        lua_pushlightuserdata(LS, (void*)Roblox::PushInstance);
        lua_gettable(LS, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(LS, Instance);
        lua_gettable(LS, -2);

        lua_pushboolean(LS, !lua_isnil(LS, -1));
        return 1;
    };

    int cloneref(lua_State* LS) {
        luaL_checktype(LS, 1, LUA_TUSERDATA);
        HelpFuncs::IsInstance(LS, 1);
        const auto OldUserdata = lua_touserdata(LS, 1);
        const auto NewUserdata = *reinterpret_cast<uintptr_t*>(OldUserdata);

        lua_pushlightuserdata(LS, (void*)Roblox::PushInstance);

        lua_rawget(LS, -10000);
        lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
        lua_rawget(LS, -2);

        lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
        lua_pushnil(LS);
        lua_rawset(LS, -4);

        Roblox::PushInstance(LS, (uintptr_t)OldUserdata);

        lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
        lua_pushvalue(LS, -3);
        lua_rawset(LS, -5);

        return 1;
    };

    int compareinstances(lua_State* LS) {
        luaL_checktype(LS, 1, LUA_TUSERDATA);
        luaL_checktype(LS, 2, LUA_TUSERDATA);

        HelpFuncs::IsInstance(LS, 1);
        HelpFuncs::IsInstance(LS, 2);

        uintptr_t First = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));
        if (!First)
            luaL_argerrorL(LS, 1, "Invalid instance");

        uintptr_t Second = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 2));
        if (!Second)
            luaL_argerrorL(LS, 2, "Invalid instance");

        if (First == Second)
            lua_pushboolean(LS, true);
        else
            lua_pushboolean(LS, false);

        return 1;
    };
    void initialize(lua_State* L)
    {
        Function(L, "cloneref", 			cache::cloneref);
        Function(L, "compareinstances", 	cache::compareinstances);

        lua_newtable(L);
        TableFunction(L, "invalidate", 		cache::invalidate);
        TableFunction(L, "iscached", 		cache::iscached);
        TableFunction(L, "replace", 		cache::replace);
        lua_setglobal(L, "cache");
    }
};

