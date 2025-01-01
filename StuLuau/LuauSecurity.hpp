//
// Created by Dottik on 24/10/2024.
//

#pragma once
#include <memory>

#include "ExecutionEngine.hpp"
#include "lstate.h"
#include "Luau/TypeFunctionRuntime.h"

#define GetThreadExtraspace(L) static_cast<RBX::ExtraSpace *>(L->userdata)

namespace RbxStu::StuLuau {
    class LuauSecurity final {
        static std::shared_ptr<RbxStu::StuLuau::LuauSecurity> pInstance;

    public:
        static std::shared_ptr<RbxStu::StuLuau::LuauSecurity> GetSingleton();

        std::int64_t ToCapabilitiesFlags(RbxStu::StuLuau::ExecutionSecurity executionSecurity);

        RBX::Security::Permissions GetIdentityFromExecutionSecurity(ExecutionSecurity executionSecurity);

        ExecutionSecurity GetExecutionSecurityFromIdentity(int32_t identity);

        void SetThreadSecurity(lua_State *L, ExecutionSecurity executionSecurity, std::int32_t identity);

        bool IsOurThread(lua_State *L);

        void ElevateClosure(const Closure *closure, RbxStu::StuLuau::ExecutionSecurity execSecurity);
        void ElevateClosureWithExplicitCapabilities(const Closure *closure, std::uintptr_t capabilities);

        bool IsOurClosure(Closure *closure);
    };
} // RbxStu::StuLuau
