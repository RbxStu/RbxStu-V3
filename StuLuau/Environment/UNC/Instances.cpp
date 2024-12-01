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
#include "StuLuau/Interop/NativeObject.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    struct ConnectionSlot;
    struct WeakThreadRef_Connections {
        std::int64_t *_Refs;
        lua_State *thread;
        std::int64_t thread_ref;
        std::int64_t objectId;
    };

    struct LuauFunctionSlot {
        void *vft;
        char filler[104];
        RBX::Lua::WeakThreadRef *objRef;
    };

    struct SlotInfo {
        int unk;
        int connectionCount;
        ConnectionSlot *head;
    };

    struct ConnectionSlot {
        int strong;
        int weak;
        void(__fastcall *call_Signal)(ConnectionSlot *connection, void *, void *, void *, void *, void *, void *,
                                      void *, void *, void *, void *);
        ConnectionSlot *pNext;
        int unk2;
        int unk3;
        SlotInfo *pConnections;
        void(__fastcall *destroySignal)(ConnectionSlot *connection);
        LuauFunctionSlot
                *pFunctionSlot; // This may not point to a valid Luau Function slot if the connection is a C connection
        /*
         *  What follows are a set of pointers read respective from LuauFunctionSlot, all valid pointers after these
         * point (Before the invalid) are to be called with call_Signal.
         */
    };

    class RbxStuConnectionTagger final : public RbxStu::StuLuau::Environment::Interop::TaggedIdentifier {
    public:
        std::string GetTagName() override { return "RbxStuConnection"; }
    };

    class RbxStuConnection : public RbxStu::StuLuau::Environment::Interop::NativeObject<RbxStuConnectionTagger> {
        bool m_bIsOtherLVM;
        bool m_bIsC;
        bool m_bUsable;
        ConnectionSlot *m_pConnection;
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_ParentExecutionEngine;
        ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> m_stubRef;
        ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> m_rpRealFunction;
        ReferencedLuauObject<lua_State *, lua_Type::LUA_TTHREAD> m_rpThread;

        void ThrowIfUnusable(lua_State *L) {
            if (!this->m_bUsable)
                luaL_error(L, "RbxStuConnection is disposed and cannot be used.");
        }

        static int GetFunction(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
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
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_rawcheckstack(L, 1);
            lua_pushboolean(L, (pConnection->m_pConnection->weak != 0 || pConnection->m_pConnection->strong != 0) &&
                                       pConnection->m_pConnection->pFunctionSlot->objRef->objectId ==
                                               pConnection->m_rpRealFunction.luaRef);
            return 1;
        }

        // TODO: Complete Disconnect, Enable and Disable for C based connections.

        static int Enable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            // pConnection->connection->Connected = 1;
            if (!pConnection->m_bIsC) {
                pConnection->m_pConnection->pFunctionSlot->objRef->objectId = pConnection->m_rpRealFunction.luaRef;
            }
            return 1;
        }

        static int Disable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            // pConnection->connection->Connected = 0;
            if (!pConnection->m_bIsC) {
                pConnection->m_pConnection->pFunctionSlot->objRef->objectId = pConnection->m_stubRef.luaRef;
            }
            return 1;
        }

        static int Disconnect(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            if (!pConnection->m_bIsC) { // Lua closure disconnection.
                auto newUserdata = static_cast<ConnectionSlot **>(
                        lua_newuserdatatagged(L, sizeof(void *), 4)); // 4 is the right tag for RBXScriptConnection
                *newUserdata = pConnection->m_pConnection;

                lua_getfield(L, LUA_REGISTRYINDEX, "RBXScriptConnection");
                lua_setmetatable(L, -2);

                lua_getfield(L, -1, "Disconnect");
                lua_pushvalue(L, -2);
                lua_call(L, 1, 0);

                pConnection->m_bUsable = false;
            } else {
                luaL_error(L, "C closure disconnection is not implemented");
            }

            return 0;
        }

        static int Fire(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            pConnection->m_pConnection->weak++;

            if (!pConnection->m_bIsC) {
                if (pConnection->m_bIsOtherLVM) {
                    RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                              "Not firing externally declared connection, it is not our LVM, as such it is risky to "
                              "pass arguments. Albeit that may change when I implement and differenciate between "
                              "GCable objects.");
                    return 0;
                }

                const auto func = pConnection->m_rpRealFunction.GetReferencedObject(L);

                if (!func.has_value())
                    luaL_error(L, "no function associated with RbxStuConnection object");

                if (pConnection->m_pConnection->weak == 0 && pConnection->m_pConnection->strong == 0 ||
                    pConnection->m_pConnection->pFunctionSlot->objRef->objectId !=
                            pConnection->m_rpRealFunction.luaRef) {
                    lua_pushnil(L);
                    return 1;
                }

                lua_rawcheckstack(L, 1);
                L->top->tt = lua_Type::LUA_TFUNCTION;
                L->top->value.p = func.value();
                L->top++;
                lua_insert(L, 1);
                lua_remove(L, 2);
                auto nL = lua_newthread(L->global->mainthread); // Avoid dangerous environment primitives.
                luaL_sandboxthread(nL);
                lua_pop(L->global->mainthread, 1);
                lua_xmove(L, nL, lua_gettop(L));

                const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
                        RbxStuOffsets::GetSingleton()->GetOffset(
                                RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

                task_defer(nL);
            } else {
                /*
                 *  Hard Coding my beloooooved.
                 */

                auto possiblyValidArgumentsWithNoRegardsForMemorySafety = 0;

                /**/

                for (auto current = reinterpret_cast<std::uintptr_t>(pConnection->m_pConnection->pFunctionSlot);
                     (Utilities::IsPointerValid(reinterpret_cast<void ***>(current)) &&
                      Utilities::IsPointerValid(*reinterpret_cast<void ***>(current)));
                     current += sizeof(void *))
                    possiblyValidArgumentsWithNoRegardsForMemorySafety++;
                if (possiblyValidArgumentsWithNoRegardsForMemorySafety > 1)
                    possiblyValidArgumentsWithNoRegardsForMemorySafety = 1;
                lua_remove(L, 1);
                const int argc = lua_gettop(L);

                if (argc > 10)
                    luaL_error(L, "Too many arguments for a C RBXScriptSignal to be fired, are you nuts?");

                // Step through args, ignore tables.

                auto pendingFreeOperations = std::vector<std::function<void()>>{};
                auto args = std::array<void *, 10>{};

                auto pointerToZero = 0;

                for (auto idx = 0; idx < 10; idx++)
                    args[idx] = &pointerToZero;

                auto argsIdx = 1;
                for (auto funcBase = L->base; funcBase != L->top; funcBase++) {
                    switch (funcBase->tt) {
                        case ::lua_Type::LUA_TSTRING:
                            args[argsIdx] = new std::string(funcBase->value.gc->ts.data, funcBase->value.gc->ts.len);
                            pendingFreeOperations.emplace_back(
                                    [argsIdx, &args]() { delete static_cast<std::string *>(args[argsIdx]); });
                            break;
                            // std::string is to be used.
                        case ::lua_Type::LUA_TNUMBER:
                            args[argsIdx] = new double(funcBase->value.n);
                            pendingFreeOperations.emplace_back(
                                    [argsIdx, &args]() { delete static_cast<double *>(args[argsIdx]); });
                            break;
                            // Allocating primitive on heap (?)
                        case ::lua_Type::LUA_TBOOLEAN:
                            args[argsIdx] = new int(funcBase->value.b);
                            pendingFreeOperations.emplace_back(
                                    [argsIdx, &args]() { delete static_cast<int *>(args[argsIdx]); });
                            break;
                        case ::lua_Type::LUA_TFUNCTION:
                        case ::lua_Type::LUA_TTHREAD:
                        case ::lua_Type::LUA_TTABLE:
                            args[argsIdx] = &pointerToZero;
                            break;

                        case LUA_TUSERDATA:
                        case LUA_TLIGHTUSERDATA:
                            args[argsIdx] = /*new void */ (const_cast<void *>(lua_topointer(L, argsIdx)));

                            if (Utilities::IsPointerValid(((std::uintptr_t *) args[argsIdx]) + 1)) {
                                auto refCount = *((std::uintptr_t *) args[argsIdx] + 1);
                                *((std::uintptr_t *) args[argsIdx] + 1) = refCount + 1;
                            }
                            auto ref = lua_ref(L, argsIdx);
                            pendingFreeOperations.emplace_back([&args, argsIdx, ref, L]() {
                                if (Utilities::IsPointerValid(((std::uintptr_t *) args[argsIdx]) + 1)) {
                                    auto refCount = *((std::uintptr_t *) args[argsIdx] + 1);
                                    *((std::uintptr_t *) args[argsIdx] + 1) = refCount - 1;
                                }
                                lua_unref(L, ref);
                            });
                            break;
                    }

                    argsIdx++;
                }

                // switch (possiblyValidArgumentsWithNoRegardsForMemorySafety) {
                //     case 0:
                //         pConnection->connection->call_Signal(pConnection->connection, args.data(), args.data() + 1,
                //                                              args.data() + 2, args.data() + 3, args.data() + 4,
                //                                              args.data() + 5, args.data() + 6, args.data() + 7,
                //                                              args.data() + 8, args.data() + 9);
                // case 1:

                // FIXED? Resolve random HEAP corruptions, possibly related to the way we currently handle pointers in
                // memory.
                try {
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                              std::format("Invoking Native Call; rcx: {}, rbx: {}", (void *) pConnection->m_pConnection,
                                          *args.data()));
                    pConnection->m_pConnection->call_Signal(pConnection->m_pConnection, *args.data(),
                                                            *(args.data() + 1), *(args.data() + 2), *(args.data() + 3),
                                                            *(args.data() + 4), *(args.data() + 5), *(args.data() + 6),
                                                            *(args.data() + 7), *(args.data() + 8), *(args.data() + 9));
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Freeing heap memory...");
                    for (const auto &function: pendingFreeOperations)
                        function();
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Call dispatched.");
                } catch (...) {
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Freeing heap memory...");
                    for (const auto &function: pendingFreeOperations)
                        function();
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Call dispatched.");
                    luaL_error(L, "what the fuck did you do?");
                }


                //    default:
                //        RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                //                  "Whopsies, more args required, ask dottik to hardcode like a pro ");
                //        ;
                //}
            }

            pConnection->m_pConnection->weak--;
            return 0;
        }

        static int GetForeignState(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            lua_pushboolean(L,
                            pConnection->m_bIsC ||
                                    lua_mainthread(L) !=
                                            lua_mainthread(pConnection->m_pConnection->pFunctionSlot->objRef->thread));

            return 1;
        }

        static int GetLuaConnection(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            lua_pushboolean(L, !pConnection->m_bIsC);
            return 1;
        }

        static int GetThread(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

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

        RbxStuConnection(ConnectionSlot *slot, lua_State *parentState, bool isC) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                      std::format("Creating RbxStuConnection on connection {}...", (void *) slot));
            this->m_pConnection = slot;
            if (!isC) {
                this->m_rpRealFunction.luaRef = slot->pFunctionSlot->objRef->objectId;
                this->m_rpThread.luaRef = slot->pFunctionSlot->objRef->thread_ref;
                this->m_bIsOtherLVM =
                        lua_mainthread(slot->pFunctionSlot->objRef->thread) != lua_mainthread(parentState);
            }


            this->m_ParentExecutionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()
                                                    ->GetTaskScheduler()
                                                    ->GetExecutionEngine(parentState);

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

        // Connect with a mock function, to get a connection instance

        lua_getfield(L, 1, "Connect");
        lua_pushvalue(L, 1);
        lua_pushcfunction(L, [](lua_State *) -> int { return 0; }, "");
        lua_call(L, 2, 1);

        const auto rawConnection = reinterpret_cast<ConnectionSlot *>(
                *reinterpret_cast<std::uintptr_t *>(*static_cast<std::uintptr_t *>(lua_touserdata(L, -1)) + 0x10));
        RbxStuLog(Warning, Anonymous, std::format("Root: {}", (void *) rawConnection));

        lua_getfield(L, -1, "Disconnect");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);

        lua_newtable(L);
        ConnectionSlot *slot = rawConnection;
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

    const char *Instances::GetLibraryName() { return "instances"; }

    bool Instances::PushToGlobals() { return true; }

    const luaL_Reg *Instances::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {{"getconnections", getconnections},

                                    {nullptr, nullptr}};

        return libreg;
    }

} // namespace RbxStu::StuLuau::Environment::UNC
