//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <memory>
#include <vector>

namespace RbxStu::Analysis {
    class Disassembler {
        static std::shared_ptr<Disassembler> pInstance;
        std::atomic_bool m_bIsInitialized;

        void Initialize();

    public:
        bool IsInitialized();

        const void *GetFunctionStart(const void *call);

        static std::shared_ptr<Disassembler> GetSingleton();
    };
};
