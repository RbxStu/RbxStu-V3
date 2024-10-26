//
// Created by Dottik on 11/10/2024.
//
#include <cstdlib>
#include <cstdio>
#include <StudioOffsets.h>

#include <Windows.h>
#include <Analysis/Disassembler.hpp>
#include <Analysis/RTTI.hpp>
#include <libhat/Scanner.hpp>

#include "ExceptionHandler.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utilities.hpp"

#include <Scanners/Luau.hpp>
#include <Scanners/Rbx.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <Scheduling/Job/ResumeYieldedThreadsJob.hpp>

#include "Security.hpp"
#include "Analysis/XrefSearcher.hpp"
#include "Communication/WebsocketServer.hpp"
#include "Scheduling/Job/ExecuteScriptJob.hpp"
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"

void Entry() {
    AllocConsole();
    RbxStu::Logger::GetSingleton()->Initialize(true);

    RbxStu::ExceptionHandler::InstallHandler();
    const auto offsets = RbxStuOffsets::GetSingleton();

    printf(R"(
 *******   **               ********   **           **      **  ****
/**////** /**              **//////   /**          /**     /** */// *
/**   /** /**      **   **/**        ****** **   **/**     /**/    /*
/*******  /****** //** ** /*********///**/ /**  /**//**    **    ***
/**///**  /**///** //***  ////////**  /**  /**  /** //**  **    /// *
/**  //** /**  /**  **/**        /**  /**  /**  /**  //****    *   /*
/**   //**/******  ** //** ********   //** //******   //**    / ****
//     // /////   //   // ////////     //   //////     //      ////
)");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              "Initializing RbxStu V3 -- The Roblox Studio Modification Tool");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              std::format("-- RobloxStudioBeta.exe @ {}", reinterpret_cast<void *>(GetModuleHandleA(
                  "RobloxStudioBeta.exe"))));
    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              std::format("-- RbxStu @ {}", reinterpret_cast<void *>(GetModuleHandleA(
                  RBXSTU_DLL_NAME))));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Security...");
    RbxStu::Security::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Utilities...");
    RbxStu::Utilities::GetSingleton(); // GetSingleton calls Initialize.

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              "-- Initializing RbxStu::Analysis::Disassembler...");
    RbxStu::Analysis::Disassembler::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::RTTI...");
    RbxStu::Analysis::RTTI::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              "-- Initializing RbxStu::Analysis::XrefSearcher - Complex Cross-Reference Searcher...");
    RbxStu::Analysis::XrefSearcher::GetSingleton()->BootstrapXrefsForModule(hat::process::get_process_module());

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Bootstrapping RbxStu V3!");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Scanning for Luau...");
    RbxStu::Scanners::Luau::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Scanning for ROBLOX...");
    RbxStu::Scanners::RBX::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing TaskSchedulerOrchestrator...");
    const auto orchestrator = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton();
    const auto scheduler = orchestrator->GetTaskScheduler();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::InitializeExecutionEngineJob>();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::ExecuteScriptJob>();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::ResumeYieldedThreadsJob>();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing WebsocketServer...");
    RbxStu::Communication::WebsocketServer::GetSingleton();

    while (scheduler->GetExecutionEngine(RBX::DataModelType::DataModelType_MainMenuStandalone) == nullptr)
        _mm_pause();

    scheduler->GetExecutionEngine(RBX::DataModelType::DataModelType_MainMenuStandalone)->ScheduleExecute(false, R"(
    print("Hello, world!")
    print(getfenv(0))
    print(getgenv())
    print(getrenv())
)", RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);
}

BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0x1000, reinterpret_cast<LPTHREAD_START_ROUTINE>(Entry), nullptr, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            if (lpvReserved != nullptr) {
                break; // do not do cleanup if process termination scenario
            }
            break;
        default:
            break;
    }
    return true;
}
