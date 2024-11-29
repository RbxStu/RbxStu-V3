//
// Created by Dottik on 26/11/2024.
//

#include "Miscellaneous.hpp"

#include <Utilities.hpp>

#include "FastFlags.hpp"
#include "Roblox/DataModel.hpp"
#include "StuLuau/Extensions/luauext.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Miscellaneous::isparallel(lua_State *L) {
        /*
         *  Is parallel can be implemented in a myriad of ways, however
         *  the easiest one to implement is by checking the DataModel's flag.
         */
        lua_normalisestack(L, 0);
        lua_getglobal(L, "game");
        Utilities::checkInstance(L, 1, "DataModel");

        const auto pDataModel = RbxStu::Roblox::DataModel::FromPointer(*static_cast<void **>(lua_touserdata(L, 1)));

        lua_pushboolean(L, pDataModel->IsParallel());

        return 1;
    }

    int Miscellaneous::rbxcrash(lua_State *L) {
        std::size_t len{};
        const auto givenKey = luaL_checklstring(L, 1, &len);
        const auto key = RbxStu::FastFlags::SFlagRbxCrashKey.GetValue();

        if (len == key.length() && strcmp(givenKey, key.c_str()) == 0) {
            // ReSharper disable once CppDFANullDereference
            return **static_cast<int **>(nullptr);
        }

        luaL_error(L, "attempted to call RbxStu::RBXCRASH with an invalid crash key; crashing on Luau context instead "
                      "of Native context.");
    }

    const luaL_Reg *Miscellaneous::GetFunctionRegistry() {
        static const luaL_Reg functions[] = {
                {"isparallel", RbxStu::StuLuau::Environment::UNC::Miscellaneous::isparallel},
                {"rbxcrash", RbxStu::StuLuau::Environment::UNC::Miscellaneous::rbxcrash},
                {nullptr, nullptr}};

        return functions;
    }
} // namespace RbxStu::StuLuau::Environment::UNC
