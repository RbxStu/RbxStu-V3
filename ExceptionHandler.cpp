//
// Created by Dottik on 11/10/2024.
//


#include "ExceptionHandler.hpp"

#include <MinHook.h>
#include <Psapi.h>
#include <Utilities.hpp>
#include <Windows.h>
#include <minidumpapiset.h>
#include <print>
#include <unordered_set>

#include "Analysis/Disassembler.hpp"
#include "FastFlags.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

std::string RbxStu::ExceptionHandler::LookupModuleByAddress(const void *address) {
    const HANDLE hProcess = GetCurrentProcess();
    DWORD Needed;

    if (HMODULE phModules[1024]; EnumProcessModules(hProcess, phModules, sizeof(phModules), &Needed)) {
        for (unsigned int i = 0; i < Needed / sizeof(HMODULE); i++) {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(hProcess, phModules[i], &moduleInfo, sizeof(moduleInfo))) {
                void *moduleBase = moduleInfo.lpBaseOfDll;
                if (const std::size_t moduleSize = moduleInfo.SizeOfImage;
                    address >= static_cast<BYTE *>(moduleBase) &&
                    address < static_cast<BYTE *>(moduleBase) + moduleSize) {
                    if (WCHAR moduleName[MAX_PATH];
                        GetModuleFileNameExW(hProcess, phModules[i], moduleName, sizeof(moduleName) / sizeof(WCHAR))) {
                        const std::filesystem::path fullPath(moduleName);
                        const std::wstring fileName = fullPath.filename();
                        std::string moduleNameConverted = RbxStu::Utilities::WcharToString(fileName.c_str());

                        return moduleNameConverted;
                    }
                }
            }
        }
    }

    char name[MAX_PATH];

    GetModuleFileNameA(GetModuleHandle(nullptr), name, MAX_PATH);

    return name;
}

std::optional<std::pair<std::string, void *>> RbxStu::ExceptionHandler::LookupAddress(const void *address) {
    HMODULE phModules[1024];
    HANDLE ourProcessHandle = GetCurrentProcess();
    DWORD Needed;

    if (EnumProcessModules(ourProcessHandle, phModules, sizeof(phModules), &Needed)) {
        for (unsigned int i = 0; i < Needed / sizeof(HMODULE); i++) {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(ourProcessHandle, phModules[i], &moduleInfo, sizeof(moduleInfo))) {
                void *moduleBase = moduleInfo.lpBaseOfDll;
                std::size_t moduleSize = moduleInfo.SizeOfImage;
                if (address >= static_cast<BYTE *>(moduleBase) &&
                    address < static_cast<BYTE *>(moduleBase) + moduleSize) {
                    WCHAR moduleName[MAX_PATH];
                    if (GetModuleFileNameExW(ourProcessHandle, phModules[i], moduleName,
                                             sizeof(moduleName) / sizeof(WCHAR))) {
                        const std::filesystem::path fullPath(moduleName);
                        const std::wstring fileName = fullPath.filename();
                        std::string moduleNameConverted = RbxStu::Utilities::WcharToString(fileName.c_str());

                        return std::pair<std::string, void *>({moduleNameConverted, moduleBase});
                    }
                }
            }
        }
    }

    return {};
}

static void *pOriginalRbxCrash{};

