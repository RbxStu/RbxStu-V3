//
// Created by Dottik on 10/8/2024.
//
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu {
    enum LogType {
        Information, Warning, Error, Debug
    };

    class Logger final {
        /// @brief Private, Static shared pointer into the instance.
        static std::shared_ptr<Logger> pInstance;

        /// @brief Disables buffering.
        bool m_bInstantFlush;
        /// @brief Defines whether the Logger instance is initialized or not.
        bool m_bInitialized;
        /// @brief The size of the buffer.
        std::uint32_t m_dwBufferSize;
        /// @brief The buffer used to store messages.
        std::string m_szMessageBuffer;

        /// @brief Flushes the buffer into the standard output.
        void Flush(RbxStu::LogType messageType);

        /// @brief Flushes the buffer only if the buffer is full.
        void FlushIfFull(RbxStu::LogType messageType);

    public:
        /// @brief Obtains the Singleton for the Logger instance.
        /// @return Returns a shared pointer to the global Logger singleton instance.
        static std::shared_ptr<Logger> GetSingleton();

        void OpenStandard();

        /// @brief Initializes the Logger instance by opening the standard pipes, setting up the buffer and its size.
        /// @param bInstantFlush Whether the logger should keep no buffer, and let the underlying implementation for stdio
        /// and files handle it.
        void Initialize(bool bInstantFlush);

        void PrintDebug(std::string_view sectionName, std::string_view msg, std::string_view line);

        /// @brief Emits an Information with the given section name into the Logger's buffer.
        /// @param sectionName The name of the section that the code is running at
        /// @param msg The content to write into the buffer, as an information.
        /// @param line
        void PrintInformation(std::string_view sectionName, std::string_view msg, std::string_view line);

        /// @brief Emits a Warning with the given section name into the Logger's buffer.
        /// @param sectionName The name of the section that the code is running at
        /// @param msg The content to write into the buffer, as a warning.
        /// @param line
        void PrintWarning(std::string_view sectionName, std::string_view msg, std::string_view line);

        /// @brief Emits an error with the given section name into the Logger's buffer.
        /// @param sectionName The name of the section that the code is running at
        /// @param msg The content to write into the buffer, as an error.
        /// @param line
        void PrintError(std::string_view sectionName, std::string_view msg, std::string_view line);
    };


    /// @brief Defines a section for use in the logger
#define DefineSectionName(varName, sectionName) constexpr auto varName = sectionName
    DefineSectionName(MainThread, "RbxStuV3::MainThread");
    DefineSectionName(SecurityName, "RbxStuV3::Security");
    DefineSectionName(Anonymous, "RbxStuV3::Anonymous");
    DefineSectionName(ExecutionEngine, "RbxStuV3::ExecutionEngine");
    DefineSectionName(EnvironmentContext, "RbxStuV3::EnvironmentContext");

    DefineSectionName(Roblox_ScriptContext, "RbxStuV3::Roblox::ScriptContext");

    DefineSectionName(Analysis_SignatureMatcher, "RbxStuV3::Analysis::SignatureMatcher");
    DefineSectionName(Analysis_StringMatcher, "RbxStuV3::Analysis::StringMatcher");
    DefineSectionName(Analysis_RTTI, "RbxStuV3::Analysis::RTTI");
    DefineSectionName(Analysis_XrefSearcher, "RbxStuV3::Analysis::XrefSearcher");
    DefineSectionName(Analysis_Disassembler, "RbxStuV3::Analysis::Disassembler");

    DefineSectionName(Scheduling_Jobs_DataModelWatcherJob,
                      "RbxStuV3::Scheduling::Jobs::DataModelWatcherJob");

    DefineSectionName(Scheduling_Jobs_InitializeExecutionEngineJob,
                      "RbxStuV3::Scheduling::Jobs::InitializeExecutionEngineJob");
    DefineSectionName(Scheduling_Jobs_ExecuteScriptJob, "RbxStuV3::Scheduling::Jobs::ExecuteScriptJob");
    DefineSectionName(Scheduling_Jobs_ResumeYieldedThreadsJob, "RbxStuV3::Scheduling::Jobs::ResumeYieldedThreadsJob");
    DefineSectionName(Scheduling_TaskSchedulerOrchestrator, "RbxStuV3::Scheduling::TaskSchedulerOrchestrator");
    DefineSectionName(Scheduling_TaskScheduler, "RbxStuV3::Scheduling::TaskScheduler");

    DefineSectionName(Scanners_Luau, "RbxStuV3::Scanners::Luau");
    DefineSectionName(Scanners_RBXReflection, "RbxStuV3::Scanners::RBXReflection");
    DefineSectionName(Scanners_RBX, "RbxStuV3::Scanners::RBX");

    DefineSectionName(Modding_ModManager, "RbxStuV3::Modding::ModManager");

    DefineSectionName(RBXCRASH, "RbxStuV3::HK_RBXCRASH");
    DefineSectionName(StructuredExceptionHandler, "RbxStuV3::StructuredExceptionHandler");
    DefineSectionName(StructuredExceptionHandlerAnalysis, "RbxStuV3::StructuredExceptionHandlerAnalysis");

    DefineSectionName(WebsocketServer, "RbxStuV3::WebsocketServer");
#undef DefineSectionName
}; // namespace RbxStu

#define RbxStuLog(logType, sectionName, logMessage) {                           \
    const auto logger = RbxStu::Logger::GetSingleton();                         \
    switch (logType) {                                                          \
        case RbxStu::LogType::Information:                                      \
            logger->PrintInformation(sectionName, logMessage, __FUNCTION__);    \
            break;                                                              \
        case RbxStu::LogType::Warning:                                          \
            logger->PrintWarning(sectionName, logMessage, __FUNCTION__);        \
            break;                                                              \
        case RbxStu::LogType::Error:                                            \
            logger->PrintError(sectionName, logMessage, __FUNCTION__);          \
            break;                                                              \
        case RbxStu::LogType::Debug:                                            \
            logger->PrintDebug(sectionName, logMessage, __FUNCTION__);          \
            break;                                                              \
        }                                                                       \
    }
