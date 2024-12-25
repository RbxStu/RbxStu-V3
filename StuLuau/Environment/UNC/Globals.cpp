//
// Created by Dottik on 25/10/2024.
//

#include "Globals.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <Utilities.hpp>

#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "lgc.h"

#include <Dependencies/HttpStatus.hpp>

#include <Scanners/Rbx.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <cpr/cpr.h>
#include <lz4.h>

#include <lapi.h>
#include <lmem.h>
#include <lstate.h>
#include <lstring.h>

#include "Roblox/Primitive.hpp"
#include "Roblox/Script.hpp"
#include "StuLuau/Extensions/luauext.hpp"
#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Globals::getgenv(lua_State *L) {
        const auto mainState = Scheduling::TaskSchedulerOrchestrator::GetSingleton()
                                       ->GetTaskScheduler()
                                       ->GetExecutionEngine(L)
                                       ->GetInitializationInformation()
                                       ->executorState;

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
        const auto rL = lua_mainthread(Scheduling::TaskSchedulerOrchestrator::GetSingleton()
                                               ->GetTaskScheduler()
                                               ->GetExecutionEngine(L)
                                               ->GetInitializationInformation()
                                               ->globalState);

        if (!rL->isactive)
            luaC_threadbarrier(rL);

        lua_pushvalue(rL, LUA_GLOBALSINDEX);
        lua_xmove(rL, L, 1);

        return 1;
    }

    int Globals::gettenv(lua_State *L) {
        // optional param == current thread
        if (lua_gettop(L) == 0 || (lua_gettop(L) == 1 && lua_type(L, 1) == ::lua_Type::LUA_TNIL))
            lua_pushthread(L);

        luaL_checktype(L, 1, ::lua_Type::LUA_TTHREAD);
        lua_normalisestack(L, 1);

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

    int Globals::settenv(lua_State *L) {
        lua_normalisestack(L, 2);
        luaL_checktype(L, 1, ::lua_Type::LUA_TTHREAD);
        luaL_checktype(L, 2, ::lua_Type::LUA_TTABLE);

        const auto th = lua_tothread(L, 1);

        if (!th->isactive)
            luaC_threadbarrier(th);

        if (th->gt == nullptr) {
            lua_pushnil(L);
        } else {
            lua_pushvalue(th, LUA_GLOBALSINDEX);
            lua_xmove(th, L, 1);
        }

        th->gt = &luaA_toobject(L, 2)->value.gc->h;
        return 1;
    }

    int Globals::httpget(lua_State *L) {
        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);

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

        executionEngine->YieldThread(
                L,
                [url](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
                    auto Headers = std::map<std::string, std::string, cpr::CaseInsensitiveCompare>();
                    Headers["User-Agent"] = "Roblox/WinInet";
                    Headers["RbxStu-Fingerprint"] = Utilities::GetHwid().value();

                    const auto response = cpr::Get(cpr::Url{url}, cpr::Header{Headers});

                    auto output = std::string("");

                    if (HttpStatus::IsError(response.status_code)) {
                        output = std::format(
                                "HttpGet failed\nResponse {} - {}. {}", std::to_string(response.status_code),
                                HttpStatus::ReasonPhrase(response.status_code), std::string(response.error.message));
                    } else {
                        output = response.text;
                    }

                    yieldRequest->fpCompletionCallback = [output, yieldRequest]() -> RbxStu::StuLuau::YieldResult {
                        lua_pushlstring(yieldRequest->lpResumeTarget, output.c_str(), output.size());
                        return {true, 1, {}};
                    };

                    yieldRequest->bIsReady = true;
                },
                true);

        return lua_yield(L, 0);
    }

    const std::array<std::string, 5> validMethods = {"get", "post", "put", "patch", "delete"};

    bool isValidHttpMethod(const std::string &method) {
        return std::ranges::find(validMethods, method) != validMethods.end();
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
        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);

        luaL_checktype(L, 1, LUA_TSTRING);

        if (lua_gettop(L) > 1)
            lua_settop(L, 1);

        auto str = &(L->top - 1)->value.gc->ts;
        const int iMaxCompressedSize = LZ4_compressBound(str->len);
        if (iMaxCompressedSize > 10000) {
            executionEngine->YieldThread(
                    L,
                    [L, str, iMaxCompressedSize](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
                        const auto pszCompressedBuffer = new char[iMaxCompressedSize];
                        memset(pszCompressedBuffer, 0, iMaxCompressedSize);

                        const auto actualSize =
                                LZ4_compress_default(str->data, pszCompressedBuffer, str->len, iMaxCompressedSize);


                        yieldRequest->fpCompletionCallback = [actualSize, L, pszCompressedBuffer,
                                                              yieldRequest]() -> RbxStu::StuLuau::YieldResult {
                            if (actualSize != 0)
                                lua_pushlstring(L, pszCompressedBuffer, actualSize);

                            return {actualSize != 0, 1, "compression failed"};
                        };

                        yieldRequest->bIsReady = true;
                    },
                    true);

            return lua_yield(L, 0);
        }

        const auto pszCompressedBuffer = new char[iMaxCompressedSize];
        memset(pszCompressedBuffer, 0, iMaxCompressedSize);

        const auto actualSize = LZ4_compress_default(str->data, pszCompressedBuffer, str->len, iMaxCompressedSize);

        if (actualSize == 0)
            luaL_error(L, "compression failed");

        lua_pushlstring(L, pszCompressedBuffer, actualSize);
        return 1;
    }

    int Globals::lz4decompress(lua_State *L) {
        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TNUMBER);

        if (lua_gettop(L) > 2)
            lua_settop(L, 2);

        auto str = &(L->top - 2)->value.gc->ts;
        const int dataSize = lua_tointeger(L, 2);

        if (dataSize > 10000 || str->len > 10000) {
            executionEngine->YieldThread(
                    L,
                    [dataSize, L, str](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
                        auto *pszUncompressedBuffer = new char[dataSize];

                        memset(pszUncompressedBuffer, 0, dataSize);

                        const auto ret = LZ4_decompress_safe(str->data, pszUncompressedBuffer, str->len, dataSize);

                        yieldRequest->fpCompletionCallback = [L, pszUncompressedBuffer, dataSize, ret,
                                                              yieldRequest]() -> RbxStu::StuLuau::YieldResult {
                            if (ret > 0)
                                lua_pushlstring(L, pszUncompressedBuffer, dataSize);

                            return {ret > 0, 1,
                                    std::format("lz4 decompression failed. lz4 error code: {}", ret).c_str()};
                        };

                        yieldRequest->bIsReady = true;
                    },
                    true);

            return lua_yield(L, 0);
        }

        auto *pszUncompressedBuffer = new char[dataSize];

        memset(pszUncompressedBuffer, 0, dataSize);

        const auto dwRetSize = LZ4_decompress_safe(str->data, pszUncompressedBuffer, str->len, dataSize);

        if (dwRetSize < 0)
            luaL_error(L, std::format("lz4 decompression failed. lz4 error code: {}", dwRetSize).c_str());

        lua_pushlstring(L, pszUncompressedBuffer, dataSize);
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

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);

        if (lua_topointer(L, 1) == executionEngine->GetInitializationInformation()->executorState->gt)
            luaL_argerror(L, 1, "cannot getrawmetatable on the global environment of the executor");

        return 1;
    }

    int Globals::setrawmetatable(lua_State *L) {
        luaL_checkany(L, 1);
        luaL_checktype(L, 2, ::lua_Type::LUA_TTABLE);

        // There may be more elements on the lua stack, this is stupid, but if we don't we will probably cause
        // issues.
        if (lua_gettop(L) != 2)
            lua_pushvalue(L, 2);

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);

        if (lua_topointer(L, 1) == executionEngine->GetInitializationInformation()->executorState->gt)
            luaL_argerror(L, 1, "cannot setrawmetatable on the global environment of the executor");


        lua_setmetatable(L, 1);
        lua_pushvalue(L, 1);
        return 1;
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

    int Globals::cloneref(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");

        const auto userdata = lua_touserdata(L, 1);
        const auto rawUserdata = *static_cast<void **>(userdata);

        const auto rbxPushInstance =
                RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance);

        if (rbxPushInstance == nullptr) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "Cannot perform cloneref: Failed to find RBX::Instance::pushInstance, a stub will be in "
                      "place.");
            lua_pushvalue(L, 1);
            return 1;
        }

        lua_pushlightuserdata(L, rbxPushInstance);

        lua_rawget(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, rawUserdata);
        lua_rawget(L, -2);

        lua_pushlightuserdata(L, rawUserdata);
        lua_pushnil(L);
        lua_rawset(L, -4);

        reinterpret_cast<r_RBX_Instance_pushInstance>(rbxPushInstance)(L, userdata);
        lua_pushlightuserdata(L, rawUserdata);
        lua_pushvalue(L, -3);
        lua_rawset(L, -5);
        return 1;
    }

    int Globals::compareinstances(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");
        Utilities::checkInstance(L, 2, "ANY");

        lua_pushboolean(L, *static_cast<const std::uintptr_t *>(lua_touserdata(L, 1)) ==
                                   *static_cast<const std::uintptr_t *>(lua_touserdata(L, 2)));

        return 1;
    }

    int Globals::getcallingscript(lua_State *L) {
        auto extraspace = GetThreadExtraspace(L);

        try {
            const auto rbxPushInstance = reinterpret_cast<r_RBX_Instance_pushInstance>(
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance));

            if (extraspace->script == nullptr || rbxPushInstance == nullptr)
                lua_pushnil(L);
            else {
                lua_pushlightuserdata(L, rbxPushInstance);
                lua_rawget(L, LUA_REGISTRYINDEX);
                lua_pushlightuserdata(L, extraspace->script);
                lua_rawget(L, -2);
                if (Utilities::getInstanceType(L, -1).first) {
                    // userdata object, safe to cloneref.
                    lua_pushcclosure(L, cloneref, nullptr, 0);
                    lua_pushvalue(L, -2);
                    lua_pcall(L, 1, 1, 0);
                }
            }
        } catch (const std::exception &e) {
            lua_pushstring(L, e.what());
            lua_pushnil(L);
        }

        return 1;
    }

    int Globals::gethui(lua_State *L) {
        // Equivalent to cloneref(cloneref(cloneref(game):GetService("CoreGui")).RobloxGui)
        // Excessive clonereffing, I made the cloneref, i will use all of it!
        // - Dottik
        lua_normalisestack(L, 0);

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_remove(L, 1);
        lua_getglobal(L, "game");
        lua_pushstring(L, "CoreGui");
        lua_call(L, 2, 1);

        lua_pushcclosure(L, cloneref, nullptr, 0);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);

        lua_getfield(L, -1, "RobloxGui");
        lua_pushcclosure(L, cloneref, nullptr, 0);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);

        return 1;
    }

    int Globals::getsenv(lua_State *L) {
        lua_normalisestack(L, 1);
        auto [isValidInstance, className] = Utilities::getInstanceType(L, 1);

        if (!isValidInstance)
            luaL_argerror(L, 1, std::format("expected ModuleScript or LocalScript, got {}", className).c_str());

        const auto rawInstance = *static_cast<uintptr_t *>(lua_touserdata(L, 1));

        if (className == "LocalScript") {
            const auto threadNode = *reinterpret_cast<uintptr_t *>(rawInstance + 0x198);

            if (!threadNode) {
                lua_pushnil(L);
                return 1;
            }

            const auto weakThreadRef = *reinterpret_cast<uintptr_t *>(threadNode + 0x8);
            const auto firstNode = *reinterpret_cast<uintptr_t *>(weakThreadRef + 0x18);
            const auto luaStateContainer = *reinterpret_cast<uintptr_t *>(firstNode + 0x20);
            const auto scriptLuaState =
                    reinterpret_cast<lua_State *>(*reinterpret_cast<uintptr_t *>(luaStateContainer + 0x8));

            if (!scriptLuaState) {
                lua_pushnil(L);
                return 1;
            }

            if (!lua_isthreadreset(scriptLuaState)) {
                lua_pushnil(L);
                return 1;
            }
            lua_rawcheckstack(L, 1);
            L->top->value.gc = reinterpret_cast<GCObject *>(scriptLuaState->gt);
            L->top->tt = LUA_TTABLE;
            incr_top(L);
            return 1;
        }

        if (className == "ModuleScript") {
            const auto threadNode = *reinterpret_cast<uintptr_t *>(rawInstance + 0x1C0);

            if (!threadNode) {
                lua_pushnil(L);
                return 1;
            }

            const auto firstNode = *reinterpret_cast<uintptr_t *>(threadNode);
            if (!firstNode) {
                lua_pushnil(L);
                return 1;
            }

            const auto moduleScriptLuaState =
                    reinterpret_cast<lua_State *>(*reinterpret_cast<uintptr_t *>(firstNode + 0x10));

            if (!moduleScriptLuaState) {
                lua_pushnil(L);
                return 1;
            }

            if (!lua_isthreadreset(moduleScriptLuaState)) {
                lua_pushnil(L);
                return 1;
            }

            lua_rawcheckstack(L, 1);
            L->top->value.gc = reinterpret_cast<GCObject *>(moduleScriptLuaState->gt);
            L->top->tt = LUA_TTABLE;
            incr_top(L);
            return 1;
        }

        return 0;
    }

    int Globals::isnetworkowner(lua_State *L) {
        Utilities::checkInstance(L, 1, "BasePart");
        auto part = *static_cast<void **>(lua_touserdata(L, 1));

        /*
         *  Quick rundown due to the hack around this function.
         *  We would normally access remoteId/peerId, but due to the fact that we are WE, we don't have that
         *  privilage, instead we must get the peerId of a part we ACTUALLY own, this wouldn't be that complicated.
         *
         *  But what is to be noted is the following:
         *      - On Local clients, RemoteId/PeerId is unreachable, PeerId == -1    (uint32_t max [underflowing]).
         *      - Anchored/Bound by a physics constraint/welded, PeerId == 2        (After trial and error)
         *
         *  Thus, the hack around this behaviour is to create a part ourselves. Parts created by ourselves that are
         *  parented to workspace are automatically simulated by our player, this means we really don't need
         *  anything else, and we can obtain our remote PeerId that way. This COULD be hardcoded, but then team
         *  tests would not work as expected. (PeerId appears to start at 4). We _cannot_ rely on HumanoidRootPart,
         *  as it can be owned by the server if the game developer desiers it.
         */

        auto partSystemAddress = RBX::SystemAddress{0};
        auto localPlayerAddress = RBX::SystemAddress{0};
        reinterpret_cast<::r_RBX_BasePart_getNetworkOwner>(RbxStuOffsets::GetSingleton()->GetOffset(
                RbxStuOffsets::OffsetKey::RBX_BasePart_getNetworkOwner))(part, &partSystemAddress);

        lua_getglobal(L, "Instance");
        lua_getfield(L, -1, "new");
        lua_pushstring(L, "Part");
        lua_getglobal(L, "workspace");
        lua_pcall(L, 2, 1, 0);
        reinterpret_cast<::r_RBX_BasePart_getNetworkOwner>(
                RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_BasePart_getNetworkOwner))(
                *static_cast<void **>(lua_touserdata(L, -1)), &localPlayerAddress);

        lua_getfield(L, -1, "Destroy");
        lua_pushvalue(L, -2);
        lua_pcall(L, 1, 0, 0);
        lua_pop(L, 1);

        lua_pushboolean(L, partSystemAddress.remoteId.peerId == 2 ||
                                   partSystemAddress.remoteId.peerId == localPlayerAddress.remoteId.peerId);
        return 1;
    }

    int Globals::firetouchinterest(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TUSERDATA);
        luaL_checktype(L, 2, lua_Type::LUA_TUSERDATA);
        luaL_checktype(L, 3, lua_Type::LUA_TNUMBER);

        Utilities::checkInstance(L, 1, "BasePart");
        Utilities::checkInstance(L, 2, "BasePart");
        const auto touchType = lua_tointeger(L, 3);

        if (touchType != 0 && touchType != 1)
            luaL_argerror(L, 3, "touch type must be either Touch (Integer<0>) or TouchEnded (Integer<1>).");

        const auto primitive0 = RbxStu::Roblox::Primitive::FromBasePart(*static_cast<void **>(lua_touserdata(L, 1)));
        const auto primitive1 = RbxStu::Roblox::Primitive::FromBasePart(*static_cast<void **>(lua_touserdata(L, 2)));

        const auto reportTouchInfo = reinterpret_cast<::r_RBX_World_reportTouchInfo>(
                RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_World_reportTouchInfo));

        if (reportTouchInfo == nullptr)
            luaL_error(L, "cannot firetouchinterest; RBX::BasePart::fireTouchSignals was not found during the "
                          "scanning step! If you believe this was caused by an update, contact the developers!");

        auto world = primitive0->GetWorld();

        if (world->GetRealStructure() == nullptr)
            world = primitive1->GetWorld();

        if (world->GetRealStructure() == nullptr)
            luaL_error(L, "cannot firetouchinterest: failed to find RBX::World *.");

        reportTouchInfo(world->GetRealStructure(), primitive0->GetRealStructure(), primitive1->GetRealStructure(),
                        static_cast<RBX::TouchEventType>(static_cast<std::uint8_t>(touchType)), true);

        return 0;
    }

    int Globals::fireproximityprompt(lua_State *L) {
        Utilities::checkInstance(L, 1, "ProximityPrompt");

        const auto proximityPrompt = *static_cast<std::uintptr_t **>(lua_touserdata(L, 1));
        reinterpret_cast<::r_RBX_ProximityPrompt_onTriggered>(RbxStuOffsets::GetSingleton()->GetOffset(
                RbxStuOffsets::OffsetKey::RBX_ProximityPrompt_onTriggered))(proximityPrompt);
        return 0;
    }

    int Globals::isrbxactive(lua_State *L) {
        lua_pushboolean(L, GetForegroundWindow() == GetCurrentProcess());
        return 1;
    }

    int Globals::getscriptbytecode(lua_State *L) {
        Utilities::checkInstance(L, 1, "LuaSourceContainer");
        auto [isInstance, className] = Utilities::getInstanceType(L, 1);

        if (!isInstance)
            luaL_error(L, "unknown error occurred");

        auto ppScript = static_cast<void **>(lua_touserdata(L, 1));

        auto scriptType = RbxStu::Roblox::Script::ScriptKind::Script;

        if (strcmp(className.c_str(), "ModuleScript") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::ModuleScript;
        } else if (strcmp(className.c_str(), "Script") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::Script;
        } else if (strcmp(className.c_str(), "LocalScript") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::LocalScript;
        }

        const RbxStu::Roblox::Script script{*ppScript, scriptType};

        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit) &&
            scriptType == RbxStu::Roblox::Script::ScriptKind::Script) {
            // Team Create, we do not support this function there, as the bytecode is in a weird ahh format.
            luaL_error(L, "unknown error occured while trying to fetch script bytecode.");
        }

        const auto bytecode = script.GetBytecode();
        if (!bytecode.has_value())
            luaG_runerrorL(L, "Failed to find bytecode");

        lua_rawcheckstack(L, 1);
        lua_pushlstring(L, bytecode.value().c_str(), bytecode.value().length());
        return 1;
    }

    int Globals::getscriptclosure(lua_State *L) {
        Utilities::checkInstance(L, 1, "LuaSourceContainer");
        auto [isInstance, className] = Utilities::getInstanceType(L, 1);

        if (!isInstance)
            luaL_error(L, "unknown error occurred");

        auto ppScript = static_cast<void **>(lua_touserdata(L, 1));

        auto scriptType = RbxStu::Roblox::Script::ScriptKind::Script;

        if (strcmp(className.c_str(), "ModuleScript") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::ModuleScript;
        } else if (strcmp(className.c_str(), "Script") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::Script;
        } else if (strcmp(className.c_str(), "LocalScript") == 0) {
            scriptType = RbxStu::Roblox::Script::ScriptKind::LocalScript;
        }

        const RbxStu::Roblox::Script script{*ppScript, scriptType};

        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit) &&
            scriptType == RbxStu::Roblox::Script::ScriptKind::Script) {
            // Team Create, we do not support this function there, as the bytecode is in a weird ahh format.
            luaL_error(L, "unknown error occured while trying to fetch script closure.");
        }

        const auto bytecode = script.GetBytecode();
        if (!bytecode.has_value())
            luaG_runerrorL(L, "Failed to find bytecode");

        lua_rawcheckstack(L, 1);
        if (luau_load(L, "=getscriptclosure", bytecode.value().c_str(), bytecode.value().length(), 0) != LUA_OK)
            lua_error(L);

        LuauSecurity::GetSingleton()->ElevateClosure(lua_toclosure(L, -1),
                                                     RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);

        return 1;
    }

    const luaL_Reg *Globals::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {
                {"getscriptclosure", RbxStu::StuLuau::Environment::UNC::Globals::getscriptclosure},
                {"getscriptbytecode", RbxStu::StuLuau::Environment::UNC::Globals::getscriptbytecode},

                {"isnetworkowner", RbxStu::StuLuau::Environment::UNC::Globals::isnetworkowner},
                {"fireproximityprompt", RbxStu::StuLuau::Environment::UNC::Globals::fireproximityprompt},
                {"firetouchinterest", RbxStu::StuLuau::Environment::UNC::Globals::firetouchinterest},

                {"gethui", RbxStu::StuLuau::Environment::UNC::Globals::gethui},

                {"cloneref", RbxStu::StuLuau::Environment::UNC::Globals::cloneref},
                {"compareinstances", RbxStu::StuLuau::Environment::UNC::Globals::compareinstances},

                {"getcallingscript", RbxStu::StuLuau::Environment::UNC::Globals::getcallingscript},

                {"getrenv", RbxStu::StuLuau::Environment::UNC::Globals::getrenv},
                {"getgenv", RbxStu::StuLuau::Environment::UNC::Globals::getgenv},

                {"gettenv", RbxStu::StuLuau::Environment::UNC::Globals::gettenv},
                {"settenv", RbxStu::StuLuau::Environment::UNC::Globals::settenv},

                {"httpget", RbxStu::StuLuau::Environment::UNC::Globals::httpget},

                {"checkcallstack", RbxStu::StuLuau::Environment::UNC::Globals::checkcallstack},
                {"checkcaller", RbxStu::StuLuau::Environment::UNC::Globals::checkcaller},

                {"getreg", RbxStu::StuLuau::Environment::UNC::Globals::getreg},

                {"identifyexecutor", RbxStu::StuLuau::Environment::UNC::Globals::identifyexecutor},
                {"getexecutorname", RbxStu::StuLuau::Environment::UNC::Globals::identifyexecutor},

                {"lz4compress", RbxStu::StuLuau::Environment::UNC::Globals::lz4compress},
                {"lz4decompress", RbxStu::StuLuau::Environment::UNC::Globals::lz4decompress},

                {"isscriptable", RbxStu::StuLuau::Environment::UNC::Globals::isscriptable},
                {"setscriptable", RbxStu::StuLuau::Environment::UNC::Globals::setscriptable},

                {"gethiddenproperty", RbxStu::StuLuau::Environment::UNC::Globals::gethiddenproperty},
                {"sethiddenproperty", RbxStu::StuLuau::Environment::UNC::Globals::sethiddenproperty},

                {"getfpscap", RbxStu::StuLuau::Environment::UNC::Globals::getfpscap},
                {"setfpscap", RbxStu::StuLuau::Environment::UNC::Globals::setfpscap},

                {"isluau", RbxStu::StuLuau::Environment::UNC::Globals::isluau},
                {"isrbxactive", RbxStu::StuLuau::Environment::UNC::Globals::isrbxactive},

                {"getrawmetatable", RbxStu::StuLuau::Environment::UNC::Globals::getrawmetatable},
                {"setrawmetatable", RbxStu::StuLuau::Environment::UNC::Globals::setrawmetatable},

                {"getnamecallmethod", RbxStu::StuLuau::Environment::UNC::Globals::getnamecallmethod},
                {"setnamecallmethod", RbxStu::StuLuau::Environment::UNC::Globals::setnamecallmethod},

                {"setreadonly", RbxStu::StuLuau::Environment::UNC::Globals::setreadonly},
                {"isreadonly", RbxStu::StuLuau::Environment::UNC::Globals::isreadonly},

                {"getsenv", RbxStu::StuLuau::Environment::UNC::Globals::getsenv},
                {nullptr, nullptr}};

        return libreg;
    }

    bool Globals::PushToGlobals() { return true; }

    const char *Globals::GetLibraryName() { return "uncrbxstu"; }
} // namespace RbxStu::StuLuau::Environment::UNC
