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

    struct HookInputState {
        lua_State *L;
        lua_CFunction original;
    };

    struct HookReturnState {
        bool bContinueNextHook;
        bool bInvokeLuaYield;
        std::int32_t returnCount;
    };

    struct InitScript {
        std::string scriptSource;
        std::string scriptName;
    };

    template<typename T, ::lua_Type U>
    struct ReferencedLuauObject {
        int luaRef;

        ReferencedLuauObject() {
            this->luaRef = LUA_REFNIL;
        }

        explicit ReferencedLuauObject(int ref) {
            this->luaRef = ref;
        }

        ReferencedLuauObject(lua_State *L, int idx) {
            this->luaRef = lua_ref(L, idx);
        }

        std::optional<T> GetReferencedObject(lua_State *L) {
            try {
                if (this->luaRef <= LUA_REFNIL) return {};

                lua_getref(L, this->luaRef);
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

    enum class FunctionKind {
        NewCClosure,
        CClosure,
        LuauClosure
    };

    struct HookInformation {
        ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION> original;
        ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION> hookedWith;

        FunctionKind dwHookedType;
        FunctionKind dwHookWithType;
    };


    class EnvironmentContext final {
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_parentEngine;

        std::vector<InitScript> m_initScripts;
        std::vector<std::shared_ptr<RbxStu::StuLuau::Environment::Library> > m_libraries;
        std::vector<Closure *> m_unhookableClosures;

        std::atomic_bool m_bIsDestroyed;

    public:
        std::map<Closure *, HookInformation> m_functionHooks;
        std::map<Closure *, ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION> > m_newcclosures;

        explicit EnvironmentContext(
            const std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> &parentEngine) : m_parentEngine(
            parentEngine) {
        };

        ~EnvironmentContext();

        void DefineNewDataModelMetaMethodClosure(Closure *originalMetamethod, Closure *func) const;

        void DestroyContext();

        void DefineInitScript(const std::string &scriptSource, const std::string &scriptName);

        void DefineLibrary(const std::shared_ptr<RbxStu::StuLuau::Environment::Library> &library);

        template<typename T>
        void DefineLibrary() {
            const auto library = std::make_shared<T>();

            for (const auto &lib: this->m_libraries) {
                if (strcmp(lib-> GetLibraryName(), library->GetLibraryName()) == 0) {
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
                                 std::function<HookReturnState(const HookInputState &)> func);

        void MakeUnhookable(Closure *closure);

        void PushEnvironment();

        bool IsWrappedClosure(Closure *cl) const;

        bool IsDataModelMetamethod(Closure * closure) const;;
    };
} // RbxStu::StuLuau::Environment
