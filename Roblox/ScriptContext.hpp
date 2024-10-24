//
// Created by Dottik on 14/10/2024.
//

#pragma once
#include <memory>

#include "lua.h"
#include "TypeDefinitions.hpp"

namespace RbxStu::StuLuau {
    struct YieldResult;
}

namespace RbxStu::Roblox {
    class ScriptContext final {
        void *m_pScriptContext;

    public:
        static std::shared_ptr<RbxStu::Roblox::ScriptContext>
        FromWaitingHybridScriptsJob(void *waitingHybridScriptsJob);

        explicit ScriptContext(void *scriptContext);

        [[nodiscard]] void *GetRbxPointer() const;

        lua_State *GetGlobalState();

        void ResumeThread(RBX::Lua::WeakThreadRef *resumptionContext, const StuLuau::YieldResult &YieldResult);
    };
} // RbxStu::Roblox
