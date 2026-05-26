#pragma once
#pragma once

#include <lstate.h>
#include <string>
#include <queue>
#include <functional>
#include <atomic>
#include "rbx/update/offsets.hpp"
#include <functional>
#include <memory>
#include <lua.h>
#include <thread>
#include <zstd/zstd.h>

#define luaL_trimstack(L, n) if (lua_gettop(L) > n) lua_settop(L, n)
inline uintptr_t MaxCapabilities = 0xFFFFFFFFFFFFFFFF;

inline __int64 __fastcall ToCapabilities(int identity)
{
    __int64 ret;

    switch (identity)
    {
    case 1:
    case 4:
        ret = 0x2000000000000003LL;
        break;
    case 3:
        ret = 0x300000000000000BLL;
        break;
    case 5:
        ret = 0x2000000000000001LL;
        break;
    case 6:
        ret = 0x700000000000000BLL;
        break;
    case 7:
    case 8:
        ret = 0x200000000000003FLL;
        break;
    case 9:
        ret = 12;
        break;
    case 0xA:
        ret = 0x6000000000000003LL;
        break;
    case 0xB:
        ret = 0x2000000000000000LL;
        break;
    case 0xC:
        ret = 0x1000000000000000LL;
        break;
    default:
        ret = 0;
        break;
    }
    return ret | 0xFFFFFFF00;
}
namespace shared
{
    inline uintptr_t c_datamodel;
    inline lua_State* LocalState;
    inline std::vector<std::string> requests;
    inline std::queue<std::function<void()>> YieldQueue;
    inline std::string DecompressBytecode(const std::string& c) {
        uint8_t h[4]; memcpy(h, c.data(), 4);
        for (int i = 0; i < 4; i++) h[i] = (h[i] ^ "RSB1"[i]) - i * 41;
        std::vector<uint8_t> v(c.begin(), c.end());
        for (size_t i = 0; i < v.size(); i++) v[i] ^= h[i % 4] + i * 41;
        int len; memcpy(&len, v.data() + 4, 4);
        std::string out(len, 0);
        return ZSTD_decompress(out.data(), len, v.data() + 8, v.size() - 8) == len ? out : "";
    }

    inline std::string ReadBytecode(uintptr_t addr) {
        uintptr_t str = addr + 0x10;
        size_t len = *(size_t*)(str + 0x10);
        size_t cap = *(size_t*)(str + 0x18);
        uintptr_t data_ptr = (cap > 0x0f) ? *(uintptr_t*)(str + 0x00) : str;
        return std::string(reinterpret_cast<const char*>(data_ptr), len);
    }
}
namespace helper
{
    inline bool check_game(uintptr_t DataModel)
    {
        uintptr_t GameLoaded = *reinterpret_cast<uintptr_t*>(DataModel + 0x5F0);
        if (GameLoaded != 31)
            return false;

        return true;
    }
}
inline void Function(lua_State* L, const char* Name, lua_CFunction Function)
{
    lua_pushcclosure(L, Function, nullptr, 0);
    lua_setglobal(L, Name);
}

inline void TableFunction(lua_State* L, const char* Name, lua_CFunction Function)
{
    lua_pushcclosure(L, Function, nullptr, 0);
    lua_setfield(L, -2, Name);
}
