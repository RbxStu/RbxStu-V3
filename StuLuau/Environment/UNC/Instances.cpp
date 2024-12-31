//
// Created by Pixeluted on 29/11/2024.
//

#include "Instances.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <Utilities.hpp>
#include <cstring>
#include <format>

#include "StuLuau/Environment/EnvironmentContext.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/Extensions/luauext.hpp"
#include "StuLuau/Interop/NativeObject.hpp"
#include "lgc.h"

namespace RbxStu::StuLuau::Environment::UNC {
    class RbxStuConnectionTagger final : public RbxStu::StuLuau::Environment::Interop::TaggedIdentifier {
    public:
        std::string GetTagName() override { return "RbxStuConnection"; }
    };

    class RbxStuConnection : public RbxStu::StuLuau::Environment::Interop::NativeObject<RbxStuConnectionTagger> {
        bool m_bIsOtherLVM;
        bool m_bIsC;
        bool m_bUsable;
        RBX::Signals::ConnectionSlot *m_pConnection;
        void(__fastcall *m_rpOriginalCFunction)(RBX::Signals::ConnectionSlot *);
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_ParentExecutionEngine;
        ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> m_stubRef;
        ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> m_rpRealFunction;
        ReferencedLuauObject<lua_State *, lua_Type::LUA_TTHREAD> m_rpThread;

        static void __CStub(RBX::Signals::ConnectionSlot *con) {}

        __forceinline void ThrowIfUnusable(lua_State *L) {
            if (!this->m_bUsable)
                luaL_error(L, "RbxStuConnection is disposed and cannot be used.");
        }

        static int GetFunction(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);
            lua_rawcheckstack(L, 1);

            if (!pConnection->m_bIsC &&
                lua_mainthread(pConnection->m_pConnection->pFunctionSlot->objRef->thread) == lua_mainthread(L))
                lua_getref(L, pConnection->m_rpRealFunction.luaRef);
            else
                lua_pushnil(L);

            return 1;
        }

        static int GetEnabled(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);
            lua_rawcheckstack(L, 1);
            auto hasRefs = (pConnection->m_pConnection->weak != 0 || pConnection->m_pConnection->strong != 0);
            if (pConnection->m_bIsC)
                lua_pushboolean(L, hasRefs && pConnection->m_pConnection->call_Signal ==
                                                      pConnection->m_rpOriginalCFunction);
            else
                lua_pushboolean(L, hasRefs && pConnection->m_pConnection->pFunctionSlot->objRef->objectId ==
                                                      pConnection->m_rpRealFunction.luaRef);

