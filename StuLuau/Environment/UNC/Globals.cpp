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
#include <lz4.h>
#include <Scanners/Rbx.hpp>

#include "lmem.h"
#include "lstring.h"
#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Globals::getgenv(lua_State *L) {
        const auto mainState = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L)->GetInitializationInformation()->executorState;

        if (mainState == L) {
            lua_pushvalue(L, LUA_GLOBALSINDEX); // We are parent thread.
            return 1;
        }

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

        executionEngine->YieldThread(L, [url](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
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

    int Globals::checkcaller(lua_State *L) {
        const auto luauSecurity = LuauSecurity::GetSingleton();
        lua_pushboolean(L, luauSecurity->IsOurThread(L));
        return 1;
    }

    int Globals::checkcallstack(lua_State *L) {
        const auto luauSecurity = LuauSecurity::GetSingleton();

        if (!luauSecurity->IsOurThread(L)) {
            lua_pushboolean(L, false);
            return 1;
        }

        for (auto startCi = L->base_ci; startCi < L->ci; startCi++) {
            if (!luauSecurity->IsOurClosure(clvalue(startCi->func))) {
                lua_pushboolean(L, false);
                return 1;
            }
        }

        lua_pushboolean(L, true);
        return 1;
    }

    int Globals::getreg(lua_State *L) {
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        return 1;
    }

    int Globals::identifyexecutor(lua_State *L) {
        lua_pushstring(L, "RbxStu");
        lua_pushstring(L, "V3");
        return 2;
    }

    int Globals::lz4compress(lua_State *L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        const char *data = lua_tostring(L, 1);
        const int iMaxCompressedSize = LZ4_compressBound(strlen(data));
        const auto pszCompressedBuffer = new char[iMaxCompressedSize];
        memset(pszCompressedBuffer, 0, iMaxCompressedSize);

        LZ4_compress_default(data, pszCompressedBuffer, strlen(data), iMaxCompressedSize);
        lua_pushlstring(L, pszCompressedBuffer, iMaxCompressedSize);
        return 1;
    }

    int Globals::lz4decompress(lua_State *L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TNUMBER);

        const char *data = lua_tostring(L, 1);
        const int data_size = lua_tointeger(L, 2);

        auto *pszUncompressedBuffer = new char[data_size];

        memset(pszUncompressedBuffer, 0, data_size);

        LZ4_decompress_safe(data, pszUncompressedBuffer, strlen(data), data_size);
        lua_pushlstring(L, pszUncompressedBuffer, data_size);
        return 1;
    }

    int Globals::isscriptable(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");
        const auto propName = std::string(luaL_checkstring(L, 2));

        for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
             const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
            if (prop->name == propName) {
                lua_pushboolean(L, prop->IsScriptable());
            }
        }
        if (lua_type(L, -1) != lua_Type::LUA_TBOOLEAN)
            lua_pushnil(L);

        return 1;
    }

    int Globals::setscriptable(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");
        const auto propName = std::string(luaL_checkstring(L, 2));
        const bool newScriptable = luaL_checkboolean(L, 3);

        for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
             const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
            if (prop->name == propName) {
                lua_pushboolean(L, prop->IsScriptable());
                prop->SetScriptable(newScriptable);
            }
        }
        if (lua_gettop(L) == 3)
            luaL_argerror(L, 2,
                      std::format("userdata<{}> does not have the property '{}'.",
                          Utilities::getInstanceType(L, 1).second, propName)
                      .c_str());

        return 1;
    }

    int Globals::gethiddenproperty(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");
        const auto propName = std::string(luaL_checkstring(L, 2));

        bool isPublic = false;
        for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
             const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
            if (prop->name == propName) {
                isPublic = prop->IsPublic();
                const auto isScriptable = prop->IsScriptable();
                prop->SetIsPublic(true);
                prop->SetScriptable(true);
                lua_getfield(L, 1, propName.c_str());
                prop->SetScriptable(isScriptable);
                prop->SetIsPublic(isPublic);
            }
        }
        if (lua_gettop(L) == 2)
            luaL_argerror(L, 2,
                      std::format("userdata<{}> does not have the property '{}'.",
                          Utilities::getInstanceType(L, 1).second, propName)
                      .c_str());

        lua_pushboolean(L, !isPublic);
        return 2;
    }

    int Globals::sethiddenproperty(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");
        const auto propName = std::string(luaL_checkstring(L, 2));
        luaL_checkany(L, 3);

        bool isPublic = false;
        for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
             const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
            if (prop->name == propName) {
                isPublic = prop->IsPublic();
                const auto isScriptable = prop->IsScriptable();
                prop->SetIsPublic(true);
                prop->SetScriptable(true);
                lua_setfield(L, 1, propName.c_str());
                prop->SetScriptable(isScriptable);
                prop->SetIsPublic(isPublic);
            }
        }
        if (lua_gettop(L) == 3) // lua_setfield will pop the new value of the property.
            luaL_argerror(L, 2,
                      std::format("userdata<{}> does not have the property '{}'.",
                          Utilities::getInstanceType(L, 1).second, propName)
                      .c_str());

        lua_pushboolean(L, !isPublic);
        return 1;
    }

    int Globals::getfpscap(lua_State *L) {
        const auto fflag = Scanners::RBX::GetSingleton()->GetFastFlag<std::int32_t *>("TaskSchedulerTargetFps");

        if (nullptr == fflag)
            luaL_error(L, "cannot getfpscap: could not find TaskSchedulerTargetFps during analysis stage.");

        lua_pushinteger(L, *fflag);
        return 1;
    }

    int Globals::setfpscap(lua_State *L) {
        luaL_checkinteger(L, 1);

        auto newFps = luaL_optinteger(L, 1, 60);

        if (newFps <= 0)
            newFps = 1000;

        const auto fflag = Scanners::RBX::GetSingleton()->GetFastFlag<std::int32_t *>("TaskSchedulerTargetFps");

        if (nullptr == fflag)
            luaL_error(L, "cannot setfpscap: could not find TaskSchedulerTargetFps during analysis stage.");

        *fflag = newFps;

        return 0;
    }

    int Globals::isluau(lua_State *L) {
        lua_pushboolean(L, true);
        return 1;
    }

    int Globals::getrawmetatable(lua_State *L) {
        luaL_checkany(L, 1);

        if (!lua_getmetatable(L, 1))
            lua_pushnil(L);

        return 1;
    }

    int Globals::setrawmetatable(lua_State *L) {
        luaL_checkany(L, 1);
        luaL_checktype(L, 2, ::lua_Type::LUA_TTABLE);

        // There may be more elements on the lua stack, this is stupid, but if we don't we will probably cause issues.
        if (lua_gettop(L) != 2)
            lua_pushvalue(L, 2);

        return lua_setmetatable(L, 1);
    }

    int Globals::getnamecallmethod(lua_State *L) {
        const auto szNamecall = lua_namecallatom(L, nullptr);

        if (szNamecall == nullptr)
            lua_pushnil(L);
        else
            lua_pushstring(L, szNamecall);

        return 1;
    }

    int Globals::setnamecallmethod(lua_State *L) {
        if (L->namecall != nullptr)
            L->namecall = luaS_new(L, luaL_checkstring(L, 1));

        return 0;
    }

    int Globals::setreadonly(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TTABLE);
        const bool bIsReadOnly = luaL_optboolean(L, 2, false);
        lua_setreadonly(L, 1, bIsReadOnly);

        return 0;
    }

    int Globals::isreadonly(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TTABLE);
        lua_pushboolean(L, lua_getreadonly(L, 1));
        return 1;
    }

    const luaL_Reg *Globals::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {
            {"getrenv", RbxStu::StuLuau::Environment::UNC::Globals::getrenv},
            {"getgenv", RbxStu::StuLuau::Environment::UNC::Globals::getgenv},
            {"gettenv", RbxStu::StuLuau::Environment::UNC::Globals::gettenv},
            {"httpget", RbxStu::StuLuau::Environment::UNC::Globals::httpget},

            {"checkcallstack", RbxStu::StuLuau::Environment::UNC::Globals::checkcallstack},
            {"checkcaller", RbxStu::StuLuau::Environment::UNC::Globals::checkcaller},

            {"getreg", RbxStu::StuLuau::Environment::UNC::Globals::getreg},

            {"identifyexecutor", RbxStu::StuLuau::Environment::UNC::Globals::identifyexecutor},

            {"lz4compress", RbxStu::StuLuau::Environment::UNC::Globals::lz4compress},
            {"lz4decompress", RbxStu::StuLuau::Environment::UNC::Globals::lz4decompress},

            {"isscriptable", RbxStu::StuLuau::Environment::UNC::Globals::isscriptable},
            {"setscriptable", RbxStu::StuLuau::Environment::UNC::Globals::setscriptable},

            {"gethiddenproperty", RbxStu::StuLuau::Environment::UNC::Globals::gethiddenproperty},
            {"sethiddenproperty", RbxStu::StuLuau::Environment::UNC::Globals::sethiddenproperty},

            {"getfpscap", RbxStu::StuLuau::Environment::UNC::Globals::getfpscap},
            {"setfpscap", RbxStu::StuLuau::Environment::UNC::Globals::setfpscap},

            {"isluau", RbxStu::StuLuau::Environment::UNC::Globals::isluau},

            {"getrawmetatable", RbxStu::StuLuau::Environment::UNC::Globals::getrawmetatable},
            {"setrawmetatable", RbxStu::StuLuau::Environment::UNC::Globals::setrawmetatable},

            {"getnamecallmethod", RbxStu::StuLuau::Environment::UNC::Globals::getnamecallmethod},
            {"setnamecallmethod", RbxStu::StuLuau::Environment::UNC::Globals::setnamecallmethod},

            {"setreadonly", RbxStu::StuLuau::Environment::UNC::Globals::setreadonly},
            {"isreadonly", RbxStu::StuLuau::Environment::UNC::Globals::isreadonly},

            {nullptr, nullptr}
        };

        return libreg;
    }

    bool Globals::PushToGlobals() { return true; }

    const char *Globals::GetLibraryName() { return "uncrbxstu"; }
}
