//
// Created by Pixeluted on 14/10/2024.
//

#include <LazyImporter.hpp>

#include "Security.hpp"

#include <random>

__forceinline const char* generateRandomString(int length) {
    const char charset[] = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char* result = new char[length + 1];

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, strlen(charset) - 1);

    for (int i = 0; i < length; ++i) {
        result[i] = charset[distribution(generator)];
    }
    result[length] = '\0';

    return result;
}

std::shared_ptr<RbxStu::Security> RbxStu::Security::pInstance;

std::shared_ptr<RbxStu::Security> RbxStu::Security::GetSingleton() {
    if (pInstance == nullptr)
        pInstance = std::make_shared<RbxStu::Security>();

    if (!pInstance->IsInitialized())
        pInstance->Initialize();

    return pInstance;
}

bool RbxStu::Security::IsInitialized() {
    return this->m_bIsInitialized;
}

std::string RbxStu::Security::HashBytes(const byte *data, const size_t length) {
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

const char *RbxStu::Security::GetHashedMemory() const {
    return this->securityContext->originalHashedMemory;
}

[[noreturn]] void MemCheckLoop(LPVOID lpBaseOfDll, DWORD SizeOfImage) {
    while (!oxorany(false) == oxorany(true)) {
        auto moduleHash = RbxStu::Security::HashModuleSections(lpBaseOfDll, SizeOfImage);
        *RbxStu::Security::GetSingleton()->securityContext->lastRan = std::time(nullptr);

        if (strcmp(moduleHash.c_str(), RbxStu::Security::GetSingleton()->GetHashedMemory()) != 0) {
            std::thread([] {
                        MessageBoxA(nullptr, oxorany_converted("Bad Boy"),
                                    oxorany_converted("You were caught by Hyperion V6"),
                                    MB_OK | MB_TOPMOST);
                    })
                    .
                    detach();
            Sleep(oxorany(1000));

            if (oxorany(1) == oxorany(1))
                TerminateProcess(GetCurrentProcess(), 0);
        }

        if (oxorany(1) == oxorany(1)) {
            if (oxorany(1) == oxorany(1)) {
                if (oxorany(1) == oxorany(1))
                    Sleep(oxorany(5000));
                else if (oxorany(1) != oxorany(1))
                    if (oxorany(1) != oxorany(1) && oxorany(1) == oxorany(1))
                        Sleep(oxorany(-1));
            }
        }
    }
}

std::string RbxStu::Security::HashModuleSections(LPVOID lpBaseOfDll, DWORD SizeOfImage) {
    try {
        auto process = hat::process::get_module(RbxStu::Utilities::GetCurrentDllName());
        if (!process.has_value()) {
            throw std::runtime_error(oxorany_converted("Failed to fetch our DLL"));
        }

        std::string combinedHash;

        for (const auto &section: {oxorany_converted(".text"), oxorany_converted(".rdata")}) {
            auto sectionData = process->get_section_data(section);
            auto sectionHash = HashBytes(reinterpret_cast<const byte *>(sectionData.data()),
                                         sectionData.size());

            combinedHash += sectionHash;
        }

        return combinedHash;
    } catch (const std::exception &ex) {
        MessageBoxA(nullptr, ex.what(), oxorany_converted("Error in parsing PE sections"), MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    return "";
}

void RbxStu::Security::Initialize() {
    if (this->m_bIsInitialized) return;

    const auto ourModule = GetModuleHandleA(RbxStu::Utilities::GetCurrentDllName().c_str());
    if (ourModule == nullptr) {
        MessageBoxA(nullptr, "Couldn't find our module for security!", "Error", MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), ourModule, &moduleInfo, sizeof(MODULEINFO))) {
        MessageBoxA(nullptr, "Couldn't find our module info for security!", "Error", MB_OK);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    auto moduleHash = HashModuleSections(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);

    this->securityContext->filler_1 = _strdup(generateRandomString((rand() % 128) + 1));
    this->securityContext->filler_2 = _strdup(generateRandomString((rand() % 128) + 1));
    this->securityContext->filler_3 = _strdup(generateRandomString((rand() % 128) + 1));
    this->securityContext->filler_4 = _strdup(generateRandomString((rand() % 128) + 1));
    this->securityContext->filler_5 = _strdup(generateRandomString((rand() % 128) + 1));
    this->securityContext->filler_6 = _strdup(generateRandomString((rand() % 128) + 1));

    this->securityContext->originalHashedMemory = _strdup(moduleHash.c_str());
    this->securityContext->lastRan = static_cast<time_t *>(malloc(sizeof(time_t)));
    *this->securityContext->lastRan = std::time(nullptr);
    RbxStuLog(RbxStu::LogType::Information, RbxStu::SecurityName,
              std::format("Our module hashed is: {}", moduleHash));

    std::thread memCheckLoopThead(MemCheckLoop, moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
    memCheckLoopThead.detach();

    this->m_bIsInitialized = true;
}