            return 1;
        }

        // TODO: Complete Disconnect, Enable and Disable for C based connections.

        static int Enable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            if (!pConnection->m_bIsC) {
                pConnection->m_pConnection->pFunctionSlot->objRef->objectId = pConnection->m_rpRealFunction.luaRef;
            } else {
                pConnection->m_pConnection->call_Signal = pConnection->m_rpOriginalCFunction;
            }

            return 0;
        }

        static int Disable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            if (!pConnection->m_bIsC) {
                pConnection->m_pConnection->pFunctionSlot->objRef->objectId = pConnection->m_stubRef.luaRef;
            } else {
                pConnection->m_pConnection->call_Signal = __CStub;
            }

            return 0;
        }

        static int Disconnect(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            const auto newUserdata = static_cast<RBX::Signals::ConnectionSlot **>(
                    lua_newuserdatatagged(L, sizeof(void *), 4)); // 4 is the right tag for RBXScriptConnection
            *newUserdata = pConnection->m_pConnection;

            lua_getfield(L, LUA_REGISTRYINDEX, "RBXScriptConnection");
            lua_setmetatable(L, -2);

            lua_getfield(L, -1, "Disconnect");
            lua_pushvalue(L, -2);
            lua_call(L, 1, 0);

            pConnection->m_bUsable = false;


            return 0;
        }

        static int Fire(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            if (pConnection->m_bIsC) {
                return 0;
            }

            pConnection->m_pConnection->weak++;


            if (pConnection->m_bIsOtherLVM) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                          "Not firing externally declared connection, it is not our LVM, as such it is risky to "
                          "pass arguments. Albeit that may change when I implement and differenciate between "
                          "GCable objects.");

                luaL_error(L, "cannot fire a connection across the Luau VM boundary.");    // TODO: Complete cross-LVM firing.

                /*
                 *  The function is declared in another LVM, we have to be excessively careful with how we handle
                 *  parameters onto the signal. We must disallow Table * and userdata (Although we can pushInstance
                 *  on userdata if they're such)
                 */

                auto rpThread = pConnection->m_pConnection->pFunctionSlot->objRef->thread;
                auto function = pConnection->m_rpRealFunction.GetReferencedObject(rpThread);

                if (!function.has_value()) {
                    pConnection->m_pConnection->weak--;
                    return 0;
                }
                luaC_threadbarrier(rpThread);


                const auto nL = lua_newthread(pConnection->m_pConnection->pFunctionSlot->objRef
                                                      ->thread); // Avoid dangerous environment primitives.
                luaL_sandboxthread(nL);
                lua_pop(rpThread, 1);
                lua_rawcheckstack(nL, 1);
                lua_getref(nL, pConnection->m_rpRealFunction.luaRef);

                for (auto stackbase = L->base; stackbase != L->top; stackbase++) {
                    switch (static_cast<::lua_Type>(stackbase->tt)) {
                        case LUA_TNIL:
                            lua_pushnil(nL);
                            break;
                        case LUA_TBOOLEAN:
                            lua_pushboolean(nL, stackbase->value.b);
                            break;
                        case LUA_TLIGHTUSERDATA:
                            lua_pushnil(nL); // TODO: Implement with pushinstance.
                            break;
                        case LUA_TUSERDATA:
                            lua_pushnil(nL); // TODO: Implement with pushinstance.
                            break;
                        case LUA_TNUMBER:
                            lua_pushnumber(nL, stackbase->value.n);
                            break;
                        case LUA_TVECTOR:
                            lua_pushnil(nL); // TODO: Implement
                            break;
                        case LUA_TSTRING:
                            lua_pushlstring(nL, stackbase->value.gc->ts.data, stackbase->value.gc->ts.len);
                            break;
                        case LUA_TTABLE:
                            lua_pushnil(nL);
                            break;
                        case LUA_TFUNCTION:
                            lua_pushnil(nL);
                            break;
                        case LUA_TTHREAD:
                            lua_pushnil(nL);
                            break;
                        case LUA_TBUFFER: {
                            const auto memory = lua_newbuffer(nL, stackbase->value.gc->buf.len);
                            memcpy(memory, stackbase->value.gc->buf.data, stackbase->value.gc->buf.len);
                            break;
                        }
                        case LUA_TPROTO:
                            lua_pushnil(nL);
                            break;
                        case LUA_TUPVAL:
                            lua_pushnil(nL);
                            break;
                        default:;
                    }
                }

                const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
                        RbxStuOffsets::GetSingleton()->GetOffset(
                                RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

                task_defer(nL);

                pConnection->m_pConnection->weak--;
                return 0;
            }

            const auto func = pConnection->m_rpRealFunction.GetReferencedObject(L);

            if (!func.has_value()) {
                pConnection->m_pConnection->weak--;
                luaL_error(L, "no function associated with RbxStuConnection object");
            }

            if (pConnection->m_pConnection->weak == 0 && pConnection->m_pConnection->strong == 0 ||
                pConnection->m_pConnection->pFunctionSlot->objRef->objectId != pConnection->m_rpRealFunction.luaRef) {
                pConnection->m_pConnection->weak--;

                return 0;
            }

            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);
            L->top->tt = lua_Type::LUA_TFUNCTION;
            L->top->value.p = func.value();
            L->top++;
            lua_insert(L, 1);
            lua_remove(L, 2);
            luaC_threadbarrier(pConnection->m_pConnection->pFunctionSlot->objRef->thread);
            const auto nL = lua_newthread(pConnection->m_pConnection->pFunctionSlot->objRef
                                                  ->thread); // Avoid dangerous environment primitives.
            luaL_sandboxthread(nL);
            lua_pop(pConnection->m_pConnection->pFunctionSlot->objRef->thread, 1);
            lua_xmove(L, nL, lua_gettop(L));

            const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

            task_defer(nL);
            // else {
            //     luaL_error(L, "C connections cannot be fired due to security concerns.");
            // }

            pConnection->m_pConnection->weak--;
            return 0;
        }

        static int GetForeignState(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            lua_pushboolean(
                    L, pConnection->m_bIsC ||
                               ((pConnection->m_pConnection->pFunctionSlot->objRef->thread &&
                                 pConnection->m_pConnection->pFunctionSlot->objRef->thread->global) &&
                                lua_mainthread(L) !=
                                        lua_mainthread(pConnection->m_pConnection->pFunctionSlot->objRef->thread)));

            return 1;
        }

        static int GetLuaConnection(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            lua_pushboolean(L, !pConnection->m_bIsC);
            return 1;
        }

        static int GetThread(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_settop(L, 1);

            if (!pConnection->m_bIsC &&
                lua_mainthread(pConnection->m_pConnection->pFunctionSlot->objRef->thread) == lua_mainthread(L)) {
                lua_getref(L, pConnection->m_rpThread.luaRef);
            } else {
                lua_pushnil(L);
            }

            return 1;
        }


    public:
        static int stub(lua_State *L) { return 0; }

        RbxStuConnection(RBX::Signals::ConnectionSlot *slot, lua_State *parentState, bool isC) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                      std::format("Creating RbxStuConnection on connection {}...", (void *) slot));
            this->m_pConnection = slot;
            this->m_ParentExecutionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()
                                                    ->GetTaskScheduler()
                                                    ->GetExecutionEngine(parentState);

            auto signalOriginal = m_ParentExecutionEngine->GetSignalOriginal(this->m_pConnection);

            if (!signalOriginal.has_value()) {
                // Does not have a value, initialization.
                auto signalInfo = std::make_shared<SignalInformation>(isC);

                if (signalInfo->bIsCSignal)
                    signalInfo->fpCallFunction = slot->call_Signal;
                else
                    signalInfo->luaRef = slot->pFunctionSlot->objRef->objectId;

                this->m_ParentExecutionEngine->SetSignalOriginal(this->m_pConnection, signalInfo);
                signalOriginal = signalInfo; // Set init.
            }

            if (!isC) {
                if (signalOriginal.has_value() && !signalOriginal.value()->bIsCSignal)
                    this->m_rpRealFunction.luaRef = signalOriginal.value()->luaRef;

                this->m_rpThread.luaRef = slot->pFunctionSlot->objRef->thread_ref;
                this->m_bIsOtherLVM =
                        lua_mainthread(slot->pFunctionSlot->objRef->thread) != lua_mainthread(parentState);
            }

            if (signalOriginal.has_value() && signalOriginal.value()->bIsCSignal)
                this->m_rpOriginalCFunction = signalOriginal.value()->fpCallFunction;

            this->m_functionMap = {{"Fire", {-1, Fire}},
                                   {"Defer", {-1, Fire}},
                                   {"Disable", {1, Disable}},
                                   {"Enable", {1, Enable}},
                                   {"Disconnect", {1, Disconnect}}};

            this->m_propertyMap = {
                    {"Enabled", {GetEnabled, 1, {}}},
                    {"Function", {GetFunction, 1, {}}},
                    {"LuaConnection", {GetLuaConnection, 1, {}}},
                    {"ForeignState", {GetForeignState, 1, {}}},
                    {"Thread", {GetThread, 1, {}}},
            };

            this->m_ParentExecutionEngine->AssociateObject(
                    std::make_shared<AssociatedObject>([this]() { delete this; }));

            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Pushing closure stub...");

            if (!isC) {
                lua_pushcclosure(parentState, stub, nullptr, 0);
                this->m_stubRef.luaRef = lua_ref(parentState, -1);
                lua_pop(parentState, 1);
            }

            this->m_bUsable = true;
            this->m_bIsC = isC;
        }
    };

    int Instances::getconnections(lua_State *L) {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        lua_getglobal(L, "typeof");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        if (strcmp(lua_tostring(L, -1), "RBXScriptSignal") != 0)
            luaL_argerrorL(L, 1, "Expected RBXScriptSignal");
        lua_pop(L, 1);

        lua_getfield(L, 1, "Connect");
        lua_pushvalue(L, 1);
        lua_pushcfunction(L, [](lua_State *) -> int { return 0; }, "");
        lua_call(L, 2, 1);

        const auto rawConnection = reinterpret_cast<RBX::Signals::ConnectionSlot *>( // SignalBridge is at top.
                *reinterpret_cast<std::uintptr_t *>(*static_cast<std::uintptr_t *>(lua_touserdata(L, -1)) + 0x10));
        RbxStuLog(Warning, Anonymous, std::format("Root: {}", (void *) rawConnection));

        lua_getfield(L, -1, "Disconnect");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);

        lua_newtable(L);
        RBX::Signals::ConnectionSlot *slot = rawConnection;
        auto idx = 1;
        while (slot != nullptr && Utilities::IsPointerValid(slot)) {
            RbxStuLog(Warning, Anonymous, std::format("new userdata"));
            const auto connection = reinterpret_cast<RbxStuConnection **>(lua_newuserdata(L, sizeof(void *)));

            auto bIsC = !Utilities::IsPointerValid(slot->pFunctionSlot) ||
                        !Utilities::IsPointerValid(slot->pFunctionSlot->objRef) ||
                        !Utilities::IsPointerValid(slot->pFunctionSlot->objRef->thread);
            if (bIsC) {
                RbxStuLog(Warning, Anonymous, std::format("Connection originating from C++: {}", (void *) slot));
                // slot = slot->pNext;
                // lua_pop(L, 1);
                // continue;
            } else {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                          std::format("Connection originating from Luau: {}", (void *) slot));

                if (!Utilities::IsPointerValid(slot->pFunctionSlot->objRef->thread) ||
                    !Utilities::IsPointerValid(slot->pFunctionSlot->objRef->thread->global) ||
                    !Utilities::IsPointerValid(slot->pFunctionSlot->objRef->thread->global->mainthread)) {
                    RbxStuLog(Warning, Anonymous,
                              std::format(
                                      "Thread pointer is invalid, cannot determine the LVM context of this connection."
                                      " Connection Address: {}",
                                      (void *) slot));
                    slot = slot->pNext;
                    lua_pop(L, 1);
                    continue;
                }

                // ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> func{slot->pFunctionSlot->objRef->objectId};
                // auto obj = func.GetReferencedObject(slot->pFunctionSlot->objRef->thread);

                // if (!obj.has_value()) {
                //     RbxStuLog(Warning, Anonymous,
                //               std::format("ObjectId is not valid, cannot fetch callback from the thread that the ref
                //               "
                //                           "seemingly originates from. Connection: {}",
                //                           (void *) slot));
                //     slot = slot->pNext;
                //     lua_pop(L, 1);
                //     continue;
                // }
            }
            RbxStuLog(Warning, Anonymous, std::format("creating rbxstu connection"));
            *connection = new RbxStuConnection{slot, L, bIsC};

            RbxStuConnectionTagger tagger{};

            RbxStuLog(Warning, Anonymous, std::format("creating metatable"));
            lua_newtable(L);
            lua_pushstring(L, tagger.GetTagName().c_str());
            lua_setfield(L, -2, "__type");
            lua_pushcclosure(L, RbxStuConnection::__index<RbxStuConnectionTagger>, nullptr, 0);
            lua_setfield(L, -2, "__index");
            lua_pushcclosure(L, RbxStuConnection::__namecall<RbxStuConnectionTagger>, nullptr, 0);
            lua_setfield(L, -2, "__namecall");
            lua_setmetatable(L, -2);

            lua_rawseti(L, -2, idx);
            idx++;
            slot = slot->pNext;
        }

        if (lua_type(L, -1) != LUA_TTABLE)
            lua_newtable(L);

        return 1;
    }

    int Instances::getinstancelist(lua_State *L) {
        lua_normalisestack(L, 0);

        const auto rbxPushInstance =
                RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance);

        if (rbxPushInstance == nullptr) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "Cannot perform getinstancelist: Failed to find RBX::Instance::pushInstance.");
            lua_newtable(L);
            return 1;
        }

        lua_pushvalue(L, LUA_REGISTRYINDEX);
        lua_pushlightuserdata(L, rbxPushInstance);
        lua_rawget(L, -2);
        return 1;
    }

    int Instances::fireclickdetector(lua_State *L) {
        Utilities::checkInstance(L, 1, "ClickDetector");
        auto fireDistance = luaL_optnumber(L, 2, 0);
        auto eventName = luaL_optstring(L, 3, "MouseClick");

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "Players");
        lua_call(L, 2, 1);
        lua_getfield(L, -1, "LocalPlayer");

        if (strcmp(eventName, "MouseClick") == 0) {
            const auto fireClick = reinterpret_cast<r_RBX_ClickDetector_fireClick>(
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireClick));

            fireClick(*static_cast<void **>(lua_touserdata(L, 1)), static_cast<float>(fireDistance),
                      *static_cast<void **>(lua_touserdata(L, -1)));
        } else if (strcmp(eventName, "RightMouseClick") == 0) {
            const auto fireClick =
                    reinterpret_cast<r_RBX_ClickDetector_fireClick>(RbxStuOffsets::GetSingleton()->GetOffset(
                            RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireRightClick));

            fireClick(*static_cast<void **>(lua_touserdata(L, 1)), static_cast<float>(fireDistance),
                      *static_cast<void **>(lua_touserdata(L, -1)));
        } else if (strcmp(eventName, "MouseHoverEnter") == 0) {
            const auto fireHover =
                    reinterpret_cast<r_RBX_ClickDetector_fireHover>(RbxStuOffsets::GetSingleton()->GetOffset(
                            RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseHover));

            fireHover(*static_cast<void **>(lua_touserdata(L, 1)), *static_cast<void **>(lua_touserdata(L, -1)));
        } else if (strcmp(eventName, "MouseHoverLeave") == 0) {
            const auto fireHover =
                    reinterpret_cast<r_RBX_ClickDetector_fireHover>(RbxStuOffsets::GetSingleton()->GetOffset(
                            RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseLeave));

            fireHover(*static_cast<void **>(lua_touserdata(L, 1)), *static_cast<void **>(lua_touserdata(L, -1)));
        }

        return 0;
    }

    const char *Instances::GetLibraryName() { return "instances"; }

    bool Instances::PushToGlobals() { return true; }

    const luaL_Reg *Instances::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {{"getconnections", getconnections},
                                    {"fireclickdetector", Instances::fireclickdetector},
                                    {"getinstancelist", Instances::getinstancelist},
                                    {nullptr, nullptr}};

        return libreg;
    }

} // namespace RbxStu::StuLuau::Environment::UNC
