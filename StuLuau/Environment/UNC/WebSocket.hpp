//
// Created by Dottik on 27/10/2024.
//

#pragma once
#include <Utilities.hpp>
#include <cmake-build-builddebug/Dependencies/cryptopp-cmake/cryptopp/rng.h>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lapi.h"
#include "lstate.h"
#include "ixwebsocket/IXWebSocket.h"
#include "Roblox/TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::Interop {
    struct FunctionInformation {
        int argumentCount;
        lua_CFunction fpFunction;
    };

    struct PropertyInformation {
        lua_CFunction fpGetter; // getter will only get in first place the pointer to the object.
        int setterArgc;
        std::optional<lua_CFunction> fpSetter;
    };

    class TaggedIdentifier abstract {
    public:
        virtual std::string GetTagName() {
            return "TaggedIdentifier";
        }
    };

    template<Concepts::TypeConstraint<TaggedIdentifier> TaggerContainer>
    class NativeObject abstract {
    protected:
        std::map<std::string, FunctionInformation> m_functionMap;
        std::map<std::string, PropertyInformation> m_propertyMap;

        static void EnsureTaggedObjectOnStack(lua_State *L) {
            luaL_checktype(L, 1, LUA_TUSERDATA);
            TaggerContainer x{};

            const TValue *pObj = luaA_toobject(L, 1);

            if (strstr(luaT_objtypename(L, pObj), x.GetTagName().c_str()) == nullptr)
                luaL_typeerror(L, 1, std::format("userdata<{}>", x.GetTagName()).c_str());
        }

    public:
        template<Concepts::TypeConstraint<TaggedIdentifier> tagContainer>
        static int __index(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            luaL_checktype(L, 2, LUA_TSTRING);
            tagContainer x{};

            const auto ppNative = *static_cast<NativeObject **>(lua_touserdata(L, 1));

            const auto index = lua_tostring(L, 2);

            if (ppNative->m_functionMap.contains(index)) {
                const auto funcInfo = ppNative->m_functionMap.at(index);
                lua_pushcfunction(L, funcInfo.fpFunction, nullptr);
                return 1;
            } else if (ppNative->m_propertyMap.contains(index)) {
                const auto propInfo = ppNative->m_propertyMap.at(index);
                lua_pushvalue(L, 1);
                lua_remove(L, 2);
                lua_remove(L, 1);
                lua_pushcfunction(L, propInfo.fpGetter, nullptr);
                lua_insert(L, 1);
                lua_call(L, 1, LUA_MULTRET);
                return lua_gettop(L);
            }

            luaL_error(L, std::format( "invalid index into userdata<{}>", x.GetTagName()).c_str());
        }

        template<Concepts::TypeConstraint<TaggedIdentifier> tagContainer>
        static int __namecall(lua_State *L) {
            EnsureTaggedObjectOnStack(L);
            const auto ppNative = *static_cast<NativeObject **>(lua_touserdata(L, 1));
            tagContainer x{};

            const auto funcName = std::string_view(L->namecall->data);

            const auto func = std::string(funcName.data());
            if (ppNative->m_functionMap.contains(func)) {
                const auto funcInfo = ppNative->m_functionMap.at(func);
                lua_pushcclosure(L, funcInfo.fpFunction, nullptr, 0);
                lua_insert(L, 1);
                lua_call(L, funcInfo.argumentCount, LUA_MULTRET);
                return lua_gettop(L);
            }

            luaL_error(L, std::format("invalid namecall into userdata<{}>", x.GetTagName()).c_str());
        }
    };
}

const int WEBSOCKET_MAX_PAYLOAD_SENDSIZE = 0x100000;
const int WEBSOCKET_MAX_PAYLOAD_RECVSIZE = 0x100000;

namespace RbxStu::StuLuau::Environment::UNC {
    class WebSocketTagger final : public RbxStu::StuLuau::Environment::Interop::TaggedIdentifier {
    public:
        std::string GetTagName() override {
            return "WebSocketInstance";
        };
    };

