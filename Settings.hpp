//
// Created by Dottik on 12/10/2024.
//

#pragma once

#define RBXSTU_ENABLE_DEBUG_LOGS true
#define RBXSTU_DLL_NAME "RbxStuV3"

// Begin Macro Definitions

#define assert_ex(condition, exception) { if (!(condition)) { throw exception; } }
#define assert(condition, message) { if (!(condition)) { throw std::exception { std::format("{} @ {}", message, __func__).c_str() }; } }
