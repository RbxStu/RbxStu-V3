//
// Created by Dottik on 13/10/2024.
//

#include "Rbx.hpp"

#include <Logger.hpp>
#include <mutex>
#include <Utilities.hpp>
#include <Analysis/Disassembler.hpp>
#include <libhat/Signature.hpp>

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
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_resume, hat::parse_signature(
            "48 8B C4 44 89 48 ? 4C 89 40 ? 48 89 50 ? 48 89 48 ? 53 56 57 41 54 41 55 41 56 41 57 "
            "48 81 EC ? ? ? ? 0F 29 70 ? 4D 8B E8 48 8B F2 48 8B F9 48 89 8C 24 ? ? ? ? 48 89 8C 24 "
            "? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 89 44 24 ?"
        ).value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_getDataModel,
        hat::parse_signature("48 83 EC ? 48 85 C9 74 72 48 89 7C 24 ? 48 8B 79 ? 48 85 FF 74 22").value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_validateThreadAccess,
        hat::parse_signature("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B DA 48 8B E9 80")
        .value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_spawn,
        hat::parse_signature(
            "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 33 FF 48 89 7C 24 ? 4C 8D 4C 24 ? 4C 8D 05 ? ? ? ? "
            "33 D2 E8 ? ? ? ? 8B D8 48 8B CE E8 ? ? ? ? 44 8B CB 4C 8D 44 24 ? 48 8D 54 24 ? 48 8B C8 E8 ? ? ? ?").
        value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer,
        hat::parse_signature(
            "48 8B C4 48 89 58 ? 55 56 57 41 56 41 57 48 83 EC ? 48 8B E9 33 FF 48 89 78 ? 4C 8D 48 "
            "? 4C 8D 05 ? ? ? ? 33 D2 E8 ? ? ? ? 44 8B F0 48 8B CD E8 ? ? ? ? 48 8B D8 40 32 F6"
        ).value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState,
        hat::parse_signature("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F8 48 8B F2 48 8B D9 80 ?? ?? ?? ?? ?? "
            "?? 74 ?? 8B 81 ? ? ? ? 90 83 F8 03 7C 0F ?? ?? ?? ?? ?? ?? ?? 33 C9 E8 ? ? ? ? 90 ?? ?? "
            "?? ?? ?? ?? ?? 4C 8B C7 48 8B D6 E8 ? ? ? ? 48 05 88 00 00 00").value()
    },


    // RBX::Instance functions
    {
        RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance,
        hat::parse_signature(
            "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 48 83 3A 00").value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_Instance_getTopAncestor,
        hat::parse_signature("48 8B 41 ? 48 85 C0 74 08 48 8B C8 E9 EF FF FF FF 48 8B C1 C3 CC CC").value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_Instance_removeAllChildren,
        hat::parse_signature("48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B 41 ? 48 85 C0 74 70 66 66 0F 1F 84 00 00 00 "
            "00 00 48 8B 48 ? 48 8B 59 ? 48 85 DB 74 08").value()
    },

    // RBX::BasePart
    {
        RbxStuOffsets::OffsetKey::RBX_BasePart_getNetworkOwner,
        hat::parse_signature("48 8B 81 ? ? ? ? 8B 88 ? ? ? ? 48 8B C2 89 0A C3").value()
    },
    {
        RbxStuOffsets::OffsetKey::RBX_BasePart_fireTouchSignals,
        hat::parse_signature("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 4C 89 74 24 ? 55 48 8B EC 48 81 EC ? ? ? ? 45 "
            "0F B6 F1 41 0F B6 F0 48 8B DA 48 8B F9 E8 ? ? ? ? 48 85 C0 0F 84 6F 01 00 ? 48 8B CF E8 "
            "? ? ? ? 48 8B F8 0F 57 C0 F3 0F 7F 45 ? 48 8B 90 ? ? ? ? 48 85 D2 74 15 8B 42 ?").value()
    },


    // Misc
    {
        RbxStuOffsets::OffsetKey::RBXCRASH,
        hat::parse_signature(
            "48 89 5C 24 ? 48 89 7C 24 ? 48 89 4C 24 ? 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B FA 48 8B D9 "
            "48 8B 05 ? ? ? ? 48 85 C0 74 0A FF D0 84 C0 0F 84 D0 04 00 ? E8 ? ? ? ? 85 C0 0F 84 C3 04 00 ?")
        .value()
    }


};

