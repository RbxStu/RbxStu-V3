//
// Created by Dottik on 11/10/2024.
//

#pragma once
#include <optional>
#include <string>
#include <vector>
#include <Windows.h>

namespace RbxStu {
    class ExceptionHandler final abstract {
        static long UnhandledSEH(EXCEPTION_POINTERS *pExceptionPointers);

        static std::optional<std::pair<std::string, void *> > LookupAddress(const void *address);

        static void RBXCRASH(const char *crashType, const char *crashDescription);

    public:
        static void InstallHandler();

        static void OverrideRBXCRASH();
    };
};
