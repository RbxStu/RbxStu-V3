//
// Created by Dottik on 13/11/2024.
//

#pragma once
#include <Windows.h>
#include "ExternalAssembly.h"

#define Begin_DivertControlFlow                     \
    __try {                                         \
        Halt();                                     \
        InterruptThree();                           \
    } __except(EXCEPTION_EXECUTE_HANDLER) {

#define End_DivertControlFlow \
    }


#define Confuse_Analysis                                                            \
    {                                                                               \
        __try {                                                                     \
            Halt();                                                                 \
            InterruptThree();                                                       \
        } __except (EXCEPTION_EXECUTE_HANDLER) {                                    \
            if (ForceComparison_Number(0, 0); (GetFlags() & 0x40) != 0x40)          \
                Halt();                                                             \
        }                                                                           \
    }

#define ObfuscateControlFlow_VariantOne(cond, run_what, ...)                        \
    {                                                                               \
        __try {                                                                     \
            if (cond) { InterruptThree(); } else { Confuse_Analysis; }              \
        } __except(EXCEPTION_EXECUTE_HANDLER) {                                     \
            run_what(__VA_ARGS__);                                                  \
        }                                                                           \
    }

#define ObfuscateControlFlow_VariantTwo(cond, run_what, ...)                        \
    {                                                                               \
        __try {                                                                     \
            if (!cond) { Confuse_Analysis; } else { Halt(); }                       \
        } __except(EXCEPTION_EXECUTE_HANDLER) {                                     \
            run_what(__VA_ARGS__);                                                  \
        }                                                                           \
    }