void RbxStu::Scanners::RBX::Initialize() {
    if (this->m_bIsInitialized) return;
    const auto initializationBegin = std::chrono::high_resolution_clock::now();


    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX, "Scanning...");

    auto foundSignatures = std::map<std::string_view, const void *>();

    const auto scanningBegin = std::chrono::high_resolution_clock::now();
    for (const auto &[enumKey, address]: RbxStu::Utilities::ScanMany(SignatureMap, true)) {
        const auto name = OffsetKeyToString(enumKey);
        SetOffset(enumKey, address.get());
        if (address.has_result()) {
            foundSignatures.emplace(
                name, address.get()
            );
        } else {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      std::format( "Failed to find ROBLOX function '{}'!", name));
        }
    }


    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("Scan completed in {}ms!", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
                  high_resolution_clock::now() - scanningBegin).count()));
    for (const auto &[funcName, funcAddress]: foundSignatures) {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
                  std::format("- {} --> {}", funcName, funcAddress));
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("Scanning for RBX::ScriptContext::getGlobalState pointer offset...", std::chrono::
                  duration_cast<std::chrono::milliseconds>(std::chrono::
                      high_resolution_clock::now() - scanningBegin).count()));

    // We must read the insns disassembled (because its easier, basically RbxStu V2 method)

    auto getGlobalState = RbxStuOffsets::GetSingleton()->GetOffset(
        RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState);

    if (getGlobalState == nullptr) {
        RbxStuLog(RbxStu::LogType::Error, RbxStu::Scanners_RBX,
                  std::format("Cannot dump pointer encryption", std::chrono::
                      duration_cast<std::chrono::milliseconds>(std::chrono::
                          high_resolution_clock::now() - scanningBegin).count()));
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("Scanning for RBX::FFlag declarations...", std::chrono::duration_cast<std::chrono::
                  milliseconds>(std::
                      chrono::
                      high_resolution_clock::now() - scanningBegin).count()));

    const auto fastFlagScanBegin = std::chrono::high_resolution_clock::now();

    const auto data = hat::process::get_process_module().get_section_data(".text");
    auto patterns = hat::find_all_pattern(data.begin(), data.end(),
                                          hat::compile_signature<
                                              "CC CC 41 B8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 0D ? ? ? ? E9 ? ? ? ? CC CC">());


    auto disassembler = RbxStu::Analysis::Disassembler::GetSingleton();
    for (const auto &scanned: patterns) {
        auto result = scanned.get();
        auto possibleInsns = disassembler->GetInstructions(
            result, reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(result) - 0x1b), true);

        if (!possibleInsns.has_value()) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      "Failed to disassemble flag function! The flag this function references may be unavailable"
            );
            continue;
        }

        const auto insns = std::move(possibleInsns.value());

        const auto possibleLoadDataReference = insns->GetInstructionWhichMatches("lea", "rdx, [rip + ", true);
        const auto possibleLoadFlagName = insns->GetInstructionWhichMatches("lea", "rcx, [rip + ", true);
        if (!possibleLoadDataReference.has_value() || !possibleLoadFlagName.has_value()) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_RBX,
                      "Cannot find the required assembly without them being "
                      "coupled! Function analysis may not continue."
            );
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
                      "to get this flag :("
            );
            continue;
        }

        this->m_FastFlagMap[static_cast<const char *>(flagNameReference.value())] = const_cast<void *>(dataReference.
            value());
    }

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("FastFlags Scanning Completed in {}ms, {} fast flags were found to be valid!", std::chrono::
                  duration_cast<std
                  ::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - fastFlagScanBegin).count(), this->
                  m_FastFlagMap.size()));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_RBX,
              std::format("ROBLOX + FastFlags Scanning Completed! Initialization completed in {}ms!", std::chrono::
                  duration_cast<std
                  ::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - initializationBegin).count()));


    this->m_bIsInitialized = true;
}


bool RbxStu::Scanners::RBX::IsInitialized() {
    return this->m_bIsInitialized;
}
