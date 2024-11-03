//
// Created by Dottik on 12/10/2024.
//

#pragma once

#define RBXSTU_REBASE_STACKTRACE_ON_SEH true
#define RBXSTU_ENABLE_DEBUG_LOGS false
#define RBXSTU_DLL_NAME "RbxStuV3"


// Begin declspec macros.
#define RBXSTU_EXPORT __declspec(dllexport)
#define RBXSTU_NOINLINE __declspec(noinline
#define RBXSTU_FORCEINLINE __forceinline

// Begin Macro Definitions

#define assert_ex(condition, exception) { if (!(condition)) { throw exception; } }
#define assert(condition, message) { if (!(condition)) { throw std::exception { std::format("{} @ {}", message, __func__).c_str() }; } }
