//
// Created by Pixeluted on 15/10/2024.
//

#include "WebsocketCommunication.hpp"

#include "FastFlags.hpp"
#include "PacketManager.hpp"
#include "ixwebsocket/IXWebSocketServer.h"

std::shared_ptr<RbxStu::Communication::WebsocketCommunication> RbxStu::Communication::WebsocketCommunication::pInstance;

std::shared_ptr<RbxStu::Communication::WebsocketCommunication>
RbxStu::Communication::WebsocketCommunication::GetSingleton() {
    if (pInstance == nullptr)
        pInstance = std::make_shared<WebsocketCommunication>();

    if (!pInstance->IsInitialized())
        pInstance->Initialize();

    return pInstance;
}

bool RbxStu::Communication::WebsocketCommunication::IsInitialized() const { return this->m_bIsInitialized; }


void RbxStu::Communication::WebsocketCommunication::Initialize() {
    if (this->IsInitialized())
        return;

    const auto fastFlags = FastFlags::GetSingleton();
    this->websocketPort = fastFlags->GetOptionalFastFlagValue<int>("IFlagWebsocketPort", 7777);

    std::thread([this, fastFlags] {
        if (fastFlags->GetOptionalFastFlagValue<bool>("FFLagUseWebsocketServer", false)) {
            ix::WebSocketServer server(this->websocketPort);
            server.setOnClientMessageCallback([](const std::shared_ptr<ix::ConnectionState> &connectionState, ix::WebSocket &webSocket,
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
                    RbxStuLog(LogType::Information, RbxStu::WebsocketCommunication, "A client has established connection to our server!");
                } else if (msg->type == ix::WebSocketMessageType::Close) {
                    RbxStuLog(LogType::Information, RbxStu::WebsocketCommunication, "A client has disconnected from our server!");
                }
            });

            const auto startupResults = server.listen();
            if (!startupResults.first) {
                RbxStuLog(RbxStu::LogType::Error, RbxStu::WebsocketCommunication,
                          std::format("Failed to start Websocket server! Error: {}", startupResults.second));
                return;
            }

            RbxStuLog(RbxStu::LogType::Information, RbxStu::WebsocketCommunication,
                      std::format("Websocket server is listening at port {}", this->websocketPort));

            server.start();
            server.wait();
        } else {
            ix::WebSocket websocketClient;
            websocketClient.setOnMessageCallback([&websocketClient](const ix::WebSocketMessagePtr &msg) {
                if (msg->type == ix::WebSocketMessageType::Message) {
                    const auto handleResults = PacketManager::HandleNewPacket(msg->str);
                    if (handleResults.has_value()) {
                        nlohmann::json errorJson;
                        errorJson["success"] = false;
                        errorJson["error"] = handleResults.value();

                        websocketClient.sendText(errorJson.dump());
                    } else {
                        nlohmann::json successJson;
                        successJson["success"] = true;

                        websocketClient.sendText(successJson.dump());
                    }
                } else if (msg->type == ix::WebSocketMessageType::Close) {
                    RbxStuLog(LogType::Information, RbxStu::WebsocketCommunication,
                              "Connection to WebSocket server has closed! We will be re-attempting shortly!");
                } else if (msg->type == ix::WebSocketMessageType::Open) {
                    RbxStuLog(LogType::Information, RbxStu::WebsocketCommunication,
                              "Successfully connected to WebSocket server!");
                }
            });

            const std::string websocketUrl = std::format("ws://localhost:{}", this->websocketPort);
            websocketClient.setUrl(websocketUrl);
            websocketClient.enableAutomaticReconnection();
            websocketClient.setPingInterval(45);
            websocketClient.setMaxWaitBetweenReconnectionRetries(1000);

            RbxStuLog(LogType::Information, RbxStu::WebsocketCommunication,
                      std::format("Now trying to connect to WebSocket server at {}", websocketUrl));

            if (!websocketClient.connect(5).success) {
                RbxStuLog(LogType::Error, RbxStu::WebsocketCommunication,
                          "Failed to connect to WebSocket server! We will now be retrying in 1 second interval!");

                while (true) {
                    if (websocketClient.connect(5).success) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            websocketClient.run();
        }
    }).detach();

    this->m_bIsInitialized = true;
}
