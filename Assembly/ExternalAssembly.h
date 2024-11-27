//
// Created by Dottik on 26/11/2024.
//

#pragma once
#include <cstdint>

#ifndef EXTERN_C
#define EXTERN_C extern "C"
#endif

/*
 *  @brief Issues a `syscall`
 */
EXTERN_C void IssueSyscall();

/*
 *  @brief Issues a `call rax` instruction.
 */
EXTERN_C void CallRax();

/*
 *  @brief exposes the `hlt` instruction.
 */
EXTERN_C __declspec(noreturn) void Halt();

/*
 * @brief exposes the `ìnt3` instruction.
 */
EXTERN_C __declspec(noreturn) void InterruptThree();

/*
 * @brief performs `cmp rcx, rdx`.
 */
EXTERN_C void __fastcall ForceComparison_Number(int a, int b);

/*
 * @brief pushes the processor flags into rax.
 */
EXTERN_C int64_t __fastcall GetFlags();

/*
 * @brief obtains the previous instruction pointer.
 */
EXTERN_C void *PushRip();