void RbxStu::ExceptionHandler::RBXCRASH(const char *crashType, const char *crashDescription) {
    RbxStuLog(RbxStu::LogType::Error, RbxStu::RBXCRASH,
              "-- RbxStu V3 -- RBXCRASH [CAUSED BY ROBLOX CODE] HAS BEEN INVOKED --");

    if (crashType == nullptr)
        crashType = "Crash Type Not Specified";

    if (crashDescription == nullptr)
        crashDescription = "Crash Not Described";

    RbxStuLog(RbxStu::LogType::Error, RbxStu::RBXCRASH,
              std::format("Crash Type: {}; Crash Description: {}; Proceeding crash analysis...", crashType,
                          crashDescription));

    /*
     *  Update this with SEH code, i love SEH!
     */

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::RBXCRASH, "-- Obtaining callstack...");

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::RBXCRASH, "-- Walking Stack...");

    const auto disassembler = RbxStu::Analysis::Disassembler::GetSingleton();

    void *stack[256];
    const auto frameCount = RtlCaptureStackBackTrace(0, 255, stack, nullptr);
    const auto callstack = std::vector<void *>(stack, stack + frameCount);

    // auto hasFramePassed = false;
    for (auto call: callstack) {
        const auto functionStart = disassembler->GetFunctionStart(call);
        std::string belongsTo = "Unknown Origin";

        void *targetModuleBaseAddress = nullptr;
        auto lookupResults = RbxStu::ExceptionHandler::LookupAddress(call);
        if (lookupResults.has_value()) {
            belongsTo = lookupResults.value().first;
            targetModuleBaseAddress = lookupResults.value().second;
        }

        auto message = std::format(
                "-> sub_{:X}() + {}, belonging to {}", reinterpret_cast<std::intptr_t>(functionStart),
                reinterpret_cast<void *>(
                        max(reinterpret_cast<std::intptr_t>(functionStart) - reinterpret_cast<std ::intptr_t>(call),
                            reinterpret_cast<std::intptr_t>(call) - reinterpret_cast<std::intptr_t>(functionStart))),
                belongsTo);

        if (strcmp(belongsTo.c_str(), "Unknown Origin") == 0) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
                      std::format("{}; Cannot trace back to any module :(", message));
        } else {
#if RBXSTU_REBASE_STACKTRACE_ON_SEH
            const auto base = 0x180000000;
#else
            const auto base = 0x0;
#endif

            // if (hasFramePassed) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
                      std::format("{}; Rebased to Module Base: {}", message,
                                  reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(call) -
                                                           reinterpret_cast<std::uintptr_t>(targetModuleBaseAddress) +
                                                           base)));
        }
    }

    // Analyse step.

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Analysing the Stack Trace for common exception patterns...");

    /*
     *  Stack Overflow Analysis (Flags possibly-recursing function)
     */
    while (true) {
        std::unordered_set<void *> nestedCalls{};
        nestedCalls.reserve(frameCount / 2);
        for (const auto call: callstack) {
            const auto functionStart = disassembler->GetFunctionStart(call);
            auto count = 0;

            for (const auto callAgain: callstack) {
                if (callAgain == call) {
                    count++;
                }
            }

            if (count > 3 && !nestedCalls.contains(call)) {
                nestedCalls.insert(call);
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandlerAnalysis,
                          std::format("sub_{}() + {} is present many times on the call-stack (Possibly a Recursive "
                                      "sub-routine)",
                                      functionStart,
                                      reinterpret_cast<void *>(
                                              max(reinterpret_cast<std::intptr_t>(functionStart) -
                                                          reinterpret_cast<std ::intptr_t>(call),
                                                  reinterpret_cast<std::intptr_t>(call) -
                                                          reinterpret_cast<std::intptr_t>(functionStart)))));
            }
        }
        break;
    }

    /*
     *  CALL into nullptr, cannot be traced :(
     */
    while (true) {
        if (callstack.size() < 5) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandlerAnalysis,
                      "-- Possibly executing a call into a nullptr (Possibly a DEP (Data-Execution-Protection) "
                      "violation?)");
        }

        break;
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler, "-- Stack Trace analysis complete.");

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler, "-- RbxStu V3 RBXCRASH -- End");

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler, "-- RbxStu V3 -- Emitting Debugging DUMP.");

    EXCEPTION_POINTERS exceptionPointers{};

    CONTEXT ctx{};
    RtlCaptureContext(&ctx);
    exceptionPointers.ContextRecord = &ctx;

    EXCEPTION_RECORD exRecord;
    exRecord.ExceptionAddress = exceptionPointers.ExceptionRecord = &exRecord;

    RbxStu::ExceptionHandler::CreateDump(&exceptionPointers);

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler, "-- RbxStu V3 -- Emitting Debugging DUMP.");

    MessageBoxA(nullptr, "Unhandled exception caught! Check console!", "Error", MB_OK);
}

