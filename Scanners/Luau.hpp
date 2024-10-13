//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Scanners {
    class Luau {
        static std::shared_ptr<RbxStu::Scanners::Luau> pInstance;
        std::atomic_bool m_bIsInitialized;

        void Initialize();

    public:
        static std::shared_ptr<RbxStu::Scanners::Luau> GetSingleton();

        bool IsInitialized();
    };
} // RbxStu::Scanners
