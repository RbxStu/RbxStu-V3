//
// Created by Dottik on 22/11/2024.
//

#include "Debug.hpp"

#include "Luau/Bytecode.h"
#include "StuLuau/Extensions/luauext.hpp"
#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"

namespace RbxStu::StuLuau::Environment::UNC {
    int Debug::getconstants(lua_State *L) {
        luaL_checkany(L, 1);
        lua_normalisestack(L, 1);

        if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1))
            luaL_typeerror(L, 1, "Expected function or number for argument #1");

        if (lua_isnumber(L, 1)) {
            lua_Debug dbgInfo{};

            if (const int num = lua_tointeger(L, 1); !lua_getinfo(L, num, "f", &dbgInfo))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1); // Push copy of stack item.
        }

        if (lua_iscfunction(L, -1))
            luaL_argerror(L, 1, "Lua function expected.");


        const auto *pClosure = lua_toclosure(L, -1);
        const auto constCount = pClosure->l.p->sizek;
        const auto consts = pClosure->l.p->k;

        lua_newtable(L);

        for (int i = 0; i < constCount; i++) {
            const TValue *tval = &(consts[i]);

            if (tval->tt == LUA_TFUNCTION) {
                lua_pushnil(L);
            } else {
                if (iscollectable(tval))
                    luaC_threadbarrier(L);
                L->top->value = tval->value;
                L->top->tt = tval->tt;
                L->top++;
            }

            lua_rawseti(L, -2, i + 1);
        }

        return 1;
    }

    int Debug::getconstant(lua_State *L) {
        luaL_checkany(L, 2);
        lua_normalisestack(L, 2);


        if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1))
            luaL_typeerror(L, 1, "Expected function or number for argument #1");

        const int constantIndex = luaL_checkinteger(L, 2);

        if (lua_isnumber(L, 1)) {
            lua_Debug dbgInfo{};
            if (!lua_getinfo(L, lua_tointeger(L, 1), "f", &dbgInfo))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1); // Push copy of stack item.
        }

        if (lua_iscfunction(L, -1))
            luaL_argerror(L, 1, "Lua function expected.");

        const auto *pClosure = lua_toclosure(L, -1);
        const auto constants = pClosure->l.p->k;

        if (constantIndex < 1)
            luaL_argerror(L, 2, "constant index starts at 1");

        if (constantIndex > pClosure->l.p->sizek)
            luaL_argerror(L, 2, "constant index is out of range");

        if (const auto tValue = &constants[constantIndex - 1]; tValue->tt == LUA_TFUNCTION) {
            lua_pushnil(L); // Closures cannot be obtained.
        } else {
            if (iscollectable(tValue))
                luaC_threadbarrier(L);

            L->top->tt = tValue->tt;
            L->top->value = tValue->value;
            L->top++;
            checkliveness(L->global, tValue);
        }

        return 1;
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    int Debug::setconstant(lua_State *L) {
        luaL_checkany(L, 1);
        luaL_checknumber(L, 2);
        luaL_checkany(L, 3);

        lua_normalisestack(L, 3);

        if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1))
            luaL_typeerror(L, 1, "function or level expected");

        const int index = luaL_checkinteger(L, 2);

        luaL_checkany(L, 3);

        if (lua_isnumber(L, 1)) {
            lua_Debug ar;

            if (!lua_getinfo(L, lua_tointeger(L, 1), "f", &ar))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1);
        }

        if (lua_iscfunction(L, -1))
            luaL_argerror(L, 1, "Lua function expected.");


        const auto cl = lua_toclosure(L, -1);
        const auto *p = cl->l.p;
        auto *k = p->k;

        if (index < 1)
            luaL_argerror(L, 2, "constant index starts at 1");

        if (index > p->sizek)
            luaL_argerror(L, 2, "constant index out of range");

        const auto constant = &k[index - 1];

        if (constant->tt == LUA_TFUNCTION) // brother, we ain't debug.setproto smh
            return 0;

        const TValue *newConstant = luaA_toobject(L, 3);

        if (newConstant->tt != constant->tt)
            luaL_argerror(
                    L, 3,
                    "cannot replace constant when the element you want to replace it with is not of the same type.");

        if (iscollectable(newConstant))
            luaC_threadbarrier(L);
        constant->tt = newConstant->tt;
        constant->value = newConstant->value;

        return 0;
    }

    int Debug::getinfo(lua_State *L) {
        luaL_checkany(L, 1);
        lua_normalisestack(L, 1);
        auto infoLevel = 0;

        if (lua_isnumber(L, 1)) {
            infoLevel = lua_tointeger(L, 1);
            luaL_argcheck(L, infoLevel >= 0, 1, "level cannot be negative");
        } else if (lua_isfunction(L, 1)) {
            infoLevel = -lua_gettop(L);
        } else {
            luaL_argerror(L, 1, "function or level expected");
        }

        lua_Debug lDebug{};

        if (!lua_getinfo(L, infoLevel, "fulasn", &lDebug))
            luaL_argerror(L, 1, "invalid level");

        lua_preparepushcollectable(L, 2);
        lua_newtable(L);

        lua_pushstring(L, lDebug.source);
        lua_setfield(L, -2, "source");

        lua_pushstring(L, lDebug.short_src);
        lua_setfield(L, -2, "short_src");

        lua_pushvalue(L, 1);
        lua_setfield(L, -2, "func");

        lua_pushstring(L, lDebug.what);
        lua_setfield(L, -2, "what");

        lua_pushinteger(L, lDebug.currentline);
        lua_setfield(L, -2, "currentline");

        lua_pushstring(L, lDebug.name);
        lua_setfield(L, -2, "name");

        lua_pushinteger(L, lDebug.nupvals);
        lua_setfield(L, -2, "nups");

        lua_pushinteger(L, lDebug.nparams);
        lua_setfield(L, -2, "numparams");

        lua_pushinteger(L, lDebug.isvararg);
        lua_setfield(L, -2, "is_vararg");

        return 1;
    }

    int Debug::getproto(lua_State *L) {
        luaL_checkany(L, 1);
        luaL_checktype(L, 2, LUA_TNUMBER);
        const bool active = luaL_optboolean(L, 3, true);
        lua_normalisestack(L, 3);
        if (!active)
            luaL_argerror(L, 3, "prototypes cannot be inactive (not implemented)");

        if (lua_isnumber(L, 1) == false && lua_isfunction(L, 1) == false)
            luaL_argerror(L, 1, "function or level expected");

        if (lua_isnumber(L, 1)) {
            const int level = lua_tointeger(L, 1);

            lua_Debug ar;
            if (!lua_getinfo(L, level, "f", &ar))
                luaL_argerror(L, 1, "level out of range");
        } else {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            lua_pushvalue(L, 1);
        }

        if (lua_iscfunction(L, -1))
            luaL_argerrorL(L, 1, "Lua function expected.");

        const auto closure = clvalue(luaA_toobject(L, -1));

        const auto index = lua_tointeger(L, 2);

        if (index < 1 || index > closure->l.p->sizep)
            luaL_argerror(L, 2, "proto index out of range");

        const auto proto = closure->l.p->p[index - 1];

        lua_newtable(L);
        lua_preparepushcollectable(L, 1);
        setclvalue(L, L->top, luaF_newLclosure(L, proto->nups, closure->env, proto));
        L->top++;
        lua_rawseti(L, -2, 1);

        return 1;
    }

    int Debug::getprotos(lua_State *L) {
        luaL_checkany(L, 1);
        lua_normalisestack(L, 1);

        if (lua_isnumber(L, 1) == false && lua_isfunction(L, 1) == false)
            luaL_argerror(L, 1, "function or level expected");

        if (lua_isnumber(L, 1)) {
            const int level = lua_tointeger(L, 1);

            lua_Debug ar;
            if (!lua_getinfo(L, level, "f", &ar))
                luaL_error(L, "level out of range");
        } else {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            lua_pushvalue(L, 1);
        }

        if (lua_iscfunction(L, -1))
            luaL_argerrorL(L, 1, "Lua function expected.");

        const auto *cl = lua_toclosure(L, -1);

        lua_preparepushcollectable(L, 1);
        lua_newtable(L);

        const auto *mProto = cl->l.p;

        for (int i = 0; i < mProto->sizep; i++) {
            Proto *proto = mProto->p[i];
            lua_preparepushcollectable(L, 1);
            Closure *lclosure = luaF_newLclosure(L, proto->nups, cl->env, proto);

            lua_preparepushcollectable(L, 1);
            setclvalue(L, L->top, lclosure);
            L->top++;
            lua_rawseti(L, -2, i + 1);
        }

        return 1;
    }

    int Debug::setstack(lua_State *L) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        luaL_checkany(L, 3);
        lua_normalisestack(L, 3);

        const auto level = lua_tointeger(L, 1);
        const auto index = lua_tointeger(L, 2);

        if (level >= L->ci - L->base_ci || level < 0)
            luaL_argerror(L, 1, "level out of range");

        const auto stackFrame = L->ci - level;
        const auto stackSize = stackFrame->top - stackFrame->base;

        if (clvalue(stackFrame->func)->isC)
            luaL_argerror(L, 1, "Lua function expected.");

        if (index < 1 || index > stackSize)
            luaL_argerror(L, 2, "stack index out of range");

        if (stackFrame->base[index - 1].tt != lua_type(L, 3))
            luaL_argerror(L, 2, "type on the stack is different than that you are trying to set!");

        if (iscollectable(luaA_toobject(L, 3))) {
            lua_preparepushcollectable(L, 1);
        } else {
            lua_preparepush(L, 1);
        }

        setobj2s(L, &stackFrame->base[index - 1], luaA_toobject(L, 3));
        return 0;
    }

    int Debug::getstack(lua_State *L) {
        luaL_checktype(L, 1, LUA_TNUMBER);

        const auto level = lua_tointeger(L, 1);
        const auto index = luaL_optinteger(L, 2, 69420);
        lua_normalisestack(L, 2);

        if (level >= L->ci - L->base_ci || level < 0)
            luaL_argerror(L, 1, "level out of range");

        const auto frame = L->ci - level;
        const std::size_t stackFrameSize = (frame->top - frame->base);

        if (clvalue(frame->func)->isC)
            luaL_argerror(L, 1, "Lua function expected.");

        if (index == 69420) {
            lua_preparepushcollectable(L, 1);
            lua_newtable(L);
            for (int i = 0; i < stackFrameSize; i++) {
                lua_preparepushcollectable(L, 1);
                setobj2s(L, L->top, &frame->base[i]);
                L->top++;

                lua_rawseti(L, -2, i + 1);
            }
        } else {
            if (index < 1 || index > stackFrameSize)
                luaL_argerror(L, 2, "index out of range");

            lua_preparepushcollectable(L, 1);
            setobj2s(L, L->top, &frame->base[index - 1]);
            L->top++;
        }

        return 1;
    }

    int Debug::lsetupvalue(lua_State *L) {
        const int index = luaL_checkinteger(L, 2);
        luaL_checkany(L, 3);
        lua_normalisestack(L, 3);

        if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
            luaL_typeerror(L, 1, "function or level expected");

        if (lua_isnumber(L, 1)) {
            lua_Debug ar;

            if (!lua_getinfo(L, lua_tointeger(L, 1), "f", &ar))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1);
        }

        if (lua_iscfunction(L, -1))
            luaL_argerror(L, 1, "Lua function expected."); // Manipulating C closure upvalues is a really risky thing.

        auto *cl = clvalue(luaA_toobject(L, -1));
        const TValue *value = luaA_toobject(L, 3);
        auto *upvalue_table = cl->l.uprefs;

        if (index < 1)
            luaL_argerror(L, 2, "upvalue index starts at 1");

        if (index > cl->nupvalues)
            luaL_argerror(L, 2, "upvalue index out of range");

        TValue *upvalue = (&upvalue_table[index - 1]);

        if (iscollectable(value)) {
            lua_preparepushcollectable(L, 1);
        } else {
            lua_preparepush(L, 1);
        }

        upvalue->value = value->value;
        upvalue->tt = value->tt;

        luaC_barrier(L, cl, value);
        lua_pushboolean(L, true);

        return 1;
    }

    int Debug::getupvalue(lua_State *L) {
        luaL_checktype(L, 2, LUA_TNUMBER); // index
        lua_normalisestack(L, 2);

        if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
            luaL_typeerror(L, 1, "function or level expected");

        if (lua_isnumber(L, 1)) {
            lua_Debug ar;

            if (!lua_getinfo(L, lua_tointeger(L, 1), "f", &ar))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1);
        }

        const int index = luaL_checkinteger(L, 2);

        const auto *cl = clvalue(luaA_toobject(L, -1));
        const auto *upvalueTable = static_cast<TValue *>(nullptr);

        if (!cl->isC)
            upvalueTable = cl->l.uprefs;
        else if (cl->isC)
            upvalueTable = cl->c.upvals;

        if (!index)
            luaL_argerror(L, 2, "upvalue index starts at 1");

        if (index > cl->nupvalues)
            luaL_argerror(L, 2, "upvalue index is out of range");

        const auto *upval = &upvalueTable[index - 1];
        auto *top = L->top;

        if (iscollectable(upval)) {
            lua_preparepushcollectable(L, 1);
        } else {
            lua_preparepush(L, 1);
        }

        if (upval->tt == ::lua_Type::LUA_TTABLE) {
            lua_pushnil(L);
            return 1;
        }

        top->value = upval->value;
        top->tt = upval->tt;
        L->top++;

        return 1;
    }

    int Debug::getupvalues(lua_State *L) {
        lua_normalisestack(L, 1);

        if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
            luaL_typeerror(L, 1, "function or level expected");

        if (lua_isnumber(L, 1)) {
            lua_Debug ar;

            if (!lua_getinfo(L, lua_tointeger(L, 1), "f", &ar))
                luaL_argerror(L, 1, "level out of range");
        } else {
            lua_pushvalue(L, 1);
        }

        const auto *cl = clvalue(luaA_toobject(L, -1));
        const auto *upvalueTable = static_cast<TValue *>(nullptr);

        lua_newtable(L);

        if (!cl->isC)
            upvalueTable = cl->l.uprefs;
        else if (cl->isC)
            upvalueTable = cl->c.upvals;

        for (int i = 0; i < cl->nupvalues; i++) {
            const auto *upval = (&upvalueTable[i]);
            auto *top = L->top;

            if (iscollectable(upval)) {
                lua_preparepushcollectable(L, 1);
            } else {
                lua_preparepush(L, 1);
            }

            if (upval->tt == ::lua_Type::LUA_TFUNCTION || upval->tt == ::lua_Type::LUA_TTABLE) {
                lua_pushnil(L);
            } else {
                top->value = upval->value;
                top->tt = upval->tt;
                L->top++;
            }

            lua_rawseti(L, -2, (i + 1));
        }

        return 1;
    }

    const luaL_Reg *Debug::GetFunctionRegistry() {
        const static luaL_Reg funcs[] = {{"getconstants", RbxStu::StuLuau::Environment::UNC::Debug::getconstants},
                                         {"getconstant", RbxStu::StuLuau::Environment::UNC::Debug::getconstant},
                                         {"setconstant", RbxStu::StuLuau::Environment::UNC::Debug::setconstant},
                                         {"getinfo", RbxStu::StuLuau::Environment::UNC::Debug::getinfo},
                                         //  {"getproto", RbxStu::StuLuau::Environment::UNC::Debug::getproto},
                                         //  {"getprotos", RbxStu::StuLuau::Environment::UNC::Debug::getprotos},
                                         {"setstack", RbxStu::StuLuau::Environment::UNC::Debug::setstack},
                                         {"getstack", RbxStu::StuLuau::Environment::UNC::Debug::getstack},
                                         {"setupvalue", RbxStu::StuLuau::Environment::UNC::Debug::lsetupvalue},
                                         {"getupvalue", RbxStu::StuLuau::Environment::UNC::Debug::getupvalue},
                                         {"getupvalues", RbxStu::StuLuau::Environment::UNC::Debug::getupvalues},
                                         {nullptr, nullptr}};

        return funcs;
    }

    bool Debug::PushToGlobals() { return false; }

    const char *Debug::GetLibraryName() { return "debug"; }
} // namespace RbxStu::StuLuau::Environment::UNC
