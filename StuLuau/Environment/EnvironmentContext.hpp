//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include <memory>
#include <functional>
#include <Logger.hpp>
#include <utility>
#include <optional>
#include <lstate.h>

#include "Library.hpp"

namespace RbxStu::StuLuau {
    class ExecutionEngine;
}

namespace RbxStu::StuLuau::Environment {
    class Library abstract;

    struct InitScript {
        std::string scriptSource;
        std::string scriptName;
    };

    struct HookInformation {
        Closure *original;
    };

    template<typename T, ::lua_Type U>
    struct ReferencedLuauObject {
        int luaRef;

        std::optional<T> GetReferencedObject(lua_State *L) {
            try {
                lua_getref(L, luaRef);
                if (lua_type(L, -1) != U) {
                    lua_pop(L, 1);
                    return {};
                }

                const auto ptr = lua_topointer(L, -1);
                lua_pop(L, 1);
                return reinterpret_cast<T>(const_cast<void *>(ptr));
            } catch (const std::exception &ex) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                          std::format("Invalid ref? Cxx exception: {}", ex.what()));
                return {};
            }
        }
    };

    class EnvironmentContext final {
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_parentEngine;

        std::vector<InitScript> m_initScripts;
        std::vector<std::shared_ptr<RbxStu::StuLuau::Environment::Library> > m_libraries;
        std::vector<Closure *> m_unhookableClosures;

    public:
        std::map<Closure *, HookInformation> m_functionHooks;
        std::map<Closure *, ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION> > m_newcclosures;

        explicit EnvironmentContext(
            const std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> &parentEngine) : m_parentEngine(
            parentEngine) {
        };

        ~EnvironmentContext();

        void DefineInitScript(const std::string &scriptSource, const std::string &scriptName);

        void DefineLibrary(const std::shared_ptr<RbxStu::StuLuau::Environment::Library> &library);

        template<typename T>
        void DefineLibrary() {
            const auto library = std::make_shared<T>();

            for (const auto &lib: this->m_libraries) {
                if (strcmp(lib->GetLibraryName(), library->GetLibraryName()) == 0) {
                    RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                              std::format("Library already defined, ignoring re-definition. Affected Library: {}",
                                  library->
                                  GetLibraryName()));
                    return;
                }
            }

            this->m_libraries.emplace_back(library);
        }

        void DefineDataModelHook(std::string_view szMetamethodName,
                                 std::function<std::int32_t(lua_State *)> func);

        void MakeUnhookable(Closure *closure);

        void PushEnviornment();
    };
} // RbxStu::StuLuau::Environment
