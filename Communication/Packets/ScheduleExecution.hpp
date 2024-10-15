//
// Created by Pixeluted on 15/10/2024.
//

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Communication/PacketBase.hpp"
#include "Scheduling/Job/ExecuteScriptJob.hpp"
#include "ixwebsocket/IXHttpServer.h"

namespace RbxStu::Communication {
    class ScheduleExecution : public PacketBase {

        std::list<std::string_view> Register() override {
            return {"scriptSource", "datamodelType"};
        };

        bool ValidateData(nlohmann::json jsonData) override {
            if (jsonData["scriptSource"].is_string() && jsonData["datamodelType"].is_number_integer()) {
                const auto receivedDatamodelType = jsonData["datamodelType"].get<std::int32_t>();
                return receivedDatamodelType > -1 && receivedDatamodelType < 4;
            }
            return false;
        };

        void Callback(nlohmann::json jsonData) override {
            const auto receivedDatamodelType = jsonData["datamodelType"].get<RBX::DataModelType>();
            const auto scriptSource = jsonData["scriptSource"].get<std::string_view>();

            auto newExecutionJob = Scheduling::ExecuteJobRequest{};
            newExecutionJob.scriptSource = scriptSource;
            newExecutionJob.bGenerateNativeCode = false;

            const auto TaskScheduler = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
            const auto allExecuteJobs = TaskScheduler->GetJobs(Scheduling::Jobs::AvailableJobs::ExecuteScriptJob);

            for (const auto& job : allExecuteJobs) {
                const auto rightJob = std::dynamic_pointer_cast<Scheduling::Jobs::ExecuteScriptJob>(job);
                rightJob->ScheduleExecuteJob(receivedDatamodelType, newExecutionJob);
                Sleep(50); // Without this wait, code will never get executed :shrug: idk why
            }
        };
    };
}
