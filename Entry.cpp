//
// Created by Dottik on 11/10/2024.
//
#include <cstdlib>
#include <cstdio>
#include <StudioOffsets.h>

#include <Windows.h>

#include "ExceptionHandler.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utilities.hpp"
#include "Analysis/Disassembler.hpp"
#include "Analysis/RTTI.hpp"
#include "Analysis/Scanner.hpp"

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

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Utilities...");
    RbxStu::Utilities::GetSingleton(); // GetSingleton calls Initialize.

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::Disassembler...");
    RbxStu::Analysis::Disassembler::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::Scanner...");
    RbxStu::Analysis::Scanner::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::RTTI...");
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
