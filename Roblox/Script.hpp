//
// Created by Dottik on 29/11/2024.
//

#pragma once
#include <optional>
#include <string>
#include "Miscellaneous/OpaqueClass.hpp"

namespace RbxStu::Roblox {

    class Script final : public RbxStu::Miscellaneous::OpaqueClass {
    public:
        enum class ScriptKind : std::int8_t;

    private:
        RbxStu::Roblox::Script::ScriptKind m_scriptKind;

    public:
        enum class ScriptKind : std::int8_t { LocalScript, ModuleScript, Script };
        explicit Script(void *pNative, RbxStu::Roblox::Script::ScriptKind kind);

        [[nodiscard]] std::string GetScriptHash() const;
        [[nodiscard]] std::optional<std::string> GetSource() const;
        [[nodiscard]] std::optional<std::string> GetBytecode() const;
    };

} // namespace RbxStu::Roblox
