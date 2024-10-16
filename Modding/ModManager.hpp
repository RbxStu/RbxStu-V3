//
// Created by Dottik on 15/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Modding {
    class ModManager final {
        static std::shared_ptr<RbxStu::Modding::ModManager> pInstance;
        std::atomic_bool m_bIsInitialized;

        bool IsInitialized();

        void Initialize();
    public:

        static std::shared_ptr<RbxStu::Modding::ModManager> GetSingleton();
    };
} // RbxStu::Modding
