//
// Created by Pixeluted on 15/10/2024.
//

#include "WebsocketServer.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Scheduling/Job/ExecuteScriptJob.hpp"
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
        ix::WebSocketServer server(8080);
        server.setOnClientMessageCallback(
                [](const std::shared_ptr<ix::ConnectionState> &connectionState, ix::WebSocket &webSocket,
                   const ix::WebSocketMessagePtr &msg) {
                    if (msg->type == ix::WebSocketMessageType::Message) {
                        auto newJob = RbxStu::Scheduling::ExecuteJobRequest{};
                        newJob.scriptSource = _strdup(msg->str.c_str());
                        newJob.bGenerateNativeCode = false;

                        auto executeJobs = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->
                                           GetTaskScheduler()->GetJobs(
                                                   Scheduling::Jobs::AvailableJobs::ExecuteScriptJob);

                        for (const auto& job: executeJobs) {
                            const auto rightJob = std::dynamic_pointer_cast<Scheduling::Jobs::ExecuteScriptJob>(job);
                            rightJob->ScheduleExecuteJob(RBX::DataModelType_PlayClient, newJob);
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
