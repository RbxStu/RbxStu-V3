//
// Created by Dottik on 11/10/2024.
//

#pragma once
#include <optional>
#include <vector>
#include <Windows.h>

namespace RbxStu {
    class ExceptionHandler {
        static long UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers);

    public:
        static void InstallHandler();
    };
};
