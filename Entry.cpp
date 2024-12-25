//
// Created by Dottik on 11/10/2024.
//
#include <StudioOffsets.h>
#include <cstdio>
#include <cstdlib>

#include <Analysis/Disassembler.hpp>
#include <Analysis/RTTI.hpp>
#include <Windows.h>
#include <libhat/Scanner.hpp>

#include "ExceptionHandler.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utilities.hpp"

#include <Scanners/Luau.hpp>
#include <Scanners/Rbx.hpp>
#include <Scheduling/Job/ExecutionEngineStepJob.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Analysis/StringSearcher.hpp"
#include "Analysis/XrefSearcher.hpp"
#include "Communication/PipeCommunication.hpp"
#include "Communication/WebsocketCommunication.hpp"
#include "FastFlags.hpp"
#include "Scheduling/Job/DataModelWatcherJob.hpp"
#include "Scheduling/Job/ImguiRenderJob.hpp"
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "Security.hpp"
#include "StuLuau/ExecutionEngine.hpp"

void EnableRobloxInternal() {
    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              "-- Initializing RbxStu::Analysis::XrefSearcher - Complex Cross-Reference Searcher...");
    auto xrefSearcher = RbxStu::Analysis::XrefSearcher::GetSingleton();
    xrefSearcher->BootstrapXrefsForModule(hat::process::get_process_module());

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread, "-- Scanning for Target to enable Roblox Internal...");

    const auto stringXrefSearcher = RbxStu::Analysis::StringSearcher::GetSingleton();

    const auto xrefs = stringXrefSearcher->FindStringXrefsInTarget(
            hat::process::get_process_module(), "Debugger can be attached to a script or module script");
    if (xrefs.empty()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                  "-- Cannot enable Roblox Internal mode! Missing xref to RBX::Scripting::ScriptDebugger::setScript!");
        return;
    }

    auto setScript = xrefs.front();

    auto rbxstuDisassembler = RbxStu::Analysis::Disassembler::GetSingleton();
    auto disassembly = rbxstuDisassembler->GetInstructions(
            reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(setScript) - 0x30), setScript, true);

    if (!disassembly.has_value()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                  "-- Cannot enable Roblox Internal mode! Failed to analyze function!");
        return;
    }

    auto chunk = std::move(disassembly.value());
    auto insn = chunk->GetInstructionWhichMatches("call", nullptr, true);

    if (!insn.has_value()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                  "-- Cannot enable Roblox Internal mode! Failed to analyze function!");
        return;
    }

    auto targetFunction = insn.value().detail->x86.operands->imm;

    auto disassemblyAgain = rbxstuDisassembler->GetInstructions(reinterpret_cast<void *>(targetFunction),
                                                                reinterpret_cast<void *>(targetFunction + 0x15), true);

    if (!disassemblyAgain.has_value()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                  "-- Cannot enable Roblox Internal mode! Failed to analyze function!");
        return;
    }

    for (const auto chunkAgain = std::move(disassemblyAgain.value()); const auto &insn: chunkAgain->GetInstructions()) {
        if (insn.id == ::x86_insn::X86_INS_CMP) {
            const auto dispFromRip = insn.detail->x86.operands->mem.disp;
            const auto dataRef = insn.address + dispFromRip + insn.size;
            *reinterpret_cast<bool *>(dataRef) = true; // Roblox overwrites this periodically for whatever reason.
        }
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread, "-- Roblox Internal mode has been enabled!");
}

void Entry() {
    AllocConsole();
    RbxStu::Logger::GetSingleton()->Initialize(true);

    RbxStu::ExceptionHandler::InstallHandler();
    const auto offsets = RbxStuOffsets::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              std::format("Initializing RbxStu V3 ({}) -- The Roblox Studio Modification Tool", RBXSTU_VERSION));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              std::format("-- RobloxStudioBeta.exe @ {}",
                          reinterpret_cast<void *>(GetModuleHandleA("RobloxStudioBeta.exe"))));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              std::format("-- RbxStu @ {}", reinterpret_cast<void *>(GetModuleHandleA(RBXSTU_DLL_NAME))));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Security...");
    RbxStu::Security::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Utilities...");
    RbxStu::Utilities::GetSingleton(); // GetSingleton calls Initialize.

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::FastFlagsManager...");
    const auto fastFlags = RbxStu::FastFlagsManager::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::Disassembler...");
    RbxStu::Analysis::Disassembler::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::RTTI...");
    RbxStu::Analysis::RTTI::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Bootstrapping RbxStu V3!");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Scanning for Luau...");
    RbxStu::Scanners::Luau::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Scanning for ROBLOX...");
    RbxStu::Scanners::RBX::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Hooking RBXCRASH...");
    RbxStu::ExceptionHandler::OverrideRBXCRASH();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing TaskSchedulerOrchestrator...");
    const auto orchestrator = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton();
    const auto scheduler = orchestrator->GetTaskScheduler();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::InitializeExecutionEngineJob>();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::ExecutionEngineStepJob>();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::DataModelWatcherJob>();
    scheduler->AddSchedulerJob<RbxStu::Scheduling::Jobs::ImguiRenderJob>();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing Websocket Communication...");
    RbxStu::Communication::WebsocketCommunication::GetSingleton();

    if (RbxStu::FastFlags::FFlagEnablePipeCommunication.GetValue()) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::MainThread, "Launching Pipe Communication [DEVELOPMENT ONLY]")

                std::thread(RbxStu::Communication::PipeCommunication::HandlePipe, "CommunicationPipe")
                        .detach();
    }

    if (RbxStu::FastFlags::FFlagIsRobloxInternalEnabled.GetValue()) {
        Sleep(100);

        RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                  "-- Enabling Roblox Internal -- This may take a bit...");

        auto tsk = std::async(std::launch::async, EnableRobloxInternal);

        if (tsk.wait_for(std::chrono::microseconds{0}) == std::future_status::timeout) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::MainThread,
                      "-- Waiting for Roblox Internal to finish initializing...");
            tsk.wait();
        }
    }

    // Test exec.
    /*    while (scheduler->GetExecutionEngine(RBX::DataModelType::DataModelType_PlayClient) == nullptr)
            _mm_pause();

        auto execEngine = scheduler->GetExecutionEngine(RBX::DataModelType::DataModelType_PlayClient);
        while (true) {
            if (execEngine.use_count() == 1)
                execEngine = scheduler->GetExecutionEngine(
                    RBX::DataModelType::DataModelType_PlayClient);

            if (execEngine != nullptr) {
                execEngine->ScheduleExecute(false, R"(
                    --
       closures.loadstring(httpget("https://gist.githubusercontent.com/SecondNewtonLaw/6f25ec379740705cbd98c62a8a1b3855/raw/a185f9f203bee60478b568e5cc2cd4ef3a3caecf/Stunc.lua"))()
                    --
       closures.loadstring(httpget("https://gitlab.com/sens3/nebunu/-/raw/main/HummingBird8's_sUNC_yes_i_moved_to_gitlab_because_my_github_acc_got_brickedd/sUNCm0m3n7.lua"))()
                )", RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor, true);
                break;
            } else {
                execEngine = scheduler->GetExecutionEngine(RBX::DataModelType::DataModelType_PlayClient);
            }
            Sleep(24000);
        }
        */
}

BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            std::thread(Entry).detach();
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
