//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <StudioOffsets.h>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scanners {
    class RBX final {
    public:
        enum class PointerOffsets {
            RBX_ScriptContext_resume,
        };

        struct PointerOffsetInformation {
            ::RBX::PointerEncryptionType encryption;
            int64_t offset;
        };

    private:
        static std::shared_ptr<RbxStu::Scanners::RBX> pInstance;
        std::atomic_bool m_bIsInitialized;

        std::map<std::string_view, void *> m_FastFlagMap;
        std::map<PointerOffsets, PointerOffsetInformation> m_pointerObfuscation;
        std::map<RbxStuOffsets::OffsetKey, const void *> m_FunctionMap;

        void Initialize();

    public:
        static std::shared_ptr<RbxStu::Scanners::RBX> GetSingleton();

        bool IsInitialized();

        template<typename T>
        T GetFastFlag(const std::string_view fastFlagName) {
            if (!this->m_FastFlagMap.contains(fastFlagName)) {
                return {};
            }

            return reinterpret_cast<T>(this->m_FastFlagMap.at(fastFlagName));
        }

        template<typename T>
        std::optional<T> GetRobloxFunction(const RbxStuOffsets::OffsetKey functionIdentifier) const {
            if (!this->m_FunctionMap.contains(functionIdentifier)) {
                return {};
            }

            return this->m_FunctionMap.at(functionIdentifier);
        }
    };
} // RbxStu::Scanners
