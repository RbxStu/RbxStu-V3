//
// Created by Dottik on 24/10/2024.
//

#include "LuauSecurity.hpp"

const static std::map<RbxStu::StuLuau::ExecutionSecurity, std::vector<RBX::Security::CapabilityPermissions> >
s_CapabilityMap = {
    {
        RbxStu::StuLuau::ExecutionSecurity::Plugin,
        {
            /*
             *  PluginSecurity is virtually all capabilities from 1 to 6.
             */
            ::RBX::Security::CapabilityPermissions::Plugin,
            ::RBX::Security::CapabilityPermissions::LocalUser,
            ::RBX::Security::CapabilityPermissions::WritePlayer,
            ::RBX::Security::CapabilityPermissions::RobloxEngine,
            ::RBX::Security::CapabilityPermissions::NotAccessible,

            ::RBX::Security::CapabilityPermissions::RunClientScript,
            ::RBX::Security::CapabilityPermissions::RunServerScript,

            ::RBX::Security::CapabilityPermissions::Unassigned,
            ::RBX::Security::CapabilityPermissions::AssetRequire,
            ::RBX::Security::CapabilityPermissions::LoadString,
            ::RBX::Security::CapabilityPermissions::ScriptGlobals,
            ::RBX::Security::CapabilityPermissions::CreateInstances,
            ::RBX::Security::CapabilityPermissions::Basic,
            ::RBX::Security::CapabilityPermissions::Audio,
            ::RBX::Security::CapabilityPermissions::DataStore,
            ::RBX::Security::CapabilityPermissions::Network,
            ::RBX::Security::CapabilityPermissions::Physics,
            ::RBX::Security::CapabilityPermissions::UI,
            ::RBX::Security::CapabilityPermissions::CSG,
            ::RBX::Security::CapabilityPermissions::Chat,
            ::RBX::Security::CapabilityPermissions::Animation,
            ::RBX::Security::CapabilityPermissions::Avatar,
            ::RBX::Security::CapabilityPermissions::RemoteEvent,

            ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
        }
    },
    {
        RbxStu::StuLuau::ExecutionSecurity::RobloxPlugin,
        {
            /*
             *  PluginSecurity (Roblox) is virtually all capabilities from 1 to 6.
             */
            ::RBX::Security::CapabilityPermissions::Plugin,
            ::RBX::Security::CapabilityPermissions::LocalUser,
            ::RBX::Security::CapabilityPermissions::WritePlayer,
            ::RBX::Security::CapabilityPermissions::RobloxScript,
            ::RBX::Security::CapabilityPermissions::RobloxEngine,
            ::RBX::Security::CapabilityPermissions::NotAccessible,

            ::RBX::Security::CapabilityPermissions::RunClientScript,
            ::RBX::Security::CapabilityPermissions::RunServerScript,

            ::RBX::Security::CapabilityPermissions::Unassigned,
            ::RBX::Security::CapabilityPermissions::AssetRequire,
            ::RBX::Security::CapabilityPermissions::LoadString,
            ::RBX::Security::CapabilityPermissions::ScriptGlobals,
            ::RBX::Security::CapabilityPermissions::CreateInstances,
            ::RBX::Security::CapabilityPermissions::Basic,
            ::RBX::Security::CapabilityPermissions::Audio,
            ::RBX::Security::CapabilityPermissions::DataStore,
            ::RBX::Security::CapabilityPermissions::Network,
            ::RBX::Security::CapabilityPermissions::Physics,
            ::RBX::Security::CapabilityPermissions::UI,
            ::RBX::Security::CapabilityPermissions::CSG,
            ::RBX::Security::CapabilityPermissions::Chat,
            ::RBX::Security::CapabilityPermissions::Animation,
            ::RBX::Security::CapabilityPermissions::Avatar,
            ::RBX::Security::CapabilityPermissions::RemoteEvent,

            ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
        }
    },
    {
        RbxStu::StuLuau::ExecutionSecurity::LocalScript,
        {
            /*
             *  LocalUser is virtually all capabilities from 1 to 3.
             */
            ::RBX::Security::CapabilityPermissions::LocalUser,
            ::RBX::Security::CapabilityPermissions::WritePlayer,

            ::RBX::Security::CapabilityPermissions::RunClientScript,

            ::RBX::Security::CapabilityPermissions::Unassigned,
            ::RBX::Security::CapabilityPermissions::AssetRequire,
            ::RBX::Security::CapabilityPermissions::LoadString,
            ::RBX::Security::CapabilityPermissions::ScriptGlobals,
            ::RBX::Security::CapabilityPermissions::CreateInstances,
            ::RBX::Security::CapabilityPermissions::Basic,
            ::RBX::Security::CapabilityPermissions::Audio,
            ::RBX::Security::CapabilityPermissions::DataStore,
            ::RBX::Security::CapabilityPermissions::Network,
            ::RBX::Security::CapabilityPermissions::Physics,
            ::RBX::Security::CapabilityPermissions::UI,
            ::RBX::Security::CapabilityPermissions::CSG,
            ::RBX::Security::CapabilityPermissions::Chat,
            ::RBX::Security::CapabilityPermissions::Animation,
            ::RBX::Security::CapabilityPermissions::Avatar,
            ::RBX::Security::CapabilityPermissions::RemoteEvent,

        }
    },
    {
        RbxStu::StuLuau::ExecutionSecurity::RobloxScript,
        {
            /*
             *  LocalUser is virtually all capabilities from 1 to 3.
             */
            ::RBX::Security::CapabilityPermissions::LocalUser,
            ::RBX::Security::CapabilityPermissions::WritePlayer,
            ::RBX::Security::CapabilityPermissions::RobloxScript,
            ::RBX::Security::CapabilityPermissions::RobloxEngine,

            ::RBX::Security::CapabilityPermissions::RunClientScript,

            ::RBX::Security::CapabilityPermissions::Unassigned,
            ::RBX::Security::CapabilityPermissions::AssetRequire,
            ::RBX::Security::CapabilityPermissions::LoadString,
            ::RBX::Security::CapabilityPermissions::ScriptGlobals,
            ::RBX::Security::CapabilityPermissions::CreateInstances,
            ::RBX::Security::CapabilityPermissions::Basic,
            ::RBX::Security::CapabilityPermissions::Audio,
            ::RBX::Security::CapabilityPermissions::DataStore,
            ::RBX::Security::CapabilityPermissions::Network,
            ::RBX::Security::CapabilityPermissions::Physics,
            ::RBX::Security::CapabilityPermissions::UI,
            ::RBX::Security::CapabilityPermissions::CSG,
            ::RBX::Security::CapabilityPermissions::Chat,
            ::RBX::Security::CapabilityPermissions::Animation,
            ::RBX::Security::CapabilityPermissions::Avatar,
            ::RBX::Security::CapabilityPermissions::RemoteEvent,

        }
    },
    {
        RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor,
        {
            /*
             *  RobloxExecutor is virtually all capabilities except RESTRICTED (-1, meaning all bits toggled :)).
             */

            ::RBX::Security::CapabilityPermissions::Plugin,
            ::RBX::Security::CapabilityPermissions::LocalUser,
            ::RBX::Security::CapabilityPermissions::WritePlayer,
            ::RBX::Security::CapabilityPermissions::RobloxScript,
            ::RBX::Security::CapabilityPermissions::RobloxEngine,
            ::RBX::Security::CapabilityPermissions::NotAccessible,

            ::RBX::Security::CapabilityPermissions::RunClientScript,
            ::RBX::Security::CapabilityPermissions::RunServerScript,
            ::RBX::Security::CapabilityPermissions::AccessOutsideWrite,

            ::RBX::Security::CapabilityPermissions::Unassigned,
            ::RBX::Security::CapabilityPermissions::AssetRequire,
            ::RBX::Security::CapabilityPermissions::LoadString,
            ::RBX::Security::CapabilityPermissions::ScriptGlobals,
            ::RBX::Security::CapabilityPermissions::CreateInstances,
            ::RBX::Security::CapabilityPermissions::Basic,
            ::RBX::Security::CapabilityPermissions::Audio,
            ::RBX::Security::CapabilityPermissions::DataStore,
            ::RBX::Security::CapabilityPermissions::Network,
            ::RBX::Security::CapabilityPermissions::Physics,
            ::RBX::Security::CapabilityPermissions::UI,
            ::RBX::Security::CapabilityPermissions::CSG,
            ::RBX::Security::CapabilityPermissions::Chat,
            ::RBX::Security::CapabilityPermissions::Animation,
            ::RBX::Security::CapabilityPermissions::Avatar,

            ::RBX::Security::CapabilityPermissions::RemoteEvent,

            ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
            ::RBX::Security::CapabilityPermissions::Assistant
        }
    }
};

