//
// Created by Dottik on 19/10/2024.
//

#pragma once
#include <memory>
#include <vector>
#include <libhat/Process.hpp>

#include "Disassembler.hpp"
#include "capstone/capstone.h"

namespace RbxStu::Analysis {
    enum class XrefKind {
        Direct,
        Obscured_LEARAX
    };

    struct RbxStuXRefInformation {
        XrefKind kind;
        void *pointsTo;
        void *instructionPointer;
    };

    class XrefSearcher {
        static std::shared_ptr<RbxStu::Analysis::XrefSearcher> pInstance;


        std::vector<std::shared_ptr<RbxStuXRefInformation> > m_instructionBuffer;

    public:
        static std::shared_ptr<RbxStu::Analysis::XrefSearcher> GetSingleton();

        bool BootstrapXrefsForModule(hat::process::module hModule);

        std::vector<void *> GetXrefsForPointer(const void *pointer);
    };
} // RbxStu::Analysis
