//
// Created by Dottik on 11/10/2024.
//

#pragma once
#include <optional>
#include <Windows.h>

namespace RbxStu {
    class ExceptionHandler {
        static std::optional<std::exception> GetCxxException();

        static long UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers);

    public:
        static void InstallHandler();
    };
};
