//
// Created by Pixeluted on 14/10/2024.
//

#include "Security.hpp"
#include <Dependencies/libhat/include/libhat.hpp> // Include libhat for PE parsing

std::shared_ptr<RbxStu::Security> RbxStu::Security::pInstance;

std::shared_ptr<RbxStu::Security> RbxStu::Security::GetSingleton()
{
    if (pInstance == nullptr)
        pInstance = std::make_shared<RbxStu::Security>();

    if (!pInstance->IsInitialized())
        pInstance->Initialize();

    return pInstance;
}

bool RbxStu::Security::IsInitialized()
{
    return this->m_bIsInitialized;
}

std::string RbxStu::Security::HashBytes(const byte* data, const size_t length)
{
    CryptoPP::SHA256 hash;
    byte digest[CryptoPP::SHA256::DIGESTSIZE];

    hash.CalculateDigest(digest, data, length);

    CryptoPP::HexEncoder encoder;
    std::string output;
    encoder.Attach(new CryptoPP::StringSink(output));
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();

    return output;
}

std::string RbxStu::Security::GetHashedMemory()
{
    return this->originalHashedMemory;
}

[[noreturn]] void MemCheckLoop(LPVOID lpBaseOfDll, DWORD SizeOfImage)
{
    while (true == true)
    {
        auto moduleHash = RbxStu::Security::HashModuleSections(lpBaseOfDll, SizeOfImage);

        if (strcmp(moduleHash.c_str(), RbxStu::Security::GetSingleton()->GetHashedMemory().c_str()) != 0)
        {
            MessageBoxA(nullptr, "Bad boy", "You were caught by Hyperion V6", MB_OK);
            TerminateProcess(GetCurrentProcess(), 0);
        }

        Sleep(10000);
    }
}

std::string RbxStu::Security::HashModuleSections(LPVOID lpBaseOfDll, DWORD SizeOfImage)
{
    try
    {

        auto process = hat::process::get_module(RBXSTU_DLL_NAME);
        if (!process.has_value())
        {
            throw std::runtime_error("Failed to fetch our DLL");
        }

        std::string combinedHash;

        for (const auto& section : { ".text", ".rdata" })
        {
            auto sectionData = process->get_section_data(section);
            auto sectionHash = HashBytes(reinterpret_cast<const byte*>(sectionData.data()), sectionData.size());

            combinedHash += sectionHash;
        }

        return combinedHash;
    }
    catch (const std::exception& ex)
    {
        MessageBoxA(nullptr, ex.what(), "Error in parsing PE sections", MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    return "";
}

void RbxStu::Security::Initialize()
{
    if (this->m_bIsInitialized) return;

    const auto ourModule = GetModuleHandleA(RBXSTU_DLL_NAME);
    if (ourModule == nullptr)
    {
        MessageBoxA(nullptr, "Couldn't find our module for security!", "Error", MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), ourModule, &moduleInfo, sizeof(MODULEINFO)))
    {
        MessageBoxA(nullptr, "Couldn't find our module info for security!", "Error", MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    auto moduleHash = HashModuleSections(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);

    this->originalHashedMemory = moduleHash;
    RbxStuLog(RbxStu::LogType::Information, RbxStu::SecurityName, std::format("Our module hashed is: {}", moduleHash));

    std::thread memCheckLoopThead(MemCheckLoop, moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
    memCheckLoopThead.detach();

    this->m_bIsInitialized = true;
}
