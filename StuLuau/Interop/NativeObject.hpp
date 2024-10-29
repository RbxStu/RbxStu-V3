//
// Created by Dottik on 28/10/2024.
//

#pragma once
#include <optional>
#include <string>
#include <map>

#include <lualib.h>
#include <lua.h>
#include <lapi.h>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::StuLuau::Environment::Interop {
    struct FunctionInformation {
        int argumentCount;
        lua_CFunction fpFunction;
    };

    struct PropertyInformation {
        lua_CFunction fpGetter; // getter will only get in first place the pointer to the object.
        int setterArgc;
        std::optional<lua_CFunction> fpSetter;
    };

    class TaggedIdentifier abstract {
    public:
        virtual std::string GetTagName() {
            return "TaggedIdentifier";
        }
    };

    template<Concepts::TypeConstraint<TaggedIdentifier> TaggerContainer>
    class NativeObject abstract {
    protected:
        std::map<std::string, FunctionInformation> m_functionMap;
        std::map<std::string, PropertyInformation> m_propertyMap;

        static void EnsureTaggedObjectOnStack(lua_State *L) {
            luaL_checktype(L, 1, LUA_TUSERDATA);
            TaggerContainer x{};

            const TValue *pObj = luaA_toobject(L, 1);

            if (strstr(luaT_objtypename(L, pObj), x.GetTagName().c_str()) == nullptr)
                luaL_typeerror(L, 1, std::format("userdata<{}>", x.GetTagName()).c_str());
        }

    public:
        template<Concepts::TypeConstraint<TaggedIdentifier> tagContainer>
        static int __index(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            luaL_checktype(L, 2, LUA_TSTRING);
            tagContainer x{};

            const auto ppNative = *static_cast<NativeObject **>(lua_touserdata(L, 1));

            const auto index = lua_tostring(L, 2);

            if (ppNative->m_functionMap.contains(index)) {
                const auto funcInfo = ppNative->m_functionMap.at(index);
                lua_pushcfunction(L, funcInfo.fpFunction, nullptr);
                return 1;
            } else if (ppNative->m_propertyMap.contains(index)) {
                const auto propInfo = ppNative->m_propertyMap.at(index);
                lua_pushvalue(L, 1);
                lua_remove(L, 2);
                lua_remove(L, 1);
                lua_pushcfunction(L, propInfo.fpGetter, nullptr);
                lua_insert(L, 1);
                lua_call(L, 1, LUA_MULTRET);
                return lua_gettop(L);
            }

            luaL_error(L, std::format( "invalid index into userdata<{}>", x.GetTagName()).c_str());
        }

        template<Concepts::TypeConstraint<TaggedIdentifier> tagContainer>
        static int __namecall(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto ppNative = *static_cast<NativeObject **>(lua_touserdata(L, 1));
            tagContainer x{};

            const auto funcName = std::string_view(L->namecall->data);

            const auto func = std::string(funcName.data());
            if (ppNative->m_functionMap.contains(func)) {
                const auto funcInfo = ppNative->m_functionMap.at(func);
                lua_pushcclosure(L, funcInfo.fpFunction, nullptr, 0);
                lua_insert(L, 1);
                lua_call(L, funcInfo.argumentCount, LUA_MULTRET);
                return lua_gettop(L);
            }

            luaL_error(L, std::format("invalid namecall into userdata<{}>", x.GetTagName()).c_str());
        }
    };
}
