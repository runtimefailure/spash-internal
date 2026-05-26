#include <lz4/include/lz4.h>
#include <Windows.h>
#include <lstate.h>
#include <lgc.h>
#include <lualib.h>
#include <sstream>
#include <shared.hpp>

namespace lz4
{
    int lz4compress(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        size_t len{};
        const char* source = lua_tolstring(L, 1, &len);

        const int max_compressed_sz = LZ4_compressBound(static_cast<int>(len));

        const auto buffer = new char[max_compressed_sz];
        memset(buffer, 0, max_compressed_sz);

        const auto actual_sz = LZ4_compress_default(source, buffer, static_cast<int>(len), max_compressed_sz);

        lua_pushlstring(L, buffer, actual_sz);
        return 1;
    }

    int lz4decompress(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TNUMBER);

        size_t len{};
        const char* source = lua_tolstring(L, 1, &len);
        int data_sz = lua_tointeger(L, 2);

        char* buffer = new char[data_sz];

        memset(buffer, 0, data_sz);

        LZ4_decompress_safe(source, buffer, static_cast<int>(len), data_sz);

        lua_pushlstring(L, buffer, data_sz);
        return 1;
    }

    void initialize(lua_State* L)
    {
        Function(L, "lz4compress", lz4compress);
        Function(L, "lz4decompress", lz4decompress);
    }
}