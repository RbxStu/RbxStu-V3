//
// Created by Dottik on 13/10/2024.
//

#include "Rbx.hpp"

#include <Analysis/Disassembler.hpp>
#include <Logger.hpp>
#include <Utilities.hpp>
#include <cstdint>
#include <libhat/Signature.hpp>
#include <mutex>

#include "Analysis/StringMatcher.hpp"
#include "Analysis/StringSearcher.hpp"
#include "Analysis/XrefSearcher.hpp"

std::shared_ptr<RbxStu::Scanners::RBX> RbxStu::Scanners::RBX::pInstance;
std::mutex RbxStuScannersRBXGetSingleton;

std::shared_ptr<RbxStu::Scanners::RBX> RbxStu::Scanners::RBX::GetSingleton() {
    if (RbxStu::Scanners::RBX::pInstance == nullptr)
        RbxStu::Scanners::RBX::pInstance = std::make_shared<RbxStu::Scanners::RBX>();

    if (!RbxStu::Scanners::RBX::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuScannersRBXGetSingleton};

        if (RbxStu::Scanners::RBX::pInstance->IsInitialized())
            return RbxStu::Scanners::RBX::pInstance;
        RbxStu::Scanners::RBX::pInstance->Initialize();
    }

    return RbxStu::Scanners::RBX::pInstance;
}

