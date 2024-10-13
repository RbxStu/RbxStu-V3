//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Analysis {
    class Scanner {
        static std::shared_ptr<RbxStu::Analysis::Scanner> pInstance;
        std::atomic_bool m_bIsInitialized;

    public:
        bool IsInitialized();

        void Initialize();

        static std::shared_ptr<RbxStu::Analysis::Scanner> GetSingleton();
    };
} // RbxStu
