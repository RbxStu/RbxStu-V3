//
// Created by Dottik on 27/10/2024.
//

#include "WebSocket.hpp"

#include <HttpStatus.hpp>

#include "lstate.h"

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
