//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Scanners {
    class RBXReflection {
        static std::shared_ptr<RbxStu::Scanners::RBXReflection> pInstance;
    public:
        static std::shared_ptr<RbxStu::Scanners::RBXReflection> GetSingleton();
    };
} // RbxStu::Scanners
