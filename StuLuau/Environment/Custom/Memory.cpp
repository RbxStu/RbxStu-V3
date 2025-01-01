//
// Created by Dottik on 27/10/2024.
//

#include "Memory.hpp"

#include "StuLuau/Extensions/luauext.hpp"
#include "lgc.h"
#include "lmem.h"

namespace RbxStu::StuLuau::Environment::Custom {
    int Memory::getgc(lua_State *L) {
        const bool addTables = luaL_optboolean(L, 1, false);
        lua_normalisestack(L, 1);
        luaC_threadbarrier(L);
        lua_newtable(L);

        typedef struct {
            lua_State *pLua;
            bool accessTables;
            int itemsFound;
        } GCOContext;

        auto gcCtx = GCOContext{L, addTables, 0};

        const auto ullOldThreshold = L->global->GCthreshold;
        L->global->GCthreshold = SIZE_MAX;
        luaM_visitgco(L, &gcCtx, [](void *ctx, lua_Page *pPage, GCObject *pGcObj) -> bool {
            const auto pCtx = static_cast<GCOContext *>(ctx);
            const auto ctxL = pCtx->pLua;

            if (isdead(ctxL->global, pGcObj))
                return false;

            if (const auto gcObjType = pGcObj->gch.tt;
                (gcObjType < LUA_TPROTO && gcObjType >= LUA_TSTRING && gcObjType != LUA_TTABLE) ||
                gcObjType == LUA_TTABLE && pCtx->accessTables) {
                luaC_threadbarrier(ctxL);
                ctxL->top->value.gc = pGcObj;
                ctxL->top->tt = gcObjType;
                ctxL->top++;

                const auto tIndx = pCtx->itemsFound++;
                lua_rawseti(ctxL, -2, tIndx + 1);
            }
            return false;
        });
        L->global->GCthreshold = ullOldThreshold;

        return 1;
    }

    int Memory::reference_object(lua_State *L) {
        luaL_checkany(L, 1);
        lua_pushinteger(L, lua_ref(L, -1));
        return 1;
    }

    int Memory::unreference_object(lua_State *L) {
        luaL_checkinteger(L, 1);
        lua_getref(L, lua_tointeger(L, 1));

        if (lua_type(L, -1) != ::lua_Type::LUA_TNIL)
            lua_unref(L, lua_tointeger(L, 1));

        return 0;
    }

    const luaL_Reg *Memory::GetFunctionRegistry() {
        static luaL_Reg memlibreg[] = {
                {"getgc", RbxStu::StuLuau::Environment::Custom::Memory::getgc},

                {"reference_object", RbxStu::StuLuau::Environment::Custom::Memory::reference_object},
                {"unreference_object", RbxStu::StuLuau::Environment::Custom::Memory::unreference_object},


                {nullptr, nullptr}};

        return memlibreg;
    }

    bool Memory::PushToGlobals() { return true; }

    const char *Memory::GetLibraryName() { return "memory"; }
} // namespace RbxStu::StuLuau::Environment::Custom
