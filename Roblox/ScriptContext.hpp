//
// Created by Dottik on 14/10/2024.
//

#pragma once
#include <memory>

#include "lua.h"

namespace RbxStu::Roblox {
    class ScriptContext {
        void *m_pScriptContext;

    public:
        static std::shared_ptr<RbxStu::Roblox::ScriptContext>
        FromWaitingHybridScriptsJob(void *waitingHybridScriptsJob);

        explicit ScriptContext(void *scriptContext);

        void *GetRbxPointer() const;

        lua_State *GetGlobalState();
    };
} // RbxStu::Roblox
