#include <lapi.h>
#include <lualib.h>
#include <shared.hpp>

namespace metatable {
    inline int getrawmetatable(lua_State* LS) {
        luaL_checkany(LS, 1);

        if (!lua_getmetatable(LS, 1))
            lua_pushnil(LS);

        return 1;
    };

    int setreadonly(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        hvalue(luaA_toobject(L, 1))->readonly = lua_toboolean(L, 2);
        return 0;
    }

    int isreadonly(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_pushboolean(L, hvalue(luaA_toobject(L, 1))->readonly);
        return 1;
    }

    void initialize(lua_State* L)
    {

        Function(L, "setreadonly", 			metatable::setreadonly);
        Function(L, "isreadonly", 			metatable::isreadonly);
        Function(L, "getrawmetatable", 		metatable::getrawmetatable);
    }
}