    class WebSocketInstance final : public RbxStu::StuLuau::Environment::Interop::NativeObject<WebSocketTagger> {
        std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> m_ParentExecutionEngine;
        ix::WebSocket m_backingSocket;
        bool m_bIsConnected;
        bool m_bIsUsable;

        int m_dwOnMessageEventRef = LUA_REFNIL;
        int m_dwOnCloseEventRef = LUA_REFNIL;

        bool CheckLifetime() {
            return this->m_bIsUsable;
        }

        static int _Send(lua_State *L) {
            EnsureTaggedObjectOnStack(L);

            luaL_checktype(L, 2, LUA_TSTRING);

            const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

            if (!ppSocket->CheckLifetime())
                luaL_argerror(L, 1, "the websocket is no longer valid.");

            if (!ppSocket->IsOpen())
                luaL_argerror(L, 1, "the websocket is not open for communication.");

            const auto pPayload = &(L->top - 1)->value.gc->ts;

            if (pPayload->len >= WEBSOCKET_MAX_PAYLOAD_SENDSIZE)
                luaL_argerror(
                L, 2, std::format(
                    "the size of the payload to send to the remote server is too big! Payload Size: {}; Maximum payload size: {}"
                    , pPayload->len, WEBSOCKET_MAX_PAYLOAD_SENDSIZE).c_str());

            ppSocket->m_backingSocket.send(pPayload->data, pPayload->len);

            return 0;
        }

        void CleanUp(lua_State *L) {
            if (this->m_backingSocket.getReadyState() != ix::ReadyState::Closed) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Closing WebSocket...");
                this->m_backingSocket.close(ix::WebSocketCloseConstants::kNormalClosureCode,
                                            ix::WebSocketCloseConstants::kNormalClosureMessage);
            }
            // nullptr == L => ~WebSocketInstance is our caller.
            if (L != nullptr) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Signaling Luau OnClose...");

                lua_getref(L, this->m_dwOnCloseEventRef);
                if (lua_type(L, -1) == LUA_TNIL) {
                    RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                              "WARNING! Failed to notify of message event to Luau! The reference to OnMessage was lost?");
                    return;
                }

                lua_getfield(L, -1, "Fire");
                lua_pushvalue(L, -2);
                lua_pcall(L, 1, 0, 0);

