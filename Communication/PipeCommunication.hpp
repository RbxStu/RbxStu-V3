//
// Created by Dottik on 12/11/2024.
//

#pragma once

#include <string>

namespace RbxStu::Communication {
    class PipeCommunication final {
    public:
        static void HandlePipe(const std::string &szPipeName);
    };
}
