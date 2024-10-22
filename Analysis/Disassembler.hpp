//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <memory>
#include <optional>
#include <vector>

#include <capstone/capstone.h>
#include <format>

namespace RbxStu::Analysis {
    class DisassembledChunk final {
        std::vector<cs_insn> vInstructionsvec;
        cs_insn *originalInstruction;
        std::size_t instructionCount;

    public:
        ~DisassembledChunk();

        DisassembledChunk(_In_ cs_insn *pInstructions, std::size_t ullInstructionCount);

        bool ContainsInstruction(_In_ const char *szMnemonic, _In_ const char *szOperationAsString, bool bUseContains);

        std::optional<const cs_insn> GetInstructionWhichMatches(const char *szMnemonic, const char *szOperationAsString,
                                                                bool bUseContains);

        std::vector<cs_insn> GetInstructions();

        std::string RenderInstructions();
    };

    class Disassembler final {
        static std::shared_ptr<Disassembler> pInstance;
        std::atomic_bool m_bIsInitialized;
        csh m_capstoneHandle{};

        void Initialize();

    public:
        bool IsInitialized();

        std::optional<std::unique_ptr<RbxStu::Analysis::DisassembledChunk> > GetInstructions(
            const void *startAddress, const void *endAddress, bool ignorePageProtection
        ) const;

        const void *GetFunctionStart(const void *address);

        const void *GetFunctionEnd(const void *address);

        std::optional<const void *> TranslateRelativeLeaIntoRuntimeAddress(const cs_insn &insn);

        csh GetCapstoneHandle() const;

        static std::shared_ptr<Disassembler> GetSingleton();
    };
};
