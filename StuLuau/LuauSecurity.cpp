//
// Created by Dottik on 24/10/2024.
//

#include "LuauSecurity.hpp"

#include <libhat/Process.hpp>

#include "Environment/UNC/Closures.hpp"
#undef LoadString

const static std::map<RbxStu::StuLuau::ExecutionSecurity, std::vector<RBX::Security::CapabilityPermissions>>
        s_CapabilityMap = {
                {RbxStu::StuLuau::ExecutionSecurity::Plugin,
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
                         ::RBX::Security::CapabilityPermissions::LoadStringCapability,
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
                         ::RBX::Security::CapabilityPermissions::LegacySound,

                         ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
                 }},
                {RbxStu::StuLuau::ExecutionSecurity::RobloxPlugin,
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
                         ::RBX::Security::CapabilityPermissions::LoadStringCapability,
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
                         ::RBX::Security::CapabilityPermissions::LegacySound,

                         ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
                 }},
                {RbxStu::StuLuau::ExecutionSecurity::LocalScript,
                 {
                         /*
                          *  LocalUser is virtually all capabilities from 1 to 3.
                          */
                         ::RBX::Security::CapabilityPermissions::LocalUser,
                         ::RBX::Security::CapabilityPermissions::WritePlayer,

                         ::RBX::Security::CapabilityPermissions::RunClientScript,

                         ::RBX::Security::CapabilityPermissions::Unassigned,
                         ::RBX::Security::CapabilityPermissions::AssetRequire,
                         ::RBX::Security::CapabilityPermissions::LoadStringCapability,
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
                         ::RBX::Security::CapabilityPermissions::LegacySound,

                 }},
                {RbxStu::StuLuau::ExecutionSecurity::RobloxScript,
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
                         ::RBX::Security::CapabilityPermissions::LoadStringCapability,
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
                         ::RBX::Security::CapabilityPermissions::LegacySound,

                 }},
                {RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor,
                 {/*
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
                  ::RBX::Security::CapabilityPermissions::LoadStringCapability,
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
                  ::RBX::Security::CapabilityPermissions::LegacySound,

                  ::RBX::Security::CapabilityPermissions::PluginOrOpenCloud,
                  ::RBX::Security::CapabilityPermissions::Assistant}}};

namespace RbxStu::StuLuau {
    std::shared_ptr<RbxStu::StuLuau::LuauSecurity> LuauSecurity::pInstance;

    std::shared_ptr<RbxStu::StuLuau::LuauSecurity> LuauSecurity::GetSingleton() {
        if (nullptr == RbxStu::StuLuau::LuauSecurity::pInstance)
            RbxStu::StuLuau::LuauSecurity::pInstance = std::make_shared<RbxStu::StuLuau::LuauSecurity>();

        return RbxStu::StuLuau::LuauSecurity::pInstance;
    }

    std::int64_t LuauSecurity::ToCapabilitiesFlags(const RbxStu::StuLuau::ExecutionSecurity executionSecurity) {
        std::int64_t qwCapabilities = 0x3FFFFFF00ull; // Base Check (Without it, we are banned from roblox APIs)

        for (const auto &capabilitySet: s_CapabilityMap.at(executionSecurity))
            qwCapabilities |= static_cast<std::int64_t>(capabilitySet);

        return qwCapabilities;
    }

    RBX::Security::Permissions
    LuauSecurity::GetIdentityFromExecutionSecurity(const RbxStu::StuLuau::ExecutionSecurity executionSecurity) {
        switch (executionSecurity) {
            case RbxStu::StuLuau::ExecutionSecurity::LocalScript:
                return RBX::Security::Permissions::LocalUserPermission;
            case RbxStu::StuLuau::ExecutionSecurity::RobloxScript:
                return RBX::Security::Permissions::RobloxScriptPermission;

            case RbxStu::StuLuau::ExecutionSecurity::Plugin:
            case RbxStu::StuLuau::ExecutionSecurity::RobloxPlugin:
                return RBX::Security::Permissions::NotAccessiblePermission;

            case RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor:
                return RBX::Security::Permissions::ExecutorLevelPermission;
        }

        return RBX::Security::Permissions::NotAccessiblePermission;
    }

    ExecutionSecurity LuauSecurity::GetExecutionSecurityFromIdentity(const int32_t identity) {
        switch (identity) {
            case RBX::Security::Permissions::LocalUserPermission:
            case RBX::Security::Permissions::GameScriptPermission:
                return ExecutionSecurity::LocalScript;
            case RBX::Security::Permissions::RobloxScriptPermission:
                return ExecutionSecurity::RobloxScript;
            case RBX::Security::PluginPermission:
                return ExecutionSecurity::Plugin;
            case RBX::Security::ExecutorLevelPermission:
                return ExecutionSecurity::RobloxExecutor;
            default:
                return ExecutionSecurity::LocalScript;
        }
    }


    bool LuauSecurity::IsOurThread(lua_State *L) {
        // Return RbxStu V2 impl, we cannot be for sure if the thread is ours if the std::weak_ptr<RBX::Script> is
        // inaccessible.
        return L != nullptr && GetThreadExtraspace(L) != nullptr && GetThreadExtraspace(L)->script == nullptr;
    }

    static void set_proto(Proto *proto, std::uint64_t *proto_identity) {
        // NEVER FORGET TO SET THE PROTOS and SUB PROTOS USERDATA!!
        proto->userdata = static_cast<void *>(proto_identity);
        for (auto i = 0; i < proto->sizep; i++)
            set_proto(proto->p[i], proto_identity);
    }

    void LuauSecurity::ElevateClosure(const Closure *closure, const RbxStu::StuLuau::ExecutionSecurity execSecurity) {
        if (closure->isC)
            return;

        auto *security = new std::uint64_t[0x1]{0};
        *security = static_cast<std::uint64_t>(this->ToCapabilitiesFlags(execSecurity));

        set_proto(closure->l.p, security);
    }

    void LuauSecurity::ElevateClosureWithExplicitCapabilities(const Closure *closure,
                                                              const std::uintptr_t capabilities) {
        if (closure->isC)
            return;

        auto *security = new std::uint64_t[0x1]{0};
        *security = static_cast<std::uint64_t>(capabilities);

        set_proto(closure->l.p, security);
    }

    bool LuauSecurity::IsOurClosure(Closure *closure) {
        /*
         *  We modify the Luau Closures we push with their line defined to be -1.
         */
        if (!closure->isC)
            return closure->l.p->linedefined == -1;

        /*
         *  We check if the c.f is our newcclosure stub, for other functions, we just check how close the memory address
         *  is to ROBLOXs .text section, if it is within the range of .text, then it is likely that it is not our
         *  function.
         */

        const auto cClosure = reinterpret_cast<std::uintptr_t>(closure->c.f);
        const auto textSection = hat::process::get_process_module().get_section_data(".text");

        const auto isRobloxFunction =
                cClosure > reinterpret_cast<std::uintptr_t>(textSection.data()) &&
                cClosure < reinterpret_cast<std::uintptr_t>(textSection.data() + textSection.size());

        return closure->c.f == RbxStu::StuLuau::Environment::UNC::Closures::newcclosure_stub || !isRobloxFunction;
    }

    void LuauSecurity::SetThreadSecurity(lua_State *L, const ExecutionSecurity executionSecurity,
                                         const std::int32_t identity) {
        const auto extraSpace = GetThreadExtraspace(L);
        if (nullptr == extraSpace)
            L->global->cb.userthread(L->global->mainthread, L); // Initialize RBX::ExtraSpace forcefully.

        if (nullptr == extraSpace)
            return;

        const std::int64_t executorSecurity = this->ToCapabilitiesFlags(executionSecurity);

        extraSpace->capabilities = executorSecurity;
        extraSpace->contextInformation.identity = static_cast<RBX::Security::Permissions>(identity);

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
                  std::format("Setting thread capabilities to {}; identity: {}; thread: {}", (int) executionSecurity,
                              identity, (void *) L));
    }
} // namespace RbxStu::StuLuau
