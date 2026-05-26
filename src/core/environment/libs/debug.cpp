#include "../environment.hpp"
#include "lstate.h"
#include "lualib.h"
#include "lapi.h"
#include "ldo.h"
#include "lgc.h"
#include "lfunc.h"
#include "lmem.h"
#include "lstring.h"
#include "../../Execution/Execution.hpp"
Closure* header_get_function(lua_State* L, bool allowCclosure = false, bool popcl = true)
{
    luaL_checkany(L, 1);

    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
        luaL_argerror(L, 1, "function or number");

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);
        if (level <= 0)
            luaL_argerror(L, 1, "level out of range");
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
        luaL_argerror(L, 1, "invalid level");

    if (!lua_isfunction(L, -1))
        luaL_argerror(L, 1, "level does not point to a function");

    if (lua_iscfunction(L, -1) && !allowCclosure)
        luaL_argerror(L, 1, "luau function expected");

    auto function = clvalue(luaA_toobject(L, -1));
    if (popcl) lua_pop(L, 1);

    return function;
}

int debug_getupvalues(lua_State* L)
{
    Closure* function = header_get_function(L, true, false);
    lua_newtable(L);

    for (int i = 0; i < function->nupvalues; i++) {
        TValue* upval;
        const char* upvalue_name = aux_upvalue_2(function, i + 1, &upval);

        if (upvalue_name) {
            if (iscollectable(upval))
                luaC_threadbarrier(L);

            lua_rawcheckstack(L, 1);
            L->top->value = upval->value;
            L->top->tt = upval->tt;
            L->top++;
            lua_rawseti(L, -2, i + 1);
        }
    }

    return 1;
}

int debug_getupvalue(lua_State* L)
{
    const auto Func = header_get_function(L, true, false);
    const auto idx = lua_tointeger(L, 2);
    int level = -lua_gettop(L);

    if (Func->nupvalues <= 0)
        luaL_argerror(L, 1, "function has no upvalues");

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
        luaL_error(L, "invalid level");

    const char* upvalue = lua_getupvalue(L, -1, idx);
    if (!upvalue) {
        lua_pushnil(L);
        return 1;
    }

    return 1;
}

int debug_setupvalue(lua_State* L)
{
    const auto Func = header_get_function(L, false, false);
    const auto idx = lua_tointeger(L, 2);

    if (Func->nupvalues <= 0)
        luaL_argerror(L, 1, "function has no upvalues");

    if (!(idx >= 1 && idx <= Func->nupvalues))
        luaL_argerror(L, 2, "index out of range");

    lua_pushvalue(L, 3);
    lua_setupvalue(L, -2, idx);
    return 1;
}

