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
        void *m_pBackingJob;

    public:
        static std::shared_ptr<RbxStu::Roblox::ScriptContext>
        FromWaitingHybridScriptsJob(void *waitingHybridScriptsJob);

        explicit ScriptContext(void *scriptContext);
        explicit ScriptContext(void *scriptContext, void*backingJob);

        [[nodiscard]] void *GetRbxPointer() const;

        lua_State *GetGlobalState();

        void ResumeThread(RBX::Lua::WeakThreadRef *resumptionContext, const StuLuau::YieldResult &YieldResult) const;
    };
} // RbxStu::Roblox
