//
// Created by Dottik on 12/11/2024.
//

#include "PipeCommunication.hpp"
#include <Windows.h>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <Logger.hpp>

#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Communication {
    void PipeCommunication::HandlePipe(const std::string &szPipeName) {
        const auto scheduler = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        DWORD Read{};
        char BufferSize[999999];
        std::string name = (R"(\\.\pipe\)" + szPipeName), Script{};
        HANDLE hPipe = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                        PIPE_WAIT,
                                        1, 9999999, 9999999, NMPWAIT_USE_DEFAULT_WAIT, nullptr);

        RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication",
                  std::format("Created Named ANSI Pipe -> '{}' for obtaining Luau code.", name));
        RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication", "Connecting to Named Pipe...");

        ConnectNamedPipe(hPipe, nullptr);

        RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication", "Connected! Awaiting Luau code!");

        while (hPipe != INVALID_HANDLE_VALUE && ReadFile(hPipe, BufferSize, sizeof(BufferSize) - 1, &Read,
                                                         nullptr)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication",
                          "RbxStu does not handle asynchronous IO requests. Pipe "
                          "requests must be synchronous (This should not happen)");
                _mm_pause();
                continue;
            }
            BufferSize[Read] = '\0';
            Script += BufferSize;

            if (scheduler->GetExecutionEngine(RBX::DataModelType_PlayClient) != nullptr) {
                RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication",
                          "Luau code scheduled for execution");

                scheduler->GetExecutionEngine(RBX::DataModelType_PlayClient)->ScheduleExecute(
                    false, Script, StuLuau::ExecutionSecurity::RobloxExecutor, true);
            } else {
                RbxStuLog(RbxStu::LogType::Information, "RbxStu::PipeCommunication",
                          "RbxStu::Scheduler is NOT initialized! Luau code NOT scheduled, please enter into a "
                          "game to be able to enqueue jobs into the Scheduler for execution!");
            }
            Script.clear();
        }
    }
}