static __inline std::map<RbxStuOffsets::OffsetKey, hat::signature> SignatureMap{
        // RBX::ScriptContext functions.
        {RbxStuOffsets::OffsetKey::RBX_ScriptContext_resume,
         hat::parse_signature("48 8B C4 44 89 48 ? 4C 89 40 ? 48 89 50 ? 48 89 48 ? 53 56 57 41 54 41 55 41 56 41 57 "
                              "48 81 EC ? ? ? ? 0F 29 70 ? 4D 8B E8 48 8B F2 48 8B F9 48 89 8C 24 ? ? ? ? 48 89 8C 24 "
                              "? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 89 44 24 ?")
                 .value()},
        {RbxStuOffsets::OffsetKey::RBX_ScriptContext_getDataModel,
         hat::parse_signature("48 83 EC ?? 48 85 C9 74 72").value()},
        {RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_spawn,
         hat::parse_signature(
                 "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 33 FF 48 89 7C 24 ? 4C 8D 4C 24 ? 4C 8D 05 ? ? ? "
                 "? "
                 "33 D2 E8 ? ? ? ? 8B D8 48 8B CE E8 ? ? ? ? 44 8B CB 4C 8D 44 24 ? 48 8D 54 24 ? 48 8B C8 E8 ? ? ? ?")
                 .value()},
        {RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer,
         hat::parse_signature("48 8B C4 48 89 58 ?? 55 56 57 41 56 41 57 48 83 EC 70")
                 .value()},
        {RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState,
         hat::parse_signature("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F8 48 8B F2 48 8B D9 80 ?? ?? ?? ?? ?? "
                              "?? 74 ?? 8B 81 ? ? ? ? 90 83 F8 03 7C 0F ?? ?? ?? ?? ?? ?? ?? 33 C9 E8 ? ? ? ? 90 ?? ?? "
                              "?? ?? ?? ?? ?? 4C 8B C7 48 8B D6 E8 ? ? ? ? 48 05 88 00 00 00")
                 .value()},


        // RBX::Instance functions
        {RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance,
         hat::parse_signature("48 89 5C 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 E8 ? "
                              "? ? ? 48 8B CB 84 C0 74 12 48 8B D7 48 8B 5C 24 ? 48 83 C4 ? 5F E9 ? ? ? ?")
                 .value()},
        {RbxStuOffsets::OffsetKey::RBX_Instance_getTopAncestor,
         hat::parse_signature("48 8B 41 ? 48 85 C0 74 08 48 8B C8 E9 EF FF FF FF 48 8B C1 C3 CC CC").value()},
        {RbxStuOffsets::OffsetKey::RBX_Instance_removeAllChildren,
         hat::parse_signature("48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B 41 ? 48 85 C0 74 70 66 66 0F 1F 84 00 00 00 "
                              "00 00 48 8B 48 ? 48 8B 59 ? 48 85 DB 74 08")
                 .value()},

        /*
         *  There's a SPECIFIC way to update these bad boys.
         *  You want to find the EventDescriptor related to them, you can find it in initialization using the name of
         * the event, "MouseClick" will do, after which find the first code xref. In the code xref, there will be a call
         * to a function, whose first parameter is the event descriptor. You want to select that one, and go into code
         * references, and there will be a group of approximately 5 or 6 pointers next to each other, those are the
         * events for fireClick, fireRightClick and fireMouseHover and fireMouseLeave.
         *
         *   ~ FireMouseClick/FireRightMouseClick:
         *      - Find the function with three arguments, void*, float, void* (clickDetector, distance, player)
         *          - In it there will be an if statement checking something on the first argument
         *              (MaxActivationDistance), the two functions have the same AOB, with alittle mutation being the
         * event pointer.
         *
         *   - FireMouseHoverEnter/Leave
         *      - Find the function which does NOT contain both, that appears to be a fire redirection of sorts, IGNORE
         * IT.
         *          - There will only be two xrefs in normal conditions, you must ONLY get the proper one to AOB.
         *          - After that is done, the arguments for such function are void *, void * (clickDetector, player)
         *
         *   IT IS TO BE NOTED THAT SENDING A PLAYER THAT IS NOT LOCAL PLAYER MAY HAVE UNINTENDED CONSEQUENCES;
         *   AND IT WILL CERTAINLY NOT FIRE THE EVENT FOR THAT PLAYER, BELIEVE ME.
         */


        {RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseHover,
         hat::parse_signature(
                 "48 89 5C 24 ?? 48 89 74 24 ?? 55 57 41 54 41 56 41 57 48 8B EC 48 81 EC 80 00 00 00 4C 8B F2")
                 .value()},
        {RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseLeave,
         hat::parse_signature("48 89 5C 24 ?? 48 89 74 24 ?? 55 57 41 54 41 56 41 57 48 8D 6C 24 ?? 48 81 EC 90 00 00 00 4C 8B F2 4C 8B F9 45 33 E4")
                 .value()},

        // RBX::BasePart
        {RbxStuOffsets::OffsetKey::RBX_BasePart_getNetworkOwner,
         hat::parse_signature("48 8B 81 ? ? ? ? 8B 88 ? ? ? ? 48 8B C2 89 0A C3").value()},

        // RBX::World
        {RbxStuOffsets::OffsetKey::RBX_World_reportTouchInfo, // "new overlap in different world", contains RBX::World
         // from RBX::Primitive offset.
         hat::parse_signature("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 48 83 79 ?? 00").value()},

        // RBX::ProximityPrompt
        {RbxStuOffsets::OffsetKey::RBX_ProximityPrompt_onTriggered,
         hat::parse_signature(
                 "48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC B0 ?? ?? ?? 4C 8B F9 E8")
                 .value()},

        // Misc
        {RbxStuOffsets::OffsetKey::RBXCRASH,
         hat::parse_signature("48 89 5C 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B FA 48 8B D9 48 8B "
                              "05 ? ? ? ? 48 85 C0 "
                              "74 0A FF D0 84 C0 0F 84 82 03 00 ? E8 ? ? ? ?")
                 .value()},


        {RbxStuOffsets::OffsetKey::RBX_Console_StandardOut,
         hat::parse_signature(
                 ""
                 "48 8B C4 48 89 50 ? 4C 89 40 ? 4C 89 48 ? 53 48 83 EC ? 8B D9 4C 8D 40 ? 48 8D 48 ? E8 "
                 "? ? ? ? 90 33 C0 48 89 44 24 ? 48 C7 44 24 ? ? ? ? ? 48 89 44 24 ? 88 44 24 ? 44 8D 48 ? "
                 "4C 8D 44 24 ? 48 8D 54 24 ? 8B CB E8 ? ? ? ? 90 48 8B 54 24 ? 48 83 FA 10 72 36 48 FF C2 "
                 "48 8B 4C 24 ? 48 8B C1 48 81 FA 00 10 00 00 72 1C 48 83 C2 ? 48 8B 49 ? 48 2B C1 48 83 C0 "
                 "? 48 83 F8 1F 76 07 FF 15 ? ? ? ?")
                 .value()},
    {RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper,
        hat::parse_signature("40 57 48 83 EC 20 48 8B F9 48 85 C9 74 ?? 48 8B 49").value()
    }

};

