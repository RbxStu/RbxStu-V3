//
// Created by Dottik on 19/10/2024.
//

#include "XrefSearcher.hpp"

#include <Logger.hpp>
#include <mutex>
#include <print>
#include <sstream>

#include "Capstone/arch/X86/X86Mapping.h"

namespace RbxStu::Analysis {
    std::mutex RbxStuAnalysisXrefSearcherGetSingleton;

    std::shared_ptr<RbxStu::Analysis::XrefSearcher> RbxStu::Analysis::XrefSearcher::pInstance;

    std::shared_ptr<RbxStu::Analysis::XrefSearcher> XrefSearcher::GetSingleton() {
        if (nullptr == RbxStu::Analysis::XrefSearcher::pInstance)
            RbxStu::Analysis::XrefSearcher::pInstance = std::make_shared<RbxStu::Analysis::XrefSearcher>();

        return RbxStu::Analysis::XrefSearcher::pInstance;
    }

    bool XrefSearcher::BootstrapXrefsForModule(const hat::process::module hModule) {
        const auto textSection = hModule.get_section_data(".text");
        auto textSectionStart = reinterpret_cast<const uint8_t *>(textSection.data());
        auto textSectionStartAddress = reinterpret_cast<std::uintptr_t>(textSectionStart);
        const auto textSectionEnd = textSection.data() + textSection.size_bytes();

        const auto rbxStuDisassembler = RbxStu::Analysis::Disassembler::GetSingleton();

        RbxStuLog(RbxStu::LogType::Warning, RbxStu::Analysis_XrefSearcher,
                  "Pre-Analyzing .text in search of possible cross references! This may take a while...");

        const auto capstoneHandle = rbxStuDisassembler->GetCapstoneHandle();

        const auto operationStart = std::chrono::high_resolution_clock::now(); // timer shit
        size_t codeSize = textSection.size_bytes();
        const auto possibleXrefInstruction = cs_malloc(capstoneHandle);
        std::stringstream strstream{};

        // dissassemble instructions in an iterator order to prevent exhausting the OS out of PRECIOUS RAM (cannot make chrome jealous).
        while (cs_disasm_iter(capstoneHandle, &textSectionStart, &codeSize,
                              &textSectionStartAddress, possibleXrefInstruction)) {
            if (possibleXrefInstruction->detail->x86.operands[0].reg == X86_REG_RAX && possibleXrefInstruction->id ==
                ::x86_insn::X86_INS_LEA && possibleXrefInstruction->detail
                ->x86.operands[1].mem.base == X86_REG_RIP) {
                // Possibly an obfuscated xref. Resolve by looking into the future 3-4 instructions, these three instructions will perform
                // light math on the pointer to move and displace it into a correct position, this could be considered a certain protection
                // from dumper or analysis, but most RE tools handle it alright, but I do not, so I gotta handle it :(

                auto insns = rbxStuDisassembler->GetInstructions(
                    reinterpret_cast<void *>(possibleXrefInstruction->address),
                    reinterpret_cast<void *>(
                        possibleXrefInstruction->address + possibleXrefInstruction->size * 16), true);

                if (insns.has_value()) {
                    const auto chunk = std::move(insns.value());
                    auto instructions = chunk->GetInstructions();

                    auto pointerObfuscationOffset = 0x0ll;

                    for (const auto &instruction: instructions) {
                        if (instruction.id == ::x86_insn::X86_INS_SUB) {
                            // this is the operation in charge of offsetting the initial LEA into the REAL pointer of the string, we must save it into our data.
                            // this instruction will make an operation on RDX, a substraction after reading it from the function stack frame, but we only need the information that we are offsetting and the original
                            // pointer to perform the required arithmetic.
                            if (instruction.detail->x86.operands[0].reg == ::x86_reg::X86_REG_RDX) {
                                pointerObfuscationOffset = instruction.detail->x86.operands[1].imm;
                            }
                        }
                    }
                    if (pointerObfuscationOffset != 0 && pointerObfuscationOffset != 38) {
                        // -38 => Garbage.
                        auto insnInformation = std::make_shared<RbxStuXRefInformation>();
                        auto dispositioned = possibleXrefInstruction->address +
                                             possibleXrefInstruction->detail->
                                             x86.operands[1].mem.disp + possibleXrefInstruction->size;

                        const auto finalAddress = reinterpret_cast<void *>(dispositioned - pointerObfuscationOffset);

                        insnInformation->kind = XrefKind::Obscured_LEARAX;
                        insnInformation->pointsTo = finalAddress;

                        // pointerObfuscationOffset is already NEGATIVE.
                        insnInformation->instructionPointer = reinterpret_cast<void *>(textSectionStartAddress);

                        this->m_instructionBuffer.emplace_back(insnInformation);

                        continue;
                    }
                } else {
                    RbxStuLog(RbxStu::LogType::Warning, RbxStu::Analysis_XrefSearcher,
                              "XREF obfuscation analysis failed.");
                }
            }

            // The op must be a mem one.
            if (possibleXrefInstruction->detail->x86.operands[1].type != x86_op_type::X86_OP_MEM) continue;

            // The base to offset from must be RIP.
            if (possibleXrefInstruction->detail->x86.operands[1].mem.base != X86_REG_RIP)
                continue;

            const auto disposition = possibleXrefInstruction->detail->x86.operands[1].mem.disp;
            const auto relativePointer = disposition + possibleXrefInstruction->address + possibleXrefInstruction->size;

            auto insnInformation = std::make_shared<RbxStuXRefInformation>();
            insnInformation->kind = XrefKind::Direct;
            insnInformation->pointsTo = reinterpret_cast<void *>(relativePointer);
            insnInformation->instructionPointer = reinterpret_cast<void *>(textSectionStartAddress);

            this->m_instructionBuffer.emplace_back(insnInformation);
        }

        cs_free(possibleXrefInstruction, 1); // thank you capstone :)

        auto obscuredXrefs = 0;
        for (const auto &instruction: this->m_instructionBuffer)
            if (instruction->kind != XrefKind::Direct)
                obscuredXrefs += 1;

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_XrefSearcher,
                  std::format(
                      "Found {} valid instructions performing relative loads from RIP! Keeping instructions on registry; Analysis statistics: Obscured: {}, Direct: {}. Operation took {}ms to be completed."
                      , this->m_instructionBuffer.size(), obscuredXrefs, this->m_instructionBuffer.size() -
                      obscuredXrefs, std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::high_resolution_clock::now() -
                          operationStart).count()));
        return true;
    }

    std::vector<void *> XrefSearcher::GetXrefsForPointer(const void *pointer) {
        std::vector<void *> results{};

        for (const auto &instruction: this->m_instructionBuffer) {
            if (instruction->pointsTo == pointer) {
                results.push_back(instruction->instructionPointer);
            }
        }

        return results;
    }
} // RbxStu::Analysis
