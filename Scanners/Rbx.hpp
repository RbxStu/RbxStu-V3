//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Scanners {
    class RBX {
        static std::shared_ptr<RbxStu::Scanners::RBX> pInstance;
    public:
        static std::shared_ptr<RbxStu::Scanners::RBX> GetSingleton();
    };
} // RbxStu::Scanners