void RbxStu::Scanners::RBX::Initialize() {
    if (this->m_bIsInitialized)
        return;
    const auto initializationBegin = std::chrono::high_resolution_clock::now();

    auto rbxStuDissassembler = RbxStu::Analysis::Disassembler::GetSingleton();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX, "Scanning...");

    auto foundSignatures = std::map<std::string_view, const void *>();

    const auto scanningBegin = std::chrono::high_resolution_clock::now();
    for (const auto &[enumKey, address]: RbxStu::Utilities::ScanMany(SignatureMap, true, ".text")) {
        const auto name = OffsetKeyToString(enumKey);
        SetOffset(enumKey, address.get());

        if (address.has_result()) {
            foundSignatures.emplace(name, address.get());
        } else {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      std::format("Failed to find ROBLOX function '{}'!", name));
        }
    }


    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("Simple Scan completed in {}ms!",
                          std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::high_resolution_clock::now() - scanningBegin)
                                  .count()));
    for (const auto &[funcName, funcAddress]: foundSignatures) {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  std::format("- {} --> {}", funcName, funcAddress));
    }

    {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  "Scanning for functions required for fireclickdetector...");

        const auto funcSignature =
                hat::parse_signature(
                        "48 8B C4 55 41 54 41 56 41 57 48 8D 68 ?? 48 81 EC 88 00 00 00 F3 0F 10 81 ?? ?? ?? ?? "
                        "45 33 E4 0F 2F C1 4D 8B F0 44 89 65 ?? 4C 8B F9 0F 86 ?? ?? ?? ?? 48 89 58")
                        .value();
        auto hProcess = hat::process::get_process_module();

        auto results = hat::find_all_pattern(hProcess.get_section_data(".text").begin(),
                                             hProcess.get_section_data(".text").end(), funcSignature);

        std::uintptr_t biggestAddress = 0x0;
        std::uintptr_t smallestAddress = SIZE_MAX;

        for (const auto &match: results) {
            /*
             *  RightMouseClick and MouseClick, whilst having the SAME function signature, literally, have a slight
             * difference, and it mostly relates to their position on the binary. MouseClick will normally have a
             * "lower" position (Closer to ImageBase than RightMouseClick).
             */

            if (biggestAddress < reinterpret_cast<std::uintptr_t>(match.get())) {
                biggestAddress = reinterpret_cast<std::uintptr_t>(match.get());
            }

            if (smallestAddress > reinterpret_cast<std::uintptr_t>(match.get())) {
                smallestAddress = reinterpret_cast<std::uintptr_t>(match.get());
            }
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  std::format("Determinated FireMouseClick to be {}, and for FireRightMouseClick to be {}",
                              (void *) smallestAddress, (void *) biggestAddress));

        SetOffset(RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireClick, reinterpret_cast<void *>(smallestAddress));
        SetOffset(RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireRightClick, reinterpret_cast<void *>(biggestAddress));
    }

    {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  "Scanning for pointer offsets (Offsets used by callers to functions, de-offsetted at callee)...");
        {
            if (auto insns = rbxStuDissassembler->GetInstructions(
                           foundSignatures["RBX::ScriptContext::resume"],
                           reinterpret_cast<void *>(
                                   reinterpret_cast<std::uintptr_t>(foundSignatures["RBX::ScriptContext::resume"]) - 0xFF),
                           true);
                   insns.has_value()) {
                auto instructions = std::move(insns.value());

                if (!instructions->ContainsInstruction("lea", "rbx, [rdi -", true)) {
                    RbxStuLog(RbxStu::LogType::Error, RbxStu::Scanners_RBX,
                              "Failed to find instruction lea rbx, [rdi - ...] to crack RBX::ScriptContext::resume pointer "
                              "offsetting.");
                    throw std::exception(
                            "Cannot proceed: Failed to dump pointer offset for RBX::ScriptContext! (Required)");
                }

                const auto insn = instructions->GetInstructionWhichMatches("lea", "rbx, [rdi -", true);
                const auto instruction = insn.value();
                RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                          std::format("Found pointer offset for RBX::ScriptContext as {:#x}",
                                      instruction.detail->x86.operands[1].mem.disp));

                const auto disposition = instruction.detail->x86.operands[1].mem.disp;

                this->m_pointerObfuscation[PointerOffsets::RBX_ScriptContext_resume] = {
                    // We must invert the pointer, why? Because it's the contrary operation for the caller that the
                    // callee does.
                    disposition < 0 ? ::RBX::PointerEncryptionType::SUB : ::RBX::PointerEncryptionType::ADD,
                    disposition};
                   } else {
                       RbxStuLog(RbxStu::LogType::Error, RbxStu::Scanners_RBX,
                                 "Cannot get instructions for RBX::ScriptContext::resume!");
                       throw std::exception("Cannot proceed: Failed to dump pointer offset for RBX::ScriptContext! (Required)");
                   }
        }
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("Scanning for RBX::FFlag declarations...",
                          std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::high_resolution_clock::now() - scanningBegin)
                                  .count()));

    const auto fastFlagScanBegin = std::chrono::high_resolution_clock::now();

    const auto data = hat::process::get_process_module().get_section_data(".text");
    auto patterns = hat::find_all_pattern(
            data.begin(), data.end(),
            hat::compile_signature<"CC CC 41 B8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 0D ? ? ? ? E9 ? ? ? ? CC CC">());


    auto disassembler = RbxStu::Analysis::Disassembler::GetSingleton();
    for (const auto &scanned: patterns) {
        auto result = scanned.get();
        auto possibleInsns = disassembler->GetInstructions(
                result, reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(result) - 0x1b), true);

        if (!possibleInsns.has_value()) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      "Failed to disassemble flag function! The flag this function references may be unavailable");
            continue;
        }

        const auto insns = std::move(possibleInsns.value());

        const auto possibleLoadDataReference = insns->GetInstructionWhichMatches("lea", "rdx, [rip + ", true);
        const auto possibleLoadFlagName = insns->GetInstructionWhichMatches("lea", "rcx, [rip + ", true);
        if (!possibleLoadDataReference.has_value() || !possibleLoadFlagName.has_value()) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      "Cannot find the required assembly without them being "
                      "coupled! Function analysis may not continue.");
            continue;
        }
        const auto loadNameInsn = possibleLoadFlagName.value();
        const auto loadDataInsn = possibleLoadDataReference.value();

        const auto flagNameReference = disassembler->TranslateRelativeLeaIntoRuntimeAddress(loadNameInsn);
        const auto dataReference = disassembler->TranslateRelativeLeaIntoRuntimeAddress(loadDataInsn);

        if (!dataReference.has_value() || !flagNameReference.has_value()) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      "Failed to translate the RIP-based offset into a memory address for any of the "
                      "LEA operations! This will result on bad things, thus, we cannot continue trying "
                      "to get this flag :(");
            continue;
        }

        this->m_FastFlagMap[static_cast<const char *>(flagNameReference.value())] =
                const_cast<void *>(dataReference.value());
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("FastFlags Scanning Completed in {}ms, {} fast flags were found to be valid!",
                          std::chrono::duration_cast<std ::chrono::milliseconds>(
                                  std::chrono::high_resolution_clock::now() - fastFlagScanBegin)
                                  .count(),
                          this->m_FastFlagMap.size()));

    {
        auto ktableScanBegin = std::chrono::high_resolution_clock::now();
        // Trying to call method on object of type: `%s` with incorrect arguments. (Updata AOB when needed)
        const auto data = hat::process::get_process_module().get_section_data(".text");
        auto scanResult =
                hat::find_all_pattern(data.begin(), data.end(),
                                      hat::compile_signature<"48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC "
                                                             "30 48 8B DA C7 44 24 ?? FF FF FF FF">());

        for (const auto matches: scanResult) {
            auto possibleInsns = disassembler->GetInstructions(
                    matches.get(), reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(matches.get()) - 0x50),
                    true);

            if (!possibleInsns.has_value())
                continue;
            const auto insns = std::move(possibleInsns.value());
            auto insn = insns->GetInstructionWhichMatches("lea", "rcx, [rip + ", true);

            this->m_RbxNamektable = static_cast<void **>(
                    const_cast<void *>(disassembler->TranslateRelativeLeaIntoRuntimeAddress(insn.value()).value()));
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  std::format("End of scan for RBX::Name::ktable in {}ms! Found RBX::Name::ktable @ {}!",
                              std::chrono::duration_cast<std ::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - ktableScanBegin)
                                      .count(),
                              static_cast<void *>(this->m_RbxNamektable)))
    }

    {
        const auto classDescriptorScanBegin = std::chrono::high_resolution_clock::now();

        const auto data = hat::process::get_process_module().get_section_data(".text");
        auto scanResult = hat::find_all_pattern(data.begin(), data.end(),
                                                hat::compile_signature<"0F 29 44 24 ? 0F 10 4C 24 ? 0F 29 4C ?">());

        for (const auto &result: scanResult) {
            auto describedName = "";
            auto leaopstr = "";
            x86_reg descriptorContainerRegister;
            void *descriptorPointer = nullptr;
            { // Step 1/2: get describedName name
                if (auto possibleInsns = disassembler->GetInstructions(
                            result.get(),
                            reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(result.get()) - 0x60), true);
                    possibleInsns.has_value()) {
                    auto insns = std::move(possibleInsns.value());
                    const auto callInstruction = insns->GetInstructionWhichMatches("call", nullptr, true);

                    if (!callInstruction.has_value() ||
                        !insns->ContainsInstruction("lea", "r8, [rip +", true) // Load string (Name)
                        || !insns->ContainsInstruction(nullptr, "rdx,", true) ||
                        !insns->ContainsInstruction("mov", "rcx,", true) // Load data pointer (ClassDescriptor)
                    ) {
                        continue;
                    }

                    auto leaInsn = insns->GetInstructionWhichMatches("lea", "r8, [rip +", true);
                    describedName = static_cast<const char *>(
                            disassembler->TranslateRelativeLeaIntoRuntimeAddress(leaInsn.value()).value());

                    auto loadPointerOptional = insns->GetInstructionWhichMatches(
                            "mov", "rcx,", true); // We must read from which register the data is being moved.
                    auto loadPointer = loadPointerOptional.value();
                    descriptorContainerRegister = loadPointer.detail->x86.operands[1].reg;
                    leaopstr = loadPointer.op_str;
                } else {
                    continue;
                }
            }

            { // Step 2: dump class descriptor pointer.

                // Walk up the assembly to read `lea ( ? ), [rip + ...]`

                if (auto possibleInsns = disassembler->GetInstructions(
                            disassembler->GetFunctionStart(result.get()),
                            reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(result.get()) + 0x60), true);
                    possibleInsns.has_value()) {
                    const auto insns = std::move(possibleInsns.value());
                    if (!insns->ContainsInstruction("lea", ", [rip +", true))
                        continue; // no lea insn, ignore.

                    for (const auto &insn: insns->GetInstructions()) {
                        if (strcmp(insn.mnemonic, "lea") == 0) {
                            // Load effective address found.
                            if (insn.detail->x86.operands[0].reg == descriptorContainerRegister) {
                                if (auto addy = disassembler->TranslateRelativeLeaIntoRuntimeAddress(insn);
                                    addy.has_value()) {
                                    descriptorPointer = const_cast<void *>(addy.value());
                                    break;
                                }
                            }
                        }
                    }
                    if (descriptorPointer == nullptr) {
                        // printf("DISASSEMBLY FOR %s (MATCHING FOR INSN LEA OPSTR '%s'):\n", describedName, leaopstr);
                        // printf("%s\n", insns->RenderInstructions().c_str());
                        continue;
                    }
                } else {
                    // RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                    //           std::format("Failed to fetch instructions for the given function -- data
                    //                       {} !",
                    //                       describedName));
                }
            }

            this->m_classDescriptorMap[describedName] =
                    static_cast<::RBX::Reflection::ClassDescriptor *>(descriptorPointer);
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  std::format("End of scan for possible RBX::Reflection::ClassDescriptor in {}ms! Obtained {} "
                              "described objects!",
                              std::chrono::duration_cast<std ::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - classDescriptorScanBegin)
                                      .count(),
                              this->m_classDescriptorMap.size()))
    }
    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("ROBLOX + FastFlags + ClassDescriptor + RBX::Name::ktable Scanning Completed! Initialization "
                          "completed in {}ms!",
                          std::chrono::duration_cast<std ::chrono::milliseconds>(
                                  std::chrono::high_resolution_clock::now() - initializationBegin)
                                  .count()));

    this->m_bIsInitialized = true;
}


bool RbxStu::Scanners::RBX::IsInitialized() { return this->m_bIsInitialized; }

std::vector<RBX::Reflection::ClassDescriptor *> RbxStu::Scanners::RBX::GetClassDescriptors() const {
    auto vec = std::vector<::RBX::Reflection::ClassDescriptor *>();
    for (const auto &classDescriptor: this->m_classDescriptorMap | std::views::values) {
        vec.emplace_back(classDescriptor);
    }

    return vec;
}
std::optional<RbxStu::Scanners::RBX::PointerOffsetInformation>
RbxStu::Scanners::RBX::GetRbxPointerOffset(const RbxStu::Scanners::RBX::PointerOffsets offset) {
    if (!this->m_pointerObfuscation.contains(offset))
        return {};

    return this->m_pointerObfuscation.at(offset);
}
