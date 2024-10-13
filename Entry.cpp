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

#include <RTTIHook/RTTIScanner.h>
#include <Scanners/Luau.hpp>

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

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread,
              "-- Initializing RbxStu::Analysis::Disassembler...");
    RbxStu::Analysis::Disassembler::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Initializing RbxStu::Analysis::RTTI...");
    RbxStu::Analysis::RTTI::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Bootstrapping RbxStu V3!");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "-- Scanning for Luau...");
    RbxStu::Scanners::Luau::GetSingleton();

    void* threadAccessError = RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::threadAccessError);

    DWORD oldProtect;
    VirtualProtect(threadAccessError, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    byte replacementBytes[] = { 0xC3 };
    memcpy(threadAccessError, replacementBytes, 1);
    VirtualProtect(threadAccessError, 1, oldProtect, &oldProtect);

    RbxStuLog(RbxStu::LogType::Information, RbxStu::MainThread, "Patched RBX::Security::threadAccessError to return immediately");
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