long RbxStu::ExceptionHandler::UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers) {
    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- Begin");

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              std::format("SEH exception code: '0x{:X}'", pExceptionPointers->ExceptionRecord->ExceptionCode));

    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &ex) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  std::format("C++ exception message: '{}'", ex.what()));
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler, "-- Obtaining callstack...");

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler, "-- Walking Stack...");

    const auto disassembler = RbxStu::Analysis::Disassembler::GetSingleton();

    void *stack[256];
    const auto frameCount = RtlCaptureStackBackTrace(0, 255, stack, nullptr);
    const auto callstack = std::vector<void *>(stack, stack + frameCount);

    // auto hasFramePassed = false;
    for (auto call: callstack) {
        const auto functionStart = disassembler->GetFunctionStart(call);
        std::string belongsTo = "Unknown Origin";

        void *targetModuleBaseAddress = nullptr;
        auto lookupResults = RbxStu::ExceptionHandler::LookupAddress(call);
        if (lookupResults.has_value()) {
            belongsTo = lookupResults.value().first;
            targetModuleBaseAddress = lookupResults.value().second;
        }

        auto message = std::format(
                "-> sub_{:X}() + {}, belonging to {}", reinterpret_cast<std::intptr_t>(functionStart),
                reinterpret_cast<void *>(
                        max(reinterpret_cast<std::intptr_t>(functionStart) - reinterpret_cast<std ::intptr_t>(call),
                            reinterpret_cast<std::intptr_t>(call) - reinterpret_cast<std::intptr_t>(functionStart))),
                belongsTo);

        if (strcmp(belongsTo.c_str(), "Unknown Origin") == 0) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
                      std::format("{}; Cannot trace back to any module :(", message));
        } else {
#if RBXSTU_REBASE_STACKTRACE_ON_SEH
            const auto base = 0x180000000;
#else
            const auto base = 0x0;
#endif

            // if (hasFramePassed) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
                      std::format("{}; Rebased to Module Base: {}", message,
                                  reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(call) -
                                                           reinterpret_cast<std::uintptr_t>(targetModuleBaseAddress) +
                                                           base)));
            // } else {
            //     RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
            //               std::format(
            //                   "{}; Rebased to Module Base: {} --------> EXCEPTION HANDLER; FUNCTIONS BEFORE ntdll.dll
            //                   and KERNELBASE.dll ARE THE TRUE STACK TRACE!" , message, reinterpret_cast<void*>(
            //                       reinterpret_cast<
            //                       std::uintptr_t>(call) -
            //                       reinterpret_cast<std::uintptr_t>(targetModuleBaseAddress) + base)));
            //     hasFramePassed = true;
            // }
        }
    }

    // Analyse step.

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Analysing the Stack Trace for common exception patterns...");

    /*
     *  Stack Overflow Analysis (Flags possibly-recursing function)
     */
    while (true) {
        std::unordered_set<void *> nestedCalls{};
        nestedCalls.reserve(frameCount / 2);
        for (const auto call: callstack) {
            const auto functionStart = disassembler->GetFunctionStart(call);
            auto count = 0;

            for (const auto callAgain: callstack) {
                if (callAgain == call) {
                    count++;
                }
            }

            if (count > 3 && !nestedCalls.contains(call)) {
                nestedCalls.insert(call);
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandlerAnalysis,
                          std::format("sub_{}() + {} is present many times on the call-stack (Possibly a Recursive "
                                      "sub-routine)",
                                      functionStart,
                                      reinterpret_cast<void *>(
                                              max(reinterpret_cast<std::intptr_t>(functionStart) -
                                                          reinterpret_cast<std ::intptr_t>(call),
                                                  reinterpret_cast<std::intptr_t>(call) -
                                                          reinterpret_cast<std::intptr_t>(functionStart)))));
            }
        }
        break;
    }

    /*
     *  CALL into nullptr, cannot be traced :(
     */
    while (true) {
        if (callstack.size() < 5) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandlerAnalysis,
                      "-- Possibly executing a call into a nullptr (Possibly a DEP (Data-Execution-Protection) "
                      "violation?)");
        }

        break;
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler, "-- Stack Trace analysis complete.");

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- End");

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler, "-- RbxStu V3 -- Emitting Debugging DUMP.");

    RbxStu::ExceptionHandler::CreateDump(pExceptionPointers);

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler, "-- RbxStu V3 -- Dump emitted.");


    MessageBoxA(nullptr, "Unhandled exception caught! Check console!", "Error", MB_OK);
    Sleep(10000);
    return EXCEPTION_EXECUTE_HANDLER;
}

