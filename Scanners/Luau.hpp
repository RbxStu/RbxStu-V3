//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <memory>

namespace RbxStu::Scanners {
    class Luau {
        static std::shared_ptr<RbxStu::Scanners::Luau> pInstance;
    public:
        static std::shared_ptr<RbxStu::Scanners::Luau> GetSingleton();
    };
} // RbxStu::Scanners
