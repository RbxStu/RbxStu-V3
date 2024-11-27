//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include "lualib.h"

namespace RbxStu::StuLuau::Environment {
    class Library abstract {
    public:
        virtual ~Library() = default;

        virtual const luaL_Reg *GetFunctionRegistry() { return nullptr; };

        virtual bool PushToGlobals() { return false; }

        virtual bool PushNoTable() { return false; }

        virtual const char *GetLibraryName() { return "????"; };
    };
} // RbxStu::StuLuau::Environment
