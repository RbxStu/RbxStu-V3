//
// Created by Dottik on 10/8/2024.
//

#include "Logger.hpp"

#include <Termcolor.hpp>
#include <format>
#include <iostream>
#include <mutex>
#include <shared_mutex>

#include "Roblox/TypeDefinitions.hpp"
#include "Settings.hpp"

std::shared_mutex mutex;
std::shared_ptr<RbxStu::Logger> RbxStu::Logger::pInstance;

std::shared_ptr<RbxStu::Logger> RbxStu::Logger::GetSingleton() {
    if (RbxStu::Logger::pInstance == nullptr)
        RbxStu::Logger::pInstance = std::make_shared<Logger>();

    return RbxStu::Logger::pInstance;
}

void RbxStu::Logger::OpenStandard() {
    freopen_s(reinterpret_cast<FILE **>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE **>(stdin), "CONIN$", "r", stdin);
    freopen_s(reinterpret_cast<FILE **>(stderr), "CONOUT$", "w", stderr);
}

void RbxStu::Logger::Flush(const RbxStu::LogType messageType) {
    // TODO: Implement flushing to file.
    switch (messageType) {
        case RbxStu::LogType::Error:
            std::cout << termcolor::bright_red << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RbxStu::LogType::Warning:
            std::cout << termcolor::bright_yellow << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RbxStu::LogType::Information:
            std::cout << termcolor::bright_blue << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RbxStu::LogType::Debug:
            std::cout << termcolor::bright_white << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
    }
    this->m_szMessageBuffer.clear();
}

void RbxStu::Logger::FlushIfFull(const RbxStu::LogType messageType) {
    if (!this->m_bInitialized)
        throw std::exception(
            std::format("The logger instance @ {} is not initialized!", reinterpret_cast<uintptr_t>(this)).c_str());

    if (this->m_bInstantFlush || this->m_szMessageBuffer.length() >= this->m_dwBufferSize)
        this->Flush(messageType);
}

void RbxStu::Logger::Initialize(const bool bInstantFlush) {
    if (this->m_bInitialized)
        return;

    this->OpenStandard();
    this->m_dwBufferSize = 0xffff;
    this->m_szMessageBuffer = std::string("");
    this->m_szMessageBuffer.reserve(this->m_dwBufferSize);
    this->m_bInstantFlush = bInstantFlush;
    this->m_bInitialized = true;
    std::atexit([] {
        const auto logger = RbxStu::Logger::GetSingleton();
        logger->Flush(RbxStu::LogType::Information);
        logger->m_szMessageBuffer.clear();
    });
}

void RbxStu::Logger::PrintDebug(std::string_view sectionName, std::string_view msg, std::string_view line) {
#if RBXSTU_ENABLE_DEBUG_LOGS
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[DEBUG/{}:{}] {}", sectionName, line, msg));
    this->FlushIfFull(RbxStu::LogType::Debug);
#endif // #if RBXSTU_ENABLE_DEBUG_LOGS
}

void RbxStu::Logger::PrintInformation(std::string_view sectionName, std::string_view msg, std::string_view line) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[INFO/{}:{}] {}", sectionName, line, msg));
    this->FlushIfFull(RbxStu::LogType::Information);
}

void RbxStu::Logger::PrintWarning(std::string_view sectionName, std::string_view msg, std::string_view line) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[WARN/{}:{}] {}", sectionName, line, msg));
    this->FlushIfFull(RbxStu::LogType::Warning);
}

void RbxStu::Logger::PrintError(std::string_view sectionName, std::string_view msg, std::string_view line) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[ERROR/{}:{}] {}", sectionName, line, msg));
    this->FlushIfFull(RbxStu::LogType::Error);
}
