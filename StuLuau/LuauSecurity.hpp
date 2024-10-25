//
// Created by Dottik on 24/10/2024.
//

#pragma once
#include <memory>

#include "ExecutionEngine.hpp"
#include "lstate.h"
#include "Luau/TypeFunctionRuntime.h"

namespace RbxStu::StuLuau {
    class LuauSecurity final {
        static std::shared_ptr<RbxStu::StuLuau::LuauSecurity> pInstance;

    public:
        static std::shared_ptr<RbxStu::StuLuau::LuauSecurity> GetSingleton();

        static RBX::ExtraSpace *GetThreadExtraspace(lua_State *L);

        std::int64_t ToCapabilitiesFlags(RbxStu::StuLuau::ExecutionSecurity executionSecurity);

        void SetThreadSecurity(lua_State *L, RbxStu::StuLuau::ExecutionSecurity executionSecurity, bool markThread);

        void MarkThread(lua_State *L);

        bool IsMarkedThread(lua_State *L);

        bool IsOurThread(lua_State *L);
    };
} // RbxStu::StuLuau
