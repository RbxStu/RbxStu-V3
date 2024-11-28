//
// Created by Dottik on 27/10/2024.
//

#include "WebSocket.hpp"

#include <HttpStatus.hpp>
#include <Scheduling/TaskScheduler.hpp>

#include "lstate.h"

bool RbxStu::StuLuau::Environment::UNC::WebSocketInstance::CheckLifetime() const {
    return this->m_bIsUsable;
}

int RbxStu::StuLuau::Environment::UNC::WebSocketInstance::_Send(lua_State *L) {
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

void RbxStu::StuLuau::Environment::UNC::WebSocketInstance::CleanUp(lua_State *L) {
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
}

bool RbxStu::StuLuau::Environment::UNC::WebSocketInstance::IsOpen() {
    return this->CheckLifetime() && this->m_backingSocket.getReadyState() == ix::ReadyState::Open;
}

int RbxStu::StuLuau::Environment::UNC::WebSocketInstance::_Close(lua_State *L) {
    EnsureTaggedObjectOnStack(L);

    const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

    if (!ppSocket->CheckLifetime())
        luaL_argerror(L, 1, "the websocket is no longer valid.");

    if (!ppSocket->IsOpen())
        luaL_argerror(L, 1, "the websocket is not open for communication.");

    ppSocket->CleanUp(L);

    return 0;
}

int RbxStu::StuLuau::Environment::UNC::WebSocketInstance::_GetOnMessage(lua_State *L) {
    EnsureTaggedObjectOnStack(L);

    const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

    if (!ppSocket->CheckLifetime())
        luaL_argerror(L, 1, "the websocket is no longer valid.");

    lua_getref(L, ppSocket->m_dwOnMessageEventRef);
    lua_getfield(L, -1, "Event");
    return 1;
}

int RbxStu::StuLuau::Environment::UNC::WebSocketInstance::_GetOnClose(lua_State *L) {
    EnsureTaggedObjectOnStack(L);

    const auto ppSocket = *static_cast<WebSocketInstance **>(lua_touserdata(L, 1));

    if (!ppSocket->CheckLifetime())
        luaL_argerror(L, 1, "the websocket is no longer valid.");

    lua_getref(L, ppSocket->m_dwOnCloseEventRef);
    lua_getfield(L, -1, "Event");
    return 1;
}

void RbxStu::StuLuau::Environment::UNC::WebSocketInstance::OnMessageReceived(const ix::WebSocketMessagePtr &message) {
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
                lua_remove(L, 1);
                lua_pushlstring(L, msg.c_str(), msg.length());
                lua_pcall(L, 2, 0, 0);

                if (lua_gettop(L) > 1)
                    printf("%s", lua_tostring(L, -1));
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

RbxStu::StuLuau::Environment::UNC::WebSocketInstance::~WebSocketInstance() {
    this->CleanUp(nullptr);
}

RbxStu::StuLuau::Environment::UNC::WebSocketInstance::WebSocketInstance(lua_State *parentState) {
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

ix::WebSocketInitResult RbxStu::StuLuau::Environment::UNC::WebSocketInstance::InternalConnect(
    const std::string_view targetUrl, const std::chrono::seconds timeout) {
    this->m_backingSocket.setUrl(targetUrl.data());
    this->m_backingSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
        return this->OnMessageReceived(msg);
    });

    auto result = this->m_backingSocket.connect(static_cast<int>(timeout.count()));
    return result;
}

void RbxStu::StuLuau::Environment::UNC::WebSocketInstance::CompleteConnect() {
    this->m_backingSocket.start();
}

void RbxStu::StuLuau::Environment::UNC::WebSocketInstance::DeclareDependency() const {
    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Declaring Dependency to ExecutionEngine...");
    this->m_ParentExecutionEngine->AssociateObject(std::make_shared<AssociatedObject>([this] {
        delete this;
    }));
}


int RbxStu::StuLuau::Environment::UNC::WebSocket::connect(lua_State *L) {
    const auto connectUrl = luaL_checkstring(L, 1);
    const auto timeout = luaL_optinteger(L, 2, 30);

    if (strstr(connectUrl, "wss://") == nullptr && strstr(connectUrl, "ws://") == nullptr)
        luaL_argerror(L, 1, "invalid connect url, missing wss:// or ws:// prefix.");

    const auto nTagged = static_cast<WebSocketInstance **>(lua_newuserdata(L, sizeof(void *)));

    lua_newtable(L);
    lua_pushcclosure(L, WebSocketInstance::__index<WebSocketTagger>, nullptr, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, WebSocketInstance::__namecall<WebSocketTagger>, nullptr, 0);
    lua_setfield(L, -2, "__namecall");
    lua_pushstring(L, WebSocketTagger{}.GetTagName().c_str());
    lua_setfield(L, -2, "__type");

    lua_setmetatable(L, -2);

    *nTagged = new WebSocketInstance(L);

    const auto taskScheduler = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
    const auto executionEngine = taskScheduler->GetExecutionEngine(L);
    executionEngine->YieldThread(
        L, [nTagged, connectUrl, timeout](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
            const auto pWebsocket = (*nTagged);
            auto status = pWebsocket->InternalConnect(connectUrl, std::chrono::seconds(timeout));
            yieldRequest->fpCompletionCallback = [pWebsocket,status]() {
                if (!status.success) {
                    return YieldResult{
                        false, 0,
                        std::format("failed to connect to websocket. Reason: {}; Status Code: {} ({}).",
                                    status.errorStr,
                                    status.http_status, HttpStatus::ReasonPhrase(status.http_status))
                    };
                }

                pWebsocket->DeclareDependency();
                return YieldResult{true, 1, {}}; // WebSocket instance is on the stack.
            };
            pWebsocket->CompleteConnect();
            yieldRequest->bIsReady = true;
        }, true);

    return lua_yield(L, 1);
}

const luaL_Reg *RbxStu::StuLuau::Environment::UNC::WebSocket::GetFunctionRegistry() {
    static luaL_Reg flib[] = {
        {"connect", RbxStu::StuLuau::Environment::UNC::WebSocket::connect},
        {nullptr, nullptr}
    };

    return flib;
}

bool RbxStu::StuLuau::Environment::UNC::WebSocket::PushToGlobals() {
    return false;
}

const char *RbxStu::StuLuau::Environment::UNC::WebSocket::GetLibraryName() { return "WebSocket"; }
