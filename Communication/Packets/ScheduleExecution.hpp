//
// Created by Pixeluted on 15/10/2024.
//
#pragma once

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Communication/PacketBase.hpp"
#include "Scheduling/Job/ExecuteScriptJob.hpp"
#include "ixwebsocket/IXHttpServer.h"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Communication {
    class ScheduleExecution final : public PacketBase {
        std::list<std::string_view> GetRequiredFields() override {
            return {"scriptSource", "datamodelType", "generateNativeCode"};
        };

        bool ValidateData(const nlohmann::json &jsonData) override {
            if (jsonData["scriptSource"].is_string() && jsonData["datamodelType"].is_number_integer() && jsonData[
                    "generateNativeCode"].is_boolean()) {
                const auto receivedDatamodelType = jsonData["datamodelType"].get<std::int32_t>();
                return receivedDatamodelType > -1 && receivedDatamodelType < 4;
            }
            return false;
        };

        void Callback(const nlohmann::json &jsonData) override {
            const auto receivedDatamodelType = jsonData["datamodelType"].get<RBX::DataModelType>();
            const auto scriptSource = jsonData["scriptSource"].get<std::string_view>();
            const auto generateNativeCode = jsonData["generateNativeCode"].get<bool>();

            const auto TaskScheduler = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();

            if (const auto executionEngine = TaskScheduler->GetExecutionEngine(receivedDatamodelType);
                executionEngine != nullptr) {
                executionEngine->ScheduleExecute(generateNativeCode, scriptSource,
                                                 RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);
                /*
                 *  Without this wait, it will not get scheduled,
                 *  Honestly, I have no clue why
                 * -- Idk until i suffer i wont try pixel :> (written by yours trully, dottik)
                 */
                // Sleep(50);
            }
        };
    };
}
