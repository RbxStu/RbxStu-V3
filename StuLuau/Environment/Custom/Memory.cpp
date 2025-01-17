//
// Created by Dottik on 27/10/2024.
//

#include "Memory.hpp"

#include <set>
#include <unordered_set>
#include <vector>
#include "StuLuau/Extensions/luauext.hpp"
#include "lapi.h"
#include "lgc.h"
#include "lmem.h"
#include "ltable.h"

class GCFilterParameters abstract {
public:
    virtual ~GCFilterParameters() = default;
    virtual bool DoesObjectMeetSpecifications(lua_State *L, GCObject *object) { return false; }
};

class GCTableFilterParameters final : public GCFilterParameters {
    std::vector<lua_TValue *> m_requiredValues;
    std::vector<lua_TValue *> m_requiredKeys;
    std::vector<std::pair<lua_TValue *, lua_TValue *>> m_requiredValuesPairs;
    Table *m_requiredMetatable;

public:
    ~GCTableFilterParameters() override = default;
    GCTableFilterParameters(const std::vector<lua_TValue *> &keys, const std::vector<lua_TValue *> &values,
                            const std::vector<std::pair<lua_TValue *, lua_TValue *>> &valuesPairs, Table *hMetatable) {
        this->m_requiredKeys = keys;
        this->m_requiredValues = values;
        this->m_requiredValuesPairs = valuesPairs;
        this->m_requiredMetatable = hMetatable;
    }

    bool CompareTables(Table *tableOne, Table *tableTwo) {
        return tableOne->sizearray == tableTwo->sizearray && tableOne->node == tableTwo->node;
    }

    bool DoesObjectMeetSpecifications(lua_State *L, GCObject *object) override {
        if (object->gch.tt != ::lua_Type::LUA_TTABLE)
            return false;

        auto hTable = &object->h;

        auto foundAllkeys = true;

        for (const auto key: this->m_requiredKeys) {
            if (luaH_get(hTable, key) == luaO_nilobject) {
                foundAllkeys = false;
                break;
            }
        }

        if (!foundAllkeys)
            return false;

        lua_preparepushcollectable(L, 1);
        L->top->tt = ::lua_Type::LUA_TTABLE;
        L->top->value.gc = object;
        L->top++;

        lua_pushnil(L);

        std::vector<lua_TValue> keys;
        std::vector<lua_TValue> values;

        while (lua_next(L, -2) != 0) {
            auto key = luaA_toobject(L, -2);
            auto value = luaA_toobject(L, -1);
            keys.emplace_back(*key);
            values.emplace_back(*value);

            lua_pop(L, 1);
        }

        // TODO: Complete.

        auto valuesFound = 0;

        for (const auto value: this->m_requiredValues) {
            for (const auto &valueFound: values) {
                if (value->tt != valueFound.tt)
                    continue; // Different types, ain't happening chief.

                auto gcobject_1 = value->value.gc;
                auto gcobject_2 = value->value.gc;

                if (value->tt == ::lua_Type::LUA_TTABLE) {
                    if (CompareTables(&gcobject_1->h, &gcobject_2->h))
                        valuesFound++;
                } else if (value->tt == ::lua_Type::LUA_TFUNCTION) {
                    if (gcobject_1->cl.isC != gcobject_2->cl.isC)
                        continue;

                    if (gcobject_1->cl.isC) {
                        if (gcobject_1->cl.c.f == gcobject_2->cl.c.f)
                            valuesFound++;
                    } else {
                        if (gcobject_1->cl.l.p->sizecode == gcobject_2->cl.l.p->sizecode ||
                            memcmp(gcobject_1->cl.l.p->code, gcobject_2->cl.l.p->code, gcobject_1->cl.l.p->sizecode) ==
                                    0)
                            valuesFound++;
                    }
                } else if (value->tt == ::lua_Type::LUA_TUSERDATA) {
                    // Cannot compare other than by pointer addy.
                    if (gcobject_1->u.data == gcobject_2->u.data)
                        valuesFound++;
                } else if (value->tt == ::lua_Type::LUA_TBUFFER) {
                    if (gcobject_1->buf.len != gcobject_2->buf.len)
                        continue;

                    if (memcmp(gcobject_1->buf.data, gcobject_2->buf.data, gcobject_1->buf.len) == 0)
                        valuesFound++;
                }
            }
        }

        for (const auto key: this->m_requiredKeys) {
            for (const auto &keyFound: keys) {
                if (key->tt != keyFound.tt)
                    continue; // Different types, ain't happening chief.

                auto gcobject_1 = key->value.gc;
                auto gcobject_2 = key->value.gc;

                if (key->tt == ::lua_Type::LUA_TTABLE) {
                    if (CompareTables(&gcobject_1->h, &gcobject_2->h))
                        valuesFound++;
                } else if (key->tt == ::lua_Type::LUA_TFUNCTION) {
                    if (gcobject_1->cl.isC != gcobject_2->cl.isC)
                        continue;

                    if (gcobject_1->cl.isC) {
                        if (gcobject_1->cl.c.f == gcobject_2->cl.c.f)
                            valuesFound++;
                    } else {
                        if (gcobject_1->cl.l.p->sizecode == gcobject_2->cl.l.p->sizecode ||
                            memcmp(gcobject_1->cl.l.p->code, gcobject_2->cl.l.p->code, gcobject_1->cl.l.p->sizecode) ==
                                    0)
                            valuesFound++;
                    }
                } else if (key->tt == ::lua_Type::LUA_TUSERDATA) {
                    // Cannot compare other than by pointer addy.
                    if (gcobject_1->u.data == gcobject_2->u.data)
                        valuesFound++;
                } else if (key->tt == ::lua_Type::LUA_TBUFFER) {
                    if (gcobject_1->buf.len != gcobject_2->buf.len)
                        continue;

                    if (memcmp(gcobject_1->buf.data, gcobject_2->buf.data, gcobject_1->buf.len) == 0)
                        valuesFound++;
                }
            }
        }
    }
};

