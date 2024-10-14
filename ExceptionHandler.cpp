//
// Created by Dottik on 11/10/2024.
//


#include "ExceptionHandler.hpp"

#include <print>
#include <Psapi.h>
#include <Windows.h>
#include <unordered_set>
#include <Utilities.hpp>

#include "Logger.hpp"
#include "Settings.hpp"
#include "Analysis/Disassembler.hpp"

std::optional<std::pair<std::string, void*>> RbxStu::ExceptionHandler::LookupAddress(const void* address)
{
    HMODULE processModules[1024];
    HANDLE ourProcessHandle = GetCurrentProcess();
    DWORD Needed;

    if (EnumProcessModules(ourProcessHandle, processModules, sizeof(processModules), &Needed))
    {
        for (unsigned int i = 0; i < Needed / sizeof(HMODULE); i++)
        {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(ourProcessHandle, processModules[i], &moduleInfo, sizeof(moduleInfo)))
            {
                void* moduleBase = moduleInfo.lpBaseOfDll;
                size_t moduleSize = moduleInfo.SizeOfImage;
                if (address >= static_cast<BYTE*>(moduleBase) && address < static_cast<BYTE*>(moduleBase) + moduleSize)
                {
                    WCHAR moduleName[MAX_PATH];
                    if (GetModuleFileNameExW(ourProcessHandle, processModules[i], moduleName, sizeof(moduleName) / sizeof(WCHAR)))
                    {
                        std::filesystem::path fullPath(moduleName);
                        std::wstring fileName = fullPath.filename();
                        std::string moduleNameConverted = RbxStu::Utilities::WcharToString(fileName.c_str());

                        return std::pair<std::string, void*>({ moduleNameConverted, moduleBase });
                    }
                }
            }
        }
    }

    return {};
}


long RbxStu::ExceptionHandler::UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers) {
    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- Begin");

    std::optional<std::exception> currentCxxException;
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &e) {
        currentCxxException = e;
    } catch (...) {
        currentCxxException = {};
    }

    if (currentCxxException.has_value()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  std::format( "C++ exception was found as a current exception: '{}'",
                      currentCxxException.value().what()));
    } else {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  "No C++ exception found (unusual)");
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Obtaining callstack...");

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Walking Stack...");

    const auto disassembler = RbxStu::Analysis::Disassembler::GetSingleton();

    void *stack[256];
    const auto frameCount = RtlCaptureStackBackTrace(0, 255, stack, nullptr);
    const auto callstack = std::vector<void *>(stack, stack + frameCount);

    for (auto call: callstack) {
        const auto functionStart = disassembler->GetFunctionStart(call);
        std::string belongsTo = "Unknown Origin";

        void* targetModuleBaseAddress = nullptr;
        auto lookupResults = RbxStu::ExceptionHandler::LookupAddress(call);
        if (lookupResults.has_value())
        {
            belongsTo = lookupResults.value().first;
            targetModuleBaseAddress = lookupResults.value().second;
        }

        auto message = std::format("-> sub_{:X}() + {}, belonging to {}",
                                   reinterpret_cast<std::intptr_t>(functionStart),
                                   reinterpret_cast<void *>(max(
                                       reinterpret_cast<std::intptr_t>(functionStart) - reinterpret_cast<std
                                       ::intptr_t>(call),
                                       reinterpret_cast<std::intptr_t>(call) - reinterpret_cast<std::intptr_t>(
                                           functionStart))),
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

            RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
                      std::format("{}; Rebased to Module Base: {}", message, reinterpret_cast<void*>(reinterpret_cast<
                          std::uintptr_t>(call) -
                          reinterpret_cast<std::uintptr_t>(targetModuleBaseAddress) + base)));
        }
    }

    // Analyse step.

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Analysing the Stack Trace for common exception patterns...");

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
                      std::format(
                          "sub_{}() + {} is present many times on the call-stack (Possibly a Recursive sub-routine)",
                          functionStart, reinterpret_cast<void *>(max(
                              reinterpret_cast<std::intptr_t>(functionStart) - reinterpret_cast<std
                              ::intptr_t>(call),
                              reinterpret_cast<std::intptr_t>(call) - reinterpret_cast<std::intptr_t>(
                                  functionStart)))));
        }
    }

    RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
              "-- Stack Trace analysis complete.");

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- End");

    Sleep(10000);
    return EXCEPTION_CONTINUE_EXECUTION;
}

void RbxStu::ExceptionHandler::InstallHandler() {
    // insert SEH into exception chain

    if (const auto previousHandler = SetUnhandledExceptionFilter(RbxStu::ExceptionHandler::UnhandledSEH);
        RbxStu::ExceptionHandler::UnhandledSEH == previousHandler) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  "Attempted to the top level SEH, but the top level SEH was ALREADY our exception handler! Did you by mistake call RbxStu::ExceptionHandler::InstallHandler twice?");
    }
}
