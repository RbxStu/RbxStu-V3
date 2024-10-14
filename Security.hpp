//
// Created by Pixeluted on 14/10/2024.
//
#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <sha.h>
#include <hex.h>
#include <Psapi.h>
#include "Settings.hpp"
#include <Logger.hpp>

namespace RbxStu
{
    class Security
    {
        static std::shared_ptr<Security> pInstance;
        std::atomic_bool m_bIsInitialized;
        std::string originalHashedMemory;

        void Initialize();
    public:

        bool IsInitialized();
        std::string GetHashedMemory();
        static std::string HashModuleSections(LPVOID lpBaseOfDll, DWORD SizeOfImage);
        static std::shared_ptr<Security> GetSingleton();
        static std::string HashBytes(const byte* data, size_t length);
    };
}
