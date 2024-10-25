//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include "lualib.h"

namespace RbxStu::StuLuau::Environment {
    class Library abstract {
    public:
        virtual ~Library() = default;

        virtual luaL_Reg *GetFunctionRegistry() { return nullptr; };

        virtual const char *GetLibraryName() { return nullptr; };
    };
} // RbxStu::StuLuau::Environment