void RbxStu::ExceptionHandler::CreateDump(EXCEPTION_POINTERS *pExceptionPointers) {
    LoadLibraryA("DbgHelp.dll");

    auto name = Utilities::GetDllDir().value().string();
    {
        SYSTEMTIME t;
        GetSystemTime(&t);
        name += std::format("/{}_{:4d}{:2d}{:2d}_{:2d}{:2d}{:2d}.dmp",
                            RbxStu::ExceptionHandler::LookupModuleByAddress(reinterpret_cast<void *>(
                                    reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)) + 0x8)),
                            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    auto hFile =
            CreateFileA(name.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    MINIDUMP_EXCEPTION_INFORMATION ex{};
    ex.ExceptionPointers = pExceptionPointers ? pExceptionPointers : nullptr;
    ex.ThreadId = GetCurrentThreadId();
    ex.ClientPointers = false;

    MiniDumpWriteDump(
            GetCurrentProcess(), GetCurrentProcessId(), hFile,
            static_cast<MINIDUMP_TYPE>(
                    ::MINIDUMP_TYPE::MiniDumpNormal | ::MINIDUMP_TYPE::MiniDumpWithDataSegs |
                    ::MINIDUMP_TYPE::MiniDumpWithFullMemory | ::MINIDUMP_TYPE::MiniDumpWithHandleData |
                    ::MINIDUMP_TYPE::MiniDumpFilterMemory | ::MINIDUMP_TYPE::MiniDumpScanMemory |
                    ::MINIDUMP_TYPE::MiniDumpWithUnloadedModules | ::MINIDUMP_TYPE::MiniDumpFilterModulePaths |
                    ::MINIDUMP_TYPE::MiniDumpWithProcessThreadData |
                    ::MINIDUMP_TYPE::MiniDumpWithPrivateReadWriteMemory | ::MINIDUMP_TYPE::MiniDumpWithoutOptionalData |
                    ::MINIDUMP_TYPE::MiniDumpWithThreadInfo | ::MINIDUMP_TYPE::MiniDumpWithCodeSegs

                    ),
            &ex, nullptr, nullptr);

    MessageBoxA(nullptr, "Dumping Process for debugging information, please wait a bit...", "Dumpíng Process", MB_OK);
    Sleep(15000);
}

void RbxStu::ExceptionHandler::InstallHandler() {
    // insert SEH into exception chain

    if (FastFlags::FFlagDisableErrorHandler.GetValue()) {
        RbxStuLog(Warning, StructuredExceptionHandler,
                  "An fast flag was disabled our error reporting, any crashes cannot be now properly reported!");
        return;
    }

    if (const auto previousHandler = SetUnhandledExceptionFilter(RbxStu::ExceptionHandler::UnhandledSEH);
        RbxStu::ExceptionHandler::UnhandledSEH == previousHandler) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  "Attempted to the top level SEH, but the top level SEH was ALREADY our exception handler! Did you by "
                  "mistake call RbxStu::ExceptionHandler::InstallHandler twice?");
    }
}

void RbxStu::ExceptionHandler::OverrideRBXCRASH() {
    RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
              "Attempting to hook RBXCRASH to monitor ROBLOX-sided crashes...");

    MH_Initialize();

    MH_CreateHook(RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBXCRASH),
                  RbxStu::ExceptionHandler::RBXCRASH, &pOriginalRbxCrash);
    MH_EnableHook(RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBXCRASH));
}