                lua_unref(L, this->m_dwOnMessageEventRef);
                this->m_dwOnMessageEventRef = LUA_REFNIL;
                lua_unref(L, this->m_dwOnCloseEventRef);
                this->m_dwOnCloseEventRef = LUA_REFNIL;
            }

            this->m_bIsUsable = false;
        };

        bool IsOpen() {
            return this->CheckLifetime() && this->m_backingSocket.getReadyState() == ix::ReadyState::Open;
        };

        static int _Close(lua_State *L) {
            EnsureTaggedObjectOnStack(L);

            const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

            if (!ppSocket->CheckLifetime())
                luaL_argerror(L, 1, "the websocket is no longer valid.");

            if (!ppSocket->IsOpen())
                luaL_argerror(L, 1, "the websocket is not open for communication.");

            ppSocket->CleanUp(L);

            return 0;
        }

        static int _GetOnMessage(lua_State *L) {
            EnsureTaggedObjectOnStack(L);

            const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

            if (!ppSocket->CheckLifetime())
                luaL_argerror(L, 1, "the websocket is no longer valid.");

            lua_getref(L, ppSocket->m_dwOnMessageEventRef);
            lua_getfield(L, -1, "Event");
            return 1;
        }

        static int _GetOnClose(lua_State *L) {
            EnsureTaggedObjectOnStack(L);

            const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

            if (!ppSocket->CheckLifetime())
                luaL_argerror(L, 1, "the websocket is no longer valid.");

            lua_getref(L, ppSocket->m_dwOnCloseEventRef);
            lua_getfield(L, -1, "Event");
            return 1;
        }

        void OnMessageReceived(const ix::WebSocketMessagePtr &message) {
            auto msg = std::string(message->str.begin(), message->str.end());
            switch (message->type) {
                case ix::WebSocketMessageType::Message:
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                              "WebSocket Message received.");

                    if (message->str.size() > WEBSOCKET_MAX_PAYLOAD_RECVSIZE) {
                        RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                                  std::format(
                                      "WARNING! Dropped OnMessage for a WebSocket, the message was too big! Message Size: {}; Maximum payload size: {}"
                                      , message->str.size(), WEBSOCKET_MAX_PAYLOAD_RECVSIZE));
                        return;
                    }


                    this->m_ParentExecutionEngine->DispatchSynchronized([this, msg](lua_State *L) {
                        lua_getref(L, this->m_dwOnMessageEventRef);
                        if (lua_type(L, -1) == LUA_TNIL) {
                            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                                      "WARNING! Failed to notify of message event to Luau! The reference to OnMessage was lost?");
                            return;
                        }

                        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                                  std::format( "WebSocket message received! Msg: '{}'", msg));

                        lua_getfield(L, -1, "Fire");
                        lua_pushvalue(L, -2);
                        lua_pushlstring(L, msg.c_str(), msg.length());
                        lua_pcall(L, 2, 0, 0);
                    });

                    break;
                case ix::WebSocketMessageType::Open:
                    break;
                case ix::WebSocketMessageType::Close:
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                              "WebSocket connection was closed.");
                    this->m_ParentExecutionEngine->DispatchSynchronized([this](lua_State *L) {
                        lua_getref(L, this->m_dwOnCloseEventRef);
                        if (lua_type(L, -1) == LUA_TNIL) {
                            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                                      "WARNING! Failed to notify of close event to Luau! The reference to OnClose was lost?");
                            return;
                        }

                        lua_getfield(L, -1, "Fire");
                        lua_pushvalue(L, -2);
                        lua_pcall(L, 1, 0, 0);
                    });
                    break;
                case ix::WebSocketMessageType::Error:
                    break;
                case ix::WebSocketMessageType::Ping:
                    break;
                case ix::WebSocketMessageType::Pong:
                    break;
                case ix::WebSocketMessageType::Fragment:
                    break;
            }
        }

    public:
        ~WebSocketInstance() {
            this->CleanUp(nullptr);
        }

        explicit WebSocketInstance(lua_State *parentState) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Creating WebSocket...");

            this->m_ParentExecutionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->
                    GetTaskScheduler()->GetExecutionEngine(parentState);
            this->m_bIsUsable = true;
            this->m_bIsConnected = false;
            this->m_functionMap = {
                {"Send", {2, _Send}},
                {"Close", {1, _Close}}
            };

            this->m_propertyMap = {
                {"OnMessage", {_GetOnMessage, 1, {}}},
                {"OnClose", {_GetOnClose, 1, {}}}
            };

            Utilities::createInstance(parentState, "BindableEvent", "onMessage");
            this->m_dwOnMessageEventRef = lua_ref(parentState, -1);
            lua_pop(parentState, 1);

            Utilities::createInstance(parentState, "BindableEvent", "onClose");
            this->m_dwOnCloseEventRef = lua_ref(parentState, -1);
            lua_pop(parentState, 1);
        }

        ix::WebSocketInitResult InternalConnect(const std::string_view targetUrl, const std::chrono::seconds timeout) {
            this->m_backingSocket.setUrl(targetUrl.data());
            this->m_backingSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
                return this->OnMessageReceived(msg);
            });

            auto result = this->m_backingSocket.connect(static_cast<int>(timeout.count()));
            this->m_backingSocket.start();
            return result;
        }

        void DeclareDependency() const {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Declaring Dependency to ExecutionEngine...");
            this->m_ParentExecutionEngine->AssociateObject(std::make_shared<AssociatedObject>([this] {
                delete this;
            }));
        }
    };

    class WebSocket final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int connect(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;;
    };
}
