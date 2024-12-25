//
// Created by Dottik on 12/10/2024.
//

#pragma once

#define RBXSTU_REBASE_STACKTRACE_ON_SEH true
#define RBXSTU_DLL_NAME "RbxStuV3"
#define RBXSTU_VERSION "v0.1.0-alpha"

// Begin declspec macros.
#define RBXSTU_EXPORT __declspec(dllexport)
#define RBXSTU_NOINLINE __declspec(noinline
#define RBXSTU_FORCEINLINE __forceinline

// Begin Macro Definitions

#ifndef RBXSTU_ASSERTOVERRIDE
#define RBXSTU_ASSERTOVERRIDE
#undef assert_ex
#undef assert

#define assert_ex(condition, exception) { if (!(condition)) { throw exception; } }
#define assert(condition, message) { if (!(condition)) { throw std::exception { std::format("{} @ {}", message, __func__).c_str() }; } }
#endif
