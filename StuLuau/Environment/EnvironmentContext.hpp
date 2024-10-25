//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include <memory>
#include <functional>
#include <utility>
#include <lstate.h>

namespace RbxStu::StuLuau {
    class ExecutionEngine;
}

namespace RbxStu::StuLuau::Environment {
    class Library abstract;

    class EnvironmentContext final {
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_parentEngine;

        std::vector<std::shared_ptr<RbxStu::StuLuau::Environment::Library> > m_libraries;
        std::vector<Closure *> m_unhookableClosures;

    public:
        explicit EnvironmentContext(
            const std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> &parentEngine) : m_parentEngine(
            parentEngine) {
        };

        void DefineLibrary(const std::shared_ptr<RbxStu::StuLuau::Environment::Library> &library);

        void DefineDataModelHook(std::string_view szMetamethodName,
                                 std::function<std::int32_t(lua_State *)> func);

        void MakeUnhookable(Closure *closure);

        void PushEnviornment();
    };
} // RbxStu::StuLuau::Environment