int debug_getconstants(lua_State* L)
{
    const auto Func = header_get_function(L);
    const auto p = (Proto*)Func->l.p;

    lua_createtable(L, p->sizek, 0);

    for (int i = 0; i < p->sizek; i++)
    {
        TValue k = p->k[i];

        if (k.tt == LUA_TNIL || k.tt == LUA_TFUNCTION || k.tt == LUA_TTABLE)
        {
            lua_pushnil(L);
        }
        else
        {
            luaC_threadbarrier(L);
            luaA_pushobject(L, &k);
        }
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int debug_getconstant(lua_State* L)
{
    const auto Func = header_get_function(L);
    const auto idx = luaL_checkinteger(L, 2);
    const auto p = (Proto*)Func->l.p;
    const auto level = -lua_gettop(L);

    if (p->sizek <= 0)
        luaL_argerror(L, 1, "function has no constants");

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
        luaL_error(L, "invalid level");

    if (!(idx >= 1 && idx <= p->sizek)) 
        luaL_argerror(L, 2, "index out of range");

    const auto k = &p->k[idx - 1];

    if (k->tt == LUA_TNIL || k->tt == LUA_TTABLE || k->tt == LUA_TFUNCTION) {
        lua_pushnil(L);
        return 1;
    }

    luaA_pushobject(L, k);
    return 1;
}

int debug_setconstant(lua_State* L)
{
    luaL_checkany(L, 3);

    const auto Func = header_get_function(L);
    const auto idx = luaL_checkinteger(L, 2);
    const auto p = (Proto*)Func->l.p;

    if (p->sizek <= 0)
        luaL_argerror(L, 1, "function has no constants");

    if (!(idx >= 1 && idx <= p->sizek))
        luaL_argerror(L, 2, "index out of range");

    TValue* k = &p->k[idx - 1];

    if (k->tt == LUA_TFUNCTION || k->tt == LUA_TTABLE)
        return 0;

    if (k->tt == luaA_toobject(L, 3)->tt)
        setobj2s(L, k, luaA_toobject(L, 3));

    return 0;
}

int debug_getprotos(lua_State* L)
{
    Closure* Func = header_get_function(L);
    bool active = !lua_isnoneornil(L, 2) ? (lua_toboolean(L, 2) != 0) : false;

    Proto* p = (Proto*)Func->l.p;
    lua_createtable(L, p->sizep, 0);

    if (!active)
    {
        for (int i = 0; i < p->sizep; i++)
        {
            Proto* proto = p->p[i];
            lua_pushlightuserdata(L, (void*)proto);
            lua_rawseti(L, -2, i + 1);
        }
    }
    else
    {
        for (int i = 0; i < p->sizep; i++)
        {
            Proto* proto = p->p[i];
            Closure* pcl = luaF_newLclosure(L, Func->nupvalues, Func->env, proto);
            luaC_threadbarrier(L);
            setclvalue(L, L->top, pcl);
            L->top++;
            lua_rawseti(L, -2, i + 1);
        }
    }
    return 1;
}

Proto* clone_proto(lua_State* L, Proto* proto)
{
    Proto* clone = luaF_newproto(L);

    clone->sizek = proto->sizek;
    clone->k = luaM_newarray(L, proto->sizek, TValue, proto->memcat);
    for (int i = 0; i < proto->sizek; ++i)
        setobj2n(L, &clone->k[i], &proto->k[i]);

    uint8_t* lineinfo_ptr = proto->lineinfo.Get();
    if (lineinfo_ptr) {
        clone->lineinfo.Set(luaM_newarray(L, proto->sizelineinfo, uint8_t, proto->memcat));
        memcpy(clone->lineinfo.Get(), lineinfo_ptr, proto->sizelineinfo);
    }
    LocVar* locvars_ptr = proto->locvars.Get();
    clone->locvars.Set(luaM_newarray(L, proto->sizelocvars, LocVar, proto->memcat));
    for (int i = 0; i < proto->sizelocvars; ++i)
    {
        const auto varname = getstr(locvars_ptr[i].varname);
        const auto varname_size = strlen(varname);

        clone->locvars.Get()[i].varname = luaS_newlstr(L, varname, varname_size);
        clone->locvars.Get()[i].endpc = locvars_ptr[i].endpc;
        clone->locvars.Get()[i].reg = locvars_ptr[i].reg;
        clone->locvars.Get()[i].startpc = locvars_ptr[i].startpc;
    }

    clone->nups = proto->nups;
    clone->sizeupvalues = proto->sizeupvalues;
    clone->sizelineinfo = proto->sizelineinfo;
    clone->linegaplog2 = proto->linegaplog2;
    clone->sizelocvars = proto->sizelocvars;
    clone->linedefined = proto->linedefined;

    TString* debugname_ptr = proto->debugname.Get();
    if (debugname_ptr)
    {
        const auto debugname = getstr(debugname_ptr);
        const auto debugname_size = strlen(debugname);

        clone->debugname.Set(luaS_newlstr(L, debugname, debugname_size));
    }

    TString* source_ptr = proto->source.Get();
    if (source_ptr)
    {
        const auto source = getstr(source_ptr);
        const auto source_size = strlen(source);

        clone->source.Set(luaS_newlstr(L, source, source_size));
    }

    clone->numparams = proto->numparams;
    clone->is_vararg = proto->is_vararg;
    clone->maxstacksize = proto->maxstacksize;
    clone->bytecodeid = proto->bytecodeid;

    auto bytecode = RBX::Execution->CompileScript("return");
    luau_load(L, "@cloneproto", bytecode.c_str(), bytecode.size(), 0);

    Closure* cl = clvalue(index2addr(L, -1));

    clone->sizecode = cl->l.p->sizecode;
    clone->code = luaM_newarray(L, clone->sizecode, Instruction, proto->memcat);
    for (size_t i = 0; i < cl->l.p->sizecode; i++) {
        clone->code[i] = cl->l.p->code[i];
    }
    lua_pop(L, 1);
    clone->codeentry = clone->code;
    clone->debuginsn = 0;

    clone->sizep = proto->sizep;
    clone->p = luaM_newarray(L, proto->sizep, Proto*, proto->memcat);
    for (int i = 0; i < proto->sizep; ++i)
    {
        clone->p[i] = clone_proto(L, proto->p[i]);
    }

    return clone;
}

int debug_getproto(lua_State* L)
{
    luaL_checktype(L, 2, LUA_TNUMBER);

    Closure* closure = nullptr;
    int index = lua_tointeger(L, 2);
    bool active = luaL_optboolean(L, 3, false);

    if (lua_isfunction(L, 1))
    {
        if (lua_iscfunction(L, 1))
            luaL_argerror(L, 1, "lua function expected");

        const TValue* obj = luaA_toobject(L, 1);

        if (ttype(obj) != LUA_TFUNCTION)
            luaL_argerror(L, 1, "function expected");

        closure = clvalue(obj);

        if (!closure || closure->isC)
            luaL_argerror(L, 1, "lua closure expected");
    }
    else if (lua_isnumber(L, 1))
    {
        lua_Debug dbg{};
        int level = lua_tointeger(L, 1);
        int callstack_size = static_cast<int>(L->ci - L->base_ci);

        if (level <= 0 || level >= callstack_size)
            luaL_argerror(L, 1, "level out of bounds");

        if (!lua_getinfo(L, level, "f", &dbg))
            luaL_argerror(L, 1, "level out of bounds");

        if (!lua_isfunction(L, -1))
            luaL_argerror(L, 1, "level does not point to a function");

        if (lua_iscfunction(L, -1))
            luaL_argerror(L, 1, "lua function expected");

        const TValue* obj = luaA_toobject(L, -1);

        if (ttype(obj) != LUA_TFUNCTION)
            luaL_argerror(L, 1, "function expected");

        closure = clvalue(obj);

        if (!closure || closure->isC)
            luaL_argerror(L, 1, "lua closure expected");

        lua_pop(L, 1);
    }

    if (!closure)
        luaL_argerror(L, 1, "function or level expected");

    Proto* p = closure->l.p;

    if (!p)
        luaL_argerror(L, 1, "closure has no proto");

    if (!p->p)
        luaL_argerror(L, 1, "proto has no child protos");

    if (index < 1 || index >(int)p->sizep)
        luaL_argerror(L, 2, "index out of bounds");

    Proto* wanted_proto = p->p[index - 1];

    if (!wanted_proto)
        luaL_argerror(L, 2, "proto is null");

    if (!active)
    {
        Proto* cloned_proto = clone_proto(L, wanted_proto);

        Closure* new_closure = luaF_newLclosure(
            L,
            closure->nupvalues,
            L->gt,
            cloned_proto
        );

        luaC_checkGC(L);
        luaC_threadbarrier(L);

        setclvalue(L, L->top, new_closure);
        luaD_checkstack(L, 1);
        L->top++;

        return 1;
    }

    const global_State* g = L->global;
    int object_count = 0;

    lua_newtable(L);

    lua_gc(L, LUA_GCSTOP, 0);

    for (lua_Page* page = g->allgcopages; page;)
    {
        lua_Page* next = page->listnext;

        char* start;
        char* end;
        int busy_blocks;
        int block_size;

        luaM_getpagewalkinfo(page, &start, &end, &busy_blocks, &block_size);

        for (char* pos = start; pos != end; pos += block_size)
        {
            GCObject* obj = reinterpret_cast<GCObject*>(pos);

            if (obj->gch.tt != LUA_TFUNCTION)
                continue;

            if (isdead(g, obj))
                continue;

            Closure* gc_closure = reinterpret_cast<Closure*>(obj);

            if (gc_closure->isC)
                continue;

            if (!gc_closure->l.p)
                continue;

            if (gc_closure->l.p != wanted_proto)
                continue;

            luaC_checkGC(L);
            luaC_threadbarrier(L);

            setclvalue(L, L->top, gc_closure);
            luaD_checkstack(L, 1);
            L->top++;

            lua_rawseti(L, -2, ++object_count);
        }

        page = next;
    }

    lua_gc(L, LUA_GCRESTART, 0);

    return 1;

}

int debug_getstack(lua_State* L)
{
    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
        luaL_argerror(L, 1, "function or number");

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);
        if (level <= 0)
            luaL_argerror(L, 1, "level out of range");
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
        luaL_argerror(L, 1, "invalid level");

    if (!lua_isfunction(L, -1))
        luaL_argerror(L, 1, "level does not point to a function");

    if (lua_iscfunction(L, -1))
        luaL_argerror(L, 1, "luau function expected");

    lua_pop(L, 1);

    auto ci = L->ci[-level];

    if (lua_isnumber(L, 2))
    {
        const auto idx = lua_tointeger(L, 2) - 1;

        if (idx >= cast_int(ci.top - ci.base) || idx < 0)
            luaL_argerror(L, 2, "index out of range");

        auto val = ci.base + idx;
        luaC_threadbarrier(L);
        luaA_pushobject(L, val);
    }
    else
    {
        int idx = 0;
        lua_newtable(L);

        for (auto val = ci.base; val < ci.top; val++)
        {
            lua_pushinteger(L, idx++ + 1);
            luaC_threadbarrier(L);
            luaA_pushobject(L, val);
            lua_settable(L, -3);
        }
    }

    return 1;
}

int debug_setstack(lua_State* L)
{
    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
        luaL_argerror(L, 1, "function or number");

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);
        if (level <= 0)
            luaL_argerror(L, 1, "level out of range");
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
        luaL_argerror(L, 1, "invalid level");

    if (!lua_isfunction(L, -1))
        luaL_argerror(L, 1, "level does not point to a function");

    if (lua_iscfunction(L, -1))
        luaL_argerror(L, 1, "luau function expected");

    lua_pop(L, 1);
    luaL_checkany(L, 3);

    auto ci = L->ci[-level];
    const auto idx = luaL_checkinteger(L, 2) - 1;

    if (idx >= cast_int(ci.top - ci.base) || idx < 0)
        luaL_argerror(L, 2, "index out of range");

    if ((ci.base + idx)->tt != luaA_toobject(L, 3)->tt)
        luaL_argerror(L, 3, "new value type does not match previous value type");

    setobj2s(L, (ci.base + idx), luaA_toobject(L, 3));
    return 0;
}


