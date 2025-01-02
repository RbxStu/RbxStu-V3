//
// Created by Dottik on 13/10/2024.
//

#pragma once

#include "Roblox/TypeDefinitions.hpp"
#include "lua.h"


#include <StudioOffsets.h>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>


namespace RBX {
    namespace Reflection {
        struct ClassDescriptor;
    }
    enum PointerEncryptionType : std::int32_t;
} // namespace RBX
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

        void **m_RbxNamektable;
        std::map<std::string_view, ::RBX::Reflection::ClassDescriptor *> m_classDescriptorMap;
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

        std::vector<::RBX::Reflection::ClassDescriptor *> GetClassDescriptors() const;

        std::optional<RbxStu::Scanners::RBX::PointerOffsetInformation>
        GetRbxPointerOffset(RbxStu::Scanners::RBX::PointerOffsets offset);

        void *GetKtableIndexFromAtom(const int atom) const {
            return *(this->m_RbxNamektable + static_cast<std::int64_t>(atom));
        }

        ::RBX::Reflection::ClassDescriptor *GetClassDescriptor(const std::string_view className) const {
            return this->m_classDescriptorMap.at(className);
        }

        void *GetPropertyForClass(lua_State *L, const std::string_view className,
                                  const std::string_view propertyName) const {
            const auto propDesc = this->GetClassDescriptor(className);
            int atom{};
            lua_pushstring(L, propertyName.data());
            lua_tostringatom(L, -1, &atom);
            lua_pop(L, 1);

            const auto searchTarget = this->GetKtableIndexFromAtom(atom);

            const auto end = static_cast<void **>(propDesc->innerPropertyDescriptorsEnd);
            auto current = static_cast<void **>(propDesc->innerPropertyDescriptorsStart);

            while (current != end) {
                if (*reinterpret_cast<std::uintptr_t *>(current) == 0) {
                    current++;
                    continue;
                }

                if (*current == searchTarget) {
                    return *reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(current) + 0x8);
                }

                current++;
            }

            return nullptr;
        }
    };
} // namespace RbxStu::Scanners
