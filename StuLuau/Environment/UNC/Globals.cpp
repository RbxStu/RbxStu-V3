//
// Created by Dottik on 25/10/2024.
//

#include "Globals.hpp"

#include <Utilities.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lgc.h"
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"

#include <Dependencies/HttpStatus.hpp>

#include <cpr/cpr.h>

namespace RbxStu::StuLuau::Environment::UNC {
    int Globals::getgenv(lua_State *L) {
        const auto mainState = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L)->GetInitializationInformation()->executorState;

        if (!mainState->isactive)
            luaC_threadbarrier(mainState);

        lua_pushvalue(mainState, LUA_GLOBALSINDEX);
        lua_xmove(mainState, L, 1);

        return 1;
    }

    int Globals::getrenv(lua_State *L) {
        const auto rL = lua_mainthread(Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
            GetExecutionEngine(L)->GetInitializationInformation()->globalState);

        if (!rL->isactive)
            luaC_threadbarrier(rL);

        lua_pushvalue(rL, LUA_GLOBALSINDEX);
        lua_xmove(rL, L, 1);

        return 1;
    }

    int Globals::gettenv(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TTHREAD);
        lua_settop(L, 1);
        const auto th = lua_tothread(L, 1);

        if (!th->isactive)
            luaC_threadbarrier(th);

        if (th->gt == nullptr) {
            lua_pushnil(L);
            return 1;
        }

        lua_pushvalue(th, LUA_GLOBALSINDEX);
        lua_xmove(th, L, 1);
        return 1;
    }

    int Globals::httpget(lua_State *L) {
        const auto executionEngine = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L);

        std::string url;
        if (!lua_isstring(L, 1)) {
            // We might be called as namecall, try second one
            luaL_checkstring(L, 2);
            // If passes, it means it was called like namecall
            url = std::string(lua_tostring(L, 2));
        } else {
            url = std::string(lua_tostring(L, 1));
        }

        if (url.find("http://") == std::string::npos && url.find("https://") == std::string::npos)
            luaL_argerror(L, 1, "Invalid protocol (expected 'http://' or 'https://')");

        executionEngine->YieldThread(L, [url](const std::shared_ptr<RbxStu::StuLuau::YieldRequest>& yieldRequest) {
            auto Headers = std::map<std::string, std::string, cpr::CaseInsensitiveCompare>();
            Headers["User-Agent"] = "Roblox/WinInet";
            Headers["RbxStu-Fingerprint"] = Utilities::GetHwid().value();

            const auto response = cpr::Get(cpr::Url{url}, cpr::Header{Headers});

            auto output = std::string("");

            if (HttpStatus::IsError(response.status_code)) {
                output = std::format("HttpGet failed\nResponse {} - {}. {}",
                                     std::to_string(response.status_code),
                                     HttpStatus::ReasonPhrase(response.status_code),
                                     std::string(response.error.message));
            } else {
                output = response.text;
            }

            yieldRequest->fpCompletionCallback = [output, yieldRequest]() -> RbxStu::StuLuau::YieldResult {
                lua_pushlstring(yieldRequest->lpResumeTarget, output.c_str(), output.size());
                return {true, 1, {}};
            };

            yieldRequest->bIsReady = true;
        }, true);

        return lua_yield(L, 0);
    }


    const luaL_Reg *Globals::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {
            {"getrenv", RbxStu::StuLuau::Environment::UNC::Globals::getrenv},
            {"getgenv", RbxStu::StuLuau::Environment::UNC::Globals::getgenv},
            {"gettenv", RbxStu::StuLuau::Environment::UNC::Globals::gettenv},
            {"httpget", RbxStu::StuLuau::Environment::UNC::Globals::httpget},

            {nullptr, nullptr}
        };

        return libreg;
    }

    bool Globals::PushToGlobals() { return true; }

    const char *Globals::GetLibraryName() { return "rbxstu"; }
}