int debug_getinfo(lua_State* L)
{
    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
        luaL_argerror(L, 1, "function or number");

    int level;
    if (lua_isnumber(L, 1))
        level = lua_tointeger(L, 1);
    else
        level = -lua_gettop(L);

    auto desc = "sluanf";

    lua_Debug ar;
    if (!lua_getinfo(L, level, desc, &ar))
        luaL_argerror(L, 1, "invalid level");

    if (!lua_isfunction(L, -1))
        luaL_argerror(L, 1, "level does not point to a function.");

    lua_newtable(L);
    {
        if (std::strchr(desc, 's'))
        {
            lua_pushstring(L, ar.source);
            lua_setfield(L, -2, "source");

            lua_pushstring(L, ar.short_src);
            lua_setfield(L, -2, "short_src");

            lua_pushstring(L, ar.what);
            lua_setfield(L, -2, "what");

            lua_pushinteger(L, ar.linedefined);
            lua_setfield(L, -2, "linedefined");
        }

        if (std::strchr(desc, 'l'))
        {
            lua_pushinteger(L, ar.currentline);
            lua_setfield(L, -2, "currentline");
        }

        if (std::strchr(desc, 'u'))
        {
            lua_pushinteger(L, ar.nupvals);
            lua_setfield(L, -2, "nups");
        }

        if (std::strchr(desc, 'a'))
        {
            lua_pushinteger(L, ar.isvararg);
            lua_setfield(L, -2, "is_vararg");

            lua_pushinteger(L, ar.nparams);
            lua_setfield(L, -2, "numparams");
        }

        if (std::strchr(desc, 'n'))
        {
            lua_pushstring(L, ar.name);
            lua_setfield(L, -2, "name");
        }

        if (std::strchr(desc, 'f'))
        {
            lua_pushvalue(L, -2);
            lua_remove(L, -3);
            lua_setfield(L, -2, "func");
        }
    }

    return 1;
}
int debug_setinfo(lua_State* L)
{
    luaL_checktype(L, 2, LUA_TTABLE);

    Closure* cl = header_get_function(L, true, true);

    bool isLua = !cl->isC;
    Proto* proto = isLua ? cl->l.p : nullptr;

    lua_pushnil(L);
    while (lua_next(L, 2) != 0)
    {
        const char* key = lua_tostring(L, -2);

        if (!key) {
            lua_pop(L, 1);
            continue;
        }

        if (!strcmp(key, "name"))
        {
            const char* val = lua_tostring(L, -1);
            if (val)
            {
                TString* ts = luaS_new(L, val);

                if (proto)
                    proto->debugname.Set(ts);
                else
                    cl->c.debugname.Set(val);
            }
        }
        else if (proto)
        {
            if (!strcmp(key, "short_src") || !strcmp(key, "source"))
            {
                const char* val = lua_tostring(L, -1);
                if (val)
                {
                    TString* ts = luaS_new(L, val);
                    proto->source.Set(ts);
                }
            }
            else if (!strcmp(key, "currentline") || !strcmp(key, "linedefined"))
            {
                if (lua_isnumber(L, -1))
                    proto->linedefined = (int)lua_tointeger(L, -1);
            }
        }

        lua_pop(L, 1);
    }

    return 0;
}

