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

    struct FunctionSlot {
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
        int Connected;
        int unk0;
        void *unk1;
        ConnectionSlot *pNext;
        int unk2;
        int unk3;
        SlotInfo *pConnections;
        void *unk4;
        FunctionSlot *pFunctionSlot;
    };

    class RbxStuConnectionTagger final : public RbxStu::StuLuau::Environment::Interop::TaggedIdentifier {
    public:
        std::string GetTagName() override { return "RbxStuConnection"; }
    };

    class RbxStuConnection : public RbxStu::StuLuau::Environment::Interop::NativeObject<RbxStuConnectionTagger> {
        bool m_bUsable;
        ConnectionSlot *connection;
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
            lua_getref(L, pConnection->m_rpRealFunction.luaRef);
            return 1;
        }

        static int GetEnabled(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            lua_rawcheckstack(L, 1);
            lua_pushboolean(L, pConnection->connection->Connected &&
                                       pConnection->connection->pFunctionSlot->objRef->objectId ==
                                               pConnection->m_rpRealFunction.luaRef);
            return 1;
        }

        static int Enable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            // pConnection->connection->Connected = 1;
            pConnection->connection->pFunctionSlot->objRef->objectId = pConnection->m_rpRealFunction.luaRef;
            return 1;
        }

        static int Disable(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);
            // pConnection->connection->Connected = 0;
            pConnection->connection->pFunctionSlot->objRef->objectId = pConnection->m_stubRef.luaRef;
            return 1;
        }

        static int Disconnect(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            auto newUserdata = (ConnectionSlot *) lua_newuserdatatagged(
                    L, sizeof(void *), 4); // 4 is the right tag for RBXScriptConnection
            *newUserdata = *pConnection->connection;

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

            auto func = pConnection->m_rpRealFunction.GetReferencedObject(L);

            if (!func.has_value())
                luaL_error(L, "no function associated with RbxStuConnection object");

            if (!pConnection->connection->Connected ||
                pConnection->connection->pFunctionSlot->objRef->objectId != pConnection->m_rpRealFunction.luaRef) {
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
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

            task_defer(nL);
            return 0;
        }

        static int GetForeignState(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            lua_pushboolean(L, lua_mainthread(L) !=
                                       lua_mainthread(pConnection->connection->pFunctionSlot->objRef->thread));

            return 1;
        }

        static int GetLuaConnection(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            lua_pushboolean(L, true);
            return 1;
        }

        static int GetThread(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            lua_settop(L, 1);
            const auto pConnection = *static_cast<RbxStuConnection **>(lua_touserdata(L, 1));
            pConnection->ThrowIfUnusable(L);

            lua_getref(L, pConnection->m_rpThread.luaRef);

            return 1;
        }


    public:
        static int stub(lua_State *L) { return 0; }

        RbxStuConnection(ConnectionSlot *slot, lua_State *parentState) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Creating RbxStuConnection...");
            this->connection = slot;
            this->m_rpRealFunction.luaRef = slot->pFunctionSlot->objRef->objectId;
            this->m_rpThread.luaRef = slot->pFunctionSlot->objRef->thread_ref;

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

            lua_pushcclosure(parentState, stub, nullptr, 0);
            this->m_stubRef.luaRef = lua_ref(parentState, -1);
            lua_pop(parentState, 1);

            this->m_bUsable = true;
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
        while (slot != nullptr) {
            RbxStuLog(Warning, Anonymous, std::format("new userdata"));
            const auto connection = reinterpret_cast<RbxStuConnection **>(lua_newuserdata(L, sizeof(void *)));

            if (!Utilities::IsPointerValid(slot->pFunctionSlot) ||
                !Utilities::IsPointerValid(slot->pFunctionSlot->objRef) ||
                !Utilities::IsPointerValid(slot->pFunctionSlot->objRef->thread)) {
                RbxStuLog(Warning, Anonymous,
                          std::format("Skipped connection due to it being likely declared outside of Luau's context, "
                                      "and being a C connection, not supported. Connection: {}",
                                      (void *) slot));
                slot = slot->pNext;
                lua_pop(L, 1);
                continue;
            }

            ReferencedLuauObject<Closure *, lua_Type::LUA_TFUNCTION> func{slot->pFunctionSlot->objRef->objectId};
            auto obj = func.GetReferencedObject(L);

            if (!obj.has_value()) {
                RbxStuLog(Warning, Anonymous,
                          std::format("ObjectId is not valid, cannot fetch callback. Connection: {}", (void *) slot));
                slot = slot->pNext;
                lua_pop(L, 1);
                continue;
            }
            if (lua_mainthread(slot->pFunctionSlot->objRef->thread) != lua_mainthread(L)) {
                RbxStuLog(Warning, Anonymous,
                          std::format("Skipped connection due to it not being declared in the current LVM context. "
                                      "Connection: {}",
                                      (void *) slot));
                slot = slot->pNext;
                lua_pop(L, 1);
                continue;
            }
            RbxStuLog(Warning, Anonymous, std::format("creating rbxstu connection"));
            *connection = new RbxStuConnection{slot, L};

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
