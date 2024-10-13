//
// Created by Dottik on 13/10/2024.
//

#include "Luau.hpp"

#include <Logger.hpp>
#include <map>

#include "Analysis/Disassembler.hpp"
#include <mutex>
#include <Utilities.hpp>
#include <libhat/Scanner.hpp>
#include <libhat/Signature.hpp>

#include <StudioOffsets.h>

std::shared_ptr<RbxStu::Scanners::Luau> RbxStu::Scanners::Luau::pInstance;

static const std::map<RbxStuOffsets::OffsetKey, hat::signature> SignatureMap{
    {
        RbxStuOffsets::OffsetKey::luaD_throw, hat::parse_signature(
            "48 83 EC ? 44 8B C2 48 8B D1 48 8D 4C 24 ? E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 "
            "8D 4C 24 ? E8 ? ? ? ? CC CC CC").value()
    },
    {
        RbxStuOffsets::OffsetKey::luau_execute, hat::parse_signature("80 79 06 00 0F 85 ? ? ? ? E9 ? ? ? ? CC").value()
    },
    {
        RbxStuOffsets::OffsetKey::lua_pushvalue, hat::parse_signature(
            "48 89 5C 24 ? 57 48 83 EC ? F6 41 01 04 48 8B D9 48 63 FA 74 0C 4C 8D 41 ? 48 8B D1 E8 "
            "? ? ? ? 85 FF 7E 24 48 8B 43 ? 48 8B CF 48 C1 E1 ? 48 83 C0 ? 48 03 C1 48 8B 4B ? 48 "
            "3B C1 72 2F 48 8D 05 ? ? ? ? EB 26 81 FF F0 D8 FF FF 7E 10").value()
    },
    {
        RbxStuOffsets::OffsetKey::luaE_newthread, hat::parse_signature(
            "48 89 5C 24 ? 57 48 83 EC ? 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? "
            "? 48 8B 57 ? 48 8B D8 44 0F B6 42 ? C6 00 ? 41 80 E0 ? 44 88 40 ?").value()
    },
    {
        RbxStuOffsets::OffsetKey::luaC_step,
        hat::parse_signature("48 8B 59 ? B8 ? ? ? ? 0F B6 F2 0F 29 74 24 ? 4C 8B F1 44 8B 43 ?").value()
    },
    {
        RbxStuOffsets::OffsetKey::luaD_rawrununprotected,
        hat::parse_signature("48 89 4C 24 ? 48 83 EC ? 48 8B C2 49 8B D0 FF D0 33 C0 EB 04 8B 44 24 48 48 83 C4 ? C3").
        value()
    },
    {
        RbxStuOffsets::OffsetKey::luaH_new,
        hat::parse_signature("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B F0 8B "
            "EA 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? ? 4C 8B 4F ? 48 8B D8 45 0F B6 "
            "51 ? C6 00 ? 41 80 E2 ? 44 88 50 ? 44 0F B6 47 ? 44 88 40 ?").value()
    },
    {
        RbxStuOffsets::OffsetKey::freeblock,
        hat::parse_signature(
            "4C 8B 51 ? 49 83 E8 ? 44 8B CA 4C 8B D9 49 8B 10 48 83 7A 28 00 75 22 83 7A 30 00 7D 1C 49 63 C1").value()
    },
    {
        RbxStuOffsets::OffsetKey::luaV_settable,

        hat::parse_signature(
            "48 89 5C 24 ? 48 89 6C 24 ? 56 41 54 41 57 48 83 EC ? 48 89 7C 24 ? 4D 8B E1 4C 89 74 "
            "24 ? 4D 8B F8 48 8B F2 48 8B D9 33 ED 0F 1F 44 00 00 83 7E 0C 06 75 4C").value()
    },
    {
        RbxStuOffsets::OffsetKey::luaV_gettable,

        hat::parse_signature(
            "48 89 5C 24 ? 55 41 54 41 55 41 56 41 57 48 83 EC ? 48 89 74 24 ? 4C 8D 2D ? ? ? ? 48 "
            "89 7C 24 ? 4D 8B E1 4D 8B F8 48 8B DA 4C 8B F1 33 ED 83 7B 0C 06 75 76").value()
    },
    {
        RbxStuOffsets::OffsetKey::luau_load,
        hat::parse_signature(
            "48 89 5C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 49 8B E9 4D 8B F0 48 8B "
            "FA 4C 8B F9 45 0F B6 28 44 88 6C 24 ? BE ? ? ? ? 45 84 ED 75 4C 48 C7 C3 ? ? ? ? 48 FF C3 80 3C 1A 00 "
            "75 F7 4C 8B CB 4C 8B C7 BA ? ? ? ? 48 8D 8C 24 ? ? ? ? E8 ? ? ? ? 49 8D 4E ? 44 8D 4D ? 48 89 4C 24 ? "
            "4C 8B C0 48 8D 15 ? ? ? ? 49 8B CF E8 ? ? ? ? E9 01 0E 00 00 41 8D 45 ? 3C 03 0F 87 A8 0D 00 00 48 8B "
            "69 ? 48 89 6C 24 ? 48 8B 45 ? 48 39 45 48 72 12").value()
    },

};