namespace RbxStu::StuLuau::Environment::Custom {

    int filtergc(lua_State *L) {
        auto str = lua_tostring(L, 1);

        if (strcmp(str, "function") != 0 && strcmp(str, "buffer") != 0 && strcmp(str, "table"))
            luaL_argerror(L, 1, "expected function, buffer or table as the expected type");

        luaL_checktype(L, 2, ::lua_Type::LUA_TTABLE);
        auto returnFirst = luaL_optboolean(L, 3, false);
        // GCFilterParameters params{};

        return 0;
    }

    int Memory::getgc(lua_State *L) {
        const bool addTables = luaL_optboolean(L, 1, false);
        lua_normalisestack(L, 1);
        lua_preparepushcollectable(L, 1);
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
                lua_preparepushcollectable(ctxL, 1);
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
        lua_normalisestack(L, 1);
        if (!iscollectable(luaA_toobject(L, 1)))
            luaL_error(L, "The object must be collectible to be referenced in the Luau Registry.");
        lua_preparepush(L, 1);
        lua_pushinteger(L, lua_ref(L, -1));
        return 1;
    }

    int Memory::unreference_object(lua_State *L) {
        luaL_checkinteger(L, 1);
        lua_normalisestack(L, 1);
        lua_preparepushcollectable(L, 1);
        lua_getref(L, lua_tointeger(L, 1));

        if (lua_type(L, -1) != ::lua_Type::LUA_TNIL)
            lua_unref(L, lua_tointeger(L, 1));

        return 0;
    }

    int Memory::get_gc_threshold(lua_State *L) {
        lua_normalisestack(L, 1);
        lua_preparepushcollectable(L, 1);
        if (!luaL_optboolean(L, 1, false))
            lua_pushnumber(L, static_cast<double>(L->global->GCthreshold));
        else
            lua_pushstring(L, std::format("{}", L->global->GCthreshold).c_str());

        return 1;
    }

    const luaL_Reg *Memory::GetFunctionRegistry() {
        static luaL_Reg memlibreg[] = {
                {"getgc", RbxStu::StuLuau::Environment::Custom::Memory::getgc},

                {"reference_object", RbxStu::StuLuau::Environment::Custom::Memory::reference_object},
                {"unreference_object", RbxStu::StuLuau::Environment::Custom::Memory::unreference_object},
                {"get_gc_threshold", RbxStu::StuLuau::Environment::Custom::Memory::get_gc_threshold},

                {nullptr, nullptr}};

        return memlibreg;
    }

    bool Memory::PushToGlobals() { return true; }

    const char *Memory::GetLibraryName() { return "memory"; }
} // namespace RbxStu::StuLuau::Environment::Custom
