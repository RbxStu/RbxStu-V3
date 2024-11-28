//
// Created by Dottik on 27/10/2024.
//

#pragma once
#include "Utilities.hpp"
#include "Scheduling/TaskSchedulerOrchestrator.hpp"

#include <lapi.h>
#include <lstate.h>
#include <ixwebsocket/IXWebSocket.h>

#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/Environment/Library.hpp"
#include "StuLuau/Interop/NativeObject.hpp"

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

        bool CheckLifetime() const;

        static int _Send(lua_State *L);

        void CleanUp(lua_State *L);;

        bool IsOpen();;

        static int _Close(lua_State *L);

        static int _GetOnMessage(lua_State *L);

        static int _GetOnClose(lua_State *L);

        void OnMessageReceived(const ix::WebSocketMessagePtr &message);

    public:
        ~WebSocketInstance();

        explicit WebSocketInstance(lua_State *parentState);

        ix::WebSocketInitResult InternalConnect(const std::string_view targetUrl, const std::chrono::seconds timeout);

        void CompleteConnect();

        void DeclareDependency() const;
    };

    class WebSocket final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int connect(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
}
