//
// Created by Dottik on 11/10/2024.
//


#include "ExceptionHandler.hpp"

#include <print>
#include <Windows.h>

#include "Logger.hpp"


std::optional<std::exception> RbxStu::ExceptionHandler::GetCxxException() {
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &e) {
        return e;
    } catch (...) {
        return {};
    }
}

long RbxStu::ExceptionHandler::UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers) {
    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- Begin");

    if (const auto currentCxxException = RbxStu::ExceptionHandler::GetCxxException(); currentCxxException.has_value()) {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  std::format( "C++ exception was found as a current exception: '{}'",
                      currentCxxException.value().what()));
    } else {
        RbxStuLog(RbxStu::LogType::Warning, RbxStu::StructuredExceptionHandler,
                  "No C++ exception found (unusual)");
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
              "-- Obtaining callstack...");

    RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler,
              "-- Walking Stack...");

    for (const auto callstack = RbxStu::ExceptionHandler::GetCallStack(); auto call: callstack) {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::StructuredExceptionHandler, std::format("{}", call));
    }

    RbxStuLog(RbxStu::LogType::Error, RbxStu::StructuredExceptionHandler,
              "-- RbxStu V3 Structured Exception Handler -- End");
    return EXCEPTION_CONTINUE_EXECUTION;
}

std::vector<void *> RbxStu::ExceptionHandler::GetCallStack() {
    void *stack[256] = {};
    const unsigned short frameCount = RtlCaptureStackBackTrace(0, 255, stack, nullptr);
    return std::vector<void *>{stack, stack + frameCount};
}

void RbxStu::ExceptionHandler::InstallHandler() {
    // insert SEH into exception chain
    SetUnhandledExceptionFilter(RbxStu::ExceptionHandler::UnhandledSEH);
}
