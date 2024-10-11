//
// Created by Dottik on 11/10/2024.
//

#include "ExceptionHandler.hpp"

#include <cstdio>
#include <Windows.h>


std::optional<const std::exception &> RbxStu::ExceptionHandler::GetCxxException() {
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &e) {
        return e;
    }

    return {};
}

long RbxStu::ExceptionHandler::UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers) {
    printf("-- RbxStu V3 Structured Exception Handler -- Begin\n");

    const auto currentCxxException = RbxStu::ExceptionHandler::GetCxxException();

    if (currentCxxException.has_value())
        printf("C++ exception was found as a current exception: '%s'\n", currentCxxException.value().what());
    else
        printf("No C++ exception was found as a current exception\n");


    printf("-- RbxStu V3 Structured Exception Handler -- End\n");

    return EXCEPTION_CONTINUE_EXECUTION;
}

void RbxStu::ExceptionHandler::InstallHandler() {
    // insert SEH into exception chain
    SetUnhandledExceptionFilter(RbxStu::ExceptionHandler::UnhandledSEH);
}
