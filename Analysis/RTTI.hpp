//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <memory>
#include <optional>
#include <RTTIHook/RTTIScanner.h>

namespace RbxStu::Analysis {
    class RTTI final {
        static std::shared_ptr<RTTI> pInstance;
        std::atomic_bool m_bIsInitialized;

    public:
        bool IsInitialized();

        void Initialize();

        std::optional<std::shared_ptr<RTTIScanner::RTTI> > GetRuntimeTypeInformation(std::string_view name);

        static std::shared_ptr<RTTI> GetSingleton();
    };
};