std::mutex RbxStuScannersLuauGetSingleton;

bool RbxStu::Scanners::Luau::IsInitialized()
{
    return this->m_bIsInitialized;
}

void RbxStu::Scanners::Luau::Initialize()
{
    if (this->m_bIsInitialized) return;

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_Luau, "Scanning...");

    auto foundSignatures = std::map<std::string_view, const void*>();

    const auto scanningBegin = std::chrono::high_resolution_clock::now();
    for (const auto& [enumKey, address] : RbxStu::Utilities::ScanMany(SignatureMap, true))
    {
        auto name = OffsetKeyToString(enumKey);
        if (address.has_result()) {
            foundSignatures.emplace(
                name, address.get()
            );
            SetOffset(enumKey, address.get());
        } else {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scanners_Luau,
                      std::format( "Failed to find Luau C function '{}'!", name));
        }
    }


    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_Luau,
              std::format("Scan completed in {}ms!", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
                  high_resolution_clock::now() - scanningBegin).count()));
    for (const auto& [funcName, funcAddress] : foundSignatures)
    {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_Luau,
                  std::format("- {} --> {}", funcName, funcAddress));
    }

    lua_State *luaState = luaL_newstate();
    try {
        auto lua_pushvalue = static_cast<void *(__fastcall *)(lua_State *L, int32_t lua_index)>(
            RbxStuOffsets::GetSingleton()->GetOffset(
                RbxStuOffsets::OffsetKey::lua_pushvalue));
        auto luaH_new = static_cast<void *(__fastcall *)(void *L, int32_t narray, int32_t nhash)>(
            RbxStuOffsets::GetSingleton()->GetOffset(
                RbxStuOffsets::OffsetKey::luaH_new));

#undef luaO_nilobject
        SetOffset(RbxStuOffsets::OffsetKey::luaO_nilobject,
                  lua_pushvalue(luaState, 1));

        auto table = luaH_new(luaState, 0, 0);
        SetOffset(RbxStuOffsets::OffsetKey::_luaH_dummynode,
                  reinterpret_cast<void *>(static_cast<Table *>(table)->node));

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_Luau,
                  std::format("- luaO_nilobject: {}", RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey
                      ::luaO_nilobject)));
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scanners_Luau,
                  std::format("- luaH_dummyNode: {}", RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey
                      ::_luaH_dummynode)));
    } catch (const std::exception &ex) {
        RbxStuLog(RbxStu::LogType::Error, RbxStu::Scanners_Luau,
                  std::format(
                      "failed to retrieve luaO_nilObject and luaH_dummyNode -> '{}'; Were lua_pushvalue or luaH_new not found during the scanning stage?"
                      , ex.what()));
    }

    lua_close(luaState);


    this->m_bIsInitialized = true;
}

std::shared_ptr<RbxStu::Scanners::Luau> RbxStu::Scanners::Luau::GetSingleton()
{
    if (nullptr == RbxStu::Scanners::Luau::pInstance)
        RbxStu::Scanners::Luau::pInstance = std::make_shared<RbxStu::Scanners::Luau>();

    if (!RbxStu::Scanners::Luau::pInstance->IsInitialized())
    {
        std::scoped_lock lock{RbxStuScannersLuauGetSingleton};
        if (RbxStu::Scanners::Luau::pInstance->IsInitialized())
            return RbxStu::Scanners::Luau::pInstance;

        RbxStu::Scanners::Luau::pInstance->Initialize();
    }
    return RbxStu::Scanners::Luau::pInstance;
}
