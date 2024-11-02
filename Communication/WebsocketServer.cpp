//
// Created by Pixeluted on 15/10/2024.
//

#include "WebsocketServer.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "PacketManager.hpp"
#include "ixwebsocket/IXWebSocketServer.h"

std::shared_ptr<RbxStu::Communication::WebsocketServer> RbxStu::Communication::WebsocketServer::pInstance;

std::shared_ptr<RbxStu::Communication::WebsocketServer> RbxStu::Communication::WebsocketServer::GetSingleton() {
    if (pInstance == nullptr)
        pInstance = std::make_shared<WebsocketServer>();

    if (!pInstance->IsInitialized())
        pInstance->Initialize();

    return pInstance;
}

bool RbxStu::Communication::WebsocketServer::IsInitialized() const {
    return this->m_bIsInitialized;
}


void RbxStu::Communication::WebsocketServer::Initialize() {
    if (this->IsInitialized())
        return;

    std::thread([] {
        ix::WebSocketServer server(7777);
        server.setOnClientMessageCallback(
                [](const std::shared_ptr<ix::ConnectionState> &connectionState, ix::WebSocket &webSocket,
                   const ix::WebSocketMessagePtr &msg) {
                    if (msg->type == ix::WebSocketMessageType::Message) {
                        const auto handleResults = PacketManager::HandleNewPacket(msg->str);
                        if (handleResults.has_value()) {
                            nlohmann::json errorJson;
                            errorJson["success"] = false;
                            errorJson["error"] = handleResults.value();

                            webSocket.sendText(errorJson.dump());
                        } else {
                            nlohmann::json successJson;
                            successJson["success"] = true;

                            webSocket.sendText(successJson.dump());
                        }
                    } else if (msg->type == ix::WebSocketMessageType::Open) {
                        RbxStuLog(RbxStu::LogType::Information, RbxStu::WebsocketServer,
                                  std::format("New connection opened"));
                    } else if (msg->type == ix::WebSocketMessageType::Close) {
                        RbxStuLog(RbxStu::LogType::Information, RbxStu::WebsocketServer,
                                  std::format("Some connection closed"));
                    }
                });

        const auto startupResults = server.listen();
        if (!startupResults.first) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::WebsocketServer,
                      std::format("Failed to start Websocket server! Error: {}", startupResults.second));
            return;
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::WebsocketServer, "Websocket server is listening at port 8080")

        server.start();
        server.wait();
    }).detach();

    this->m_bIsInitialized = true;
}