namespace RbxStu::StuLuau {
    std::shared_ptr<RbxStu::StuLuau::LuauSecurity> LuauSecurity::pInstance;

    std::shared_ptr<RbxStu::StuLuau::LuauSecurity> LuauSecurity::GetSingleton() {
        if (nullptr == RbxStu::StuLuau::LuauSecurity::pInstance)
            RbxStu::StuLuau::LuauSecurity::pInstance = std::make_shared<RbxStu::StuLuau::LuauSecurity>();

        return RbxStu::StuLuau::LuauSecurity::pInstance;
    }

    RBX::ExtraSpace *LuauSecurity::GetThreadExtraspace(lua_State *L) {
        return static_cast<RBX::ExtraSpace *>(L->userdata);
    }

    std::int64_t LuauSecurity::ToCapabilitiesFlags(const RbxStu::StuLuau::ExecutionSecurity executionSecurity) {
        std::int64_t qwCapabilities = 0x1FFFFFF00ull; // Base Check (Without it, we are banned from roblox APIs)

        for (const auto &capabilitySet: s_CapabilityMap.at(executionSecurity))
            qwCapabilities |= static_cast<std::int64_t>(capabilitySet);

        return qwCapabilities;
    }

    void LuauSecurity::MarkThread(lua_State *L) {
        GetThreadExtraspace(L)->capabilities |= 1ull << 48ull;
    }

