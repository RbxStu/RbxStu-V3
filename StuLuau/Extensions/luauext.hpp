//
// Created by Dottik on 16/11/2024.
//
#pragma once

#include <lua.h>
#include <lualib.h>
#include <lgc.h>

#define lua_normalisestack(L, maxSize) { if (lua_gettop(L) > maxSize) lua_settop(L, maxSize); }

#define lua_preparepush(L, pushCount) lua_rawcheckstack(L, pushCount)

#define lua_preparepushcollectable(L, pushCount) { lua_preparepush(L, pushCount); luaC_threadbarrier(L); }

#define lua_enforcestack(L, maxSize) { if (lua_gettop(L) > maxSize) luaL_errorL(L, "function expects only %d arguments, you provided %d.", maxSize, lua_gettop(L)); }

#define lua_tomutclosure(L, idx) const_cast<Closure*>(reinterpret_cast<const Closure*>(lua_topointer(L, idx)))

#define lua_toclosure(L, idx) reinterpret_cast<const Closure*>(lua_topointer(L, idx))