int debug_getregistry(lua_State* L)
{
    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

int debug_getmetatable(lua_State* L)
{
    luaL_checkany(L, 1);

    if (!lua_getmetatable(L, 1))
        lua_pushnil(L);

    return 1;
}

int debug_setmetatable(lua_State* L)
{
    luaL_checkany(L, 1);

    int t = lua_type(L, 2);
    if (t != LUA_TNIL && t != LUA_TTABLE)
        luaL_typeerror(L, 2, "nil or table expected");

    lua_pushvalue(L, 2);
    lua_setmetatable(L, 1);
    return 1;
}
void Executor::CEnvironment::DebugLib(lua_State* L)
{
    lua_newtable(L);

    int originalRobloxStateStack = lua_gettop(LuauThreads::rL);

    lua_getglobal(LuauThreads::rL, "debug");

    if (!lua_istable(LuauThreads::rL, -1)) {
        lua_settop(LuauThreads::rL, originalRobloxStateStack);
        return;
    }

    lua_pushnil(LuauThreads::rL);

    while (lua_next(LuauThreads::rL, -2) != 0) {
        if (lua_isfunction(LuauThreads::rL, -1)) {
            const char* key = lua_tostring(LuauThreads::rL, -2);

            lua_xmove(LuauThreads::rL, L, 1);
            lua_setfield(L, -2, key);
        }
    }

    lua_settop(LuauThreads::rL, originalRobloxStateStack);
    Environment->make_member(L, OBF("getupvalues"), debug_getupvalues);
    Environment->make_member(L, OBF("getupvalue"), debug_getupvalue);
    Environment->make_member(L, OBF("setupvalue"), debug_setupvalue);

    Environment->make_member(L, OBF("getconstants"), debug_getconstants);
    Environment->make_member(L, OBF("getconstant"), debug_getconstant);
    Environment->make_member(L, OBF("setconstant"), debug_setconstant);

    Environment->make_member(L, OBF("getproto"), debug_getproto);
    Environment->make_member(L, OBF("getprotos"), debug_getprotos);

    Environment->make_member(L, OBF("getstack"), debug_getstack);
    Environment->make_member(L, OBF("setstack"), debug_setstack);

    Environment->make_member(L, OBF("getinfo"), debug_getinfo);
    Environment->make_member(L, OBF("setinfo"), debug_setinfo);

    Environment->make_member(L, OBF("getregistry"), debug_getregistry);
    Environment->make_member(L, OBF("getmetatable"), debug_getmetatable);
    Environment->make_member(L, OBF("setmetatable"), debug_setmetatable);

    lua_setglobal(L, OBF("debug"));

    Environment->make_global(L, OBF("getupvalues"), debug_getupvalues);
    Environment->make_global(L, OBF("getupvalue"), debug_getupvalue);
    Environment->make_global(L, OBF("setupvalue"), debug_setupvalue);

    Environment->make_global(L, OBF("getconstants"), debug_getconstants);
    Environment->make_global(L, OBF("getconstant"), debug_getconstant);
    Environment->make_global(L, OBF("setconstant"), debug_setconstant);

    Environment->make_global(L, OBF("getproto"), debug_getproto);
    Environment->make_global(L, OBF("getprotos"), debug_getprotos);

    Environment->make_global(L, OBF("getstack"), debug_getstack);
    Environment->make_global(L, OBF("setstack"), debug_setstack);

    Environment->make_global(L, OBF("getinfo"), debug_getinfo);
    Environment->make_global(L, OBF("setinfo"), debug_setinfo);
}
