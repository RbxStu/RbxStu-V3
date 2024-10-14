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
        std::shared_ptr<RTTIScanner> pRTTIScanner;
        std::atomic_bool m_bIsInitialized;

        void Initialize();

    public:
        static std::shared_ptr<RTTI> GetSingleton();

        bool IsInitialized();

        std::optional<std::shared_ptr<RTTIScanner::RTTI> > GetRuntimeTypeInformation(std::string_view name);

        std::vector<std::shared_ptr<RTTIScanner::RTTI> > GetAllRTTI();
    };
};
