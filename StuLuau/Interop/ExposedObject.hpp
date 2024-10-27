//
// Created by Dottik on 26/10/2024.
//

#pragma once
#include <format>
#include <map>
#include <functional>
#include <string>

#include <lua.h>
#include <memory>

#include "lualib.h"

namespace RbxStu::StuLuau::Interop {
    struct FunctionInformation {
        std::function<int(lua_State *)> fpImplementation;
        std::function<void(lua_State *)> fpCheckStack;
    };

    template<typename T, int dwTag, const char *szTypeName>
    class ExposedObject abstract {
    protected:
        std::map<std::string, RbxStu::StuLuau::Interop::FunctionInformation> m_functionMap;

        ExposedObject(const std::map<std::string, FunctionInformation> &functionMap, const std::string &typeName,
                      int objectTag) {
            this->m_functionMap = functionMap;
        };

        static int __namecall(lua_State *L) {
            auto userdata = reinterpret_cast<std::shared_ptr<T>>(lua_tolightuserdatatagged(L, 1, dwTag));

            if (nullptr == userdata)
                luaL_argerrorL(L, 1, std::format("expected userdata<{}>, got {}.", szTypeName, userdata).c_str());

        }

        static int __index(lua_State *L) {
            auto userdata = reinterpret_cast<std::shared_ptr<T>>(lua_tolightuserdatatagged(L, 1, dwTag));

            if (nullptr == userdata)
                luaL_argerrorL(L, 1, std::format("expected userdata<{}>, got {}.", szTypeName, userdata).c_str());
        }

        static int __tostring(lua_State *L) {
            auto userdata = reinterpret_cast<std::shared_ptr<T>>(lua_tolightuserdatatagged(L, 1, dwTag));

            if (nullptr == userdata)
                luaL_argerrorL(L, 1, std::format("expected userdata<{}>, got {}.", szTypeName, userdata).c_str());

            lua_pushstring(L, szTypeName);

            return 1;
        }

        static int __newindex(lua_State *L) {
            auto userdata = reinterpret_cast<std::shared_ptr<T>>(lua_tolightuserdatatagged(L, 1, dwTag));

            if (nullptr == userdata)
                luaL_argerrorL(L, 1, std::format("expected userdata<{}>, got {}.", szTypeName, userdata).c_str());
        }

    public:
        virtual std::shared_ptr<T> ConstructToLuauObject(lua_State *L) {
            const auto originalTop = lua_gettop(L);
            lua_newtable(L);
            lua_pushlightuserdatatagged(L, std::make_shared<T>(this), dwTag);

            lua_pushcclosure(L, __namecall, nullptr, 0);
            lua_rawsetfield(L, -3, "__namecall");

            lua_pushcclosure(L, __index, nullptr, 0);
            lua_rawsetfield(L, -3, "__index");

            lua_pushcclosure(L, __newindex, nullptr, 0);
            lua_rawsetfield(L, -3, "__newindex");

            lua_pushcclosure(L, __tostring, nullptr, 0);
            lua_rawsetfield(L, -3, "__tostring");


            lua_pushlstring(L, szTypeName, strlen(szTypeName));
            lua_rawsetfield(L, -3, "__type");

            lua_pushvalue(L, -2);
            lua_setmetatable(L, -2);
            lua_settop(L, originalTop);
        }
    };
}