    bool LuauSecurity::IsMarkedThread(lua_State *L) {
        return (GetThreadExtraspace(L)->capabilities & 1ull << 48ull) == 1ull << 48ull;
    }

    bool LuauSecurity::IsOurThread(lua_State *L) {
        if (const auto extraSpace = GetThreadExtraspace(L); !extraSpace->script.expired()) {
            // Script ptr is not expired, we can access it.
            const auto locked = extraSpace->script.lock();

            const auto isOurThread = locked.get() == nullptr || LuauSecurity::IsMarkedThread(L);

            GetThreadExtraspace(L)->script.reset();
            return isOurThread;
        }

        // Return RbxStu V2 impl, we cannot be for sure if the thread is ours if the std::weak_ptr<RBX::Script> is unaccessible.
        return LuauSecurity::IsMarkedThread(L);
    }

    static void set_proto(Proto *proto, std::uint64_t *proto_identity) {
        // NEVER FORGET TO SET THE PROTOS and SUB PROTOS USERDATA!!
        proto->userdata = static_cast<void *>(proto_identity);
        for (auto i = 0; i < proto->sizep; i++) {
            set_proto(proto->p[i], proto_identity);
        }
    }

    void LuauSecurity::ElevateClosure(Closure *closure, const RbxStu::StuLuau::ExecutionSecurity execSecurity) {
        if (closure->isC) return;

        auto *security = new std::uint64_t[0x1];
        *security = static_cast<std::uint64_t>(this->ToCapabilitiesFlags(execSecurity));

        set_proto(closure->l.p, security);
    }

    void LuauSecurity::SetThreadSecurity(lua_State *L, const ExecutionSecurity executionSecurity,
                                         const bool markThread) {
        const auto extraSpace = GetThreadExtraspace(L);
        if (nullptr == extraSpace)
            L->global->cb.userthread(L->global->mainthread, L); // Initialize RBX::ExtraSpace forcefully.

        if (nullptr == extraSpace)
            return;

        const std::int64_t executorSecurity = this->ToCapabilitiesFlags(executionSecurity);

        extraSpace->capabilities = executorSecurity;

        if (markThread) this->MarkThread(L);
    }
} // RbxStu::StuLuau
