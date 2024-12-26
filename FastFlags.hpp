//
// Created by Pixeluted on 10/11/2024.
//
#pragma once

#include <Logger.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "Miscellaneous/Initializable.hpp"
#include "Miscellaneous/ListenableEvent.hpp"

namespace RbxStu {
    class FastFlagsManager;
    enum FastFlagType final : std::int8_t {
        FastFlagString,
        FastFlagBoolean,
        FastFlagFloat,
        FastFlagInteger,

        FastFlagUnknown
    };

    class OnFlagsReloaded final {
    public:
        std::shared_ptr<FastFlagsManager> pManager;
    };

    class FastFlagsManager final {
        using FlagValueType = std::variant<std::string, bool, float, int>;
        using FlagMapType = std::map<std::string, FlagValueType>;

        static std::shared_ptr<FastFlagsManager> instance;
        static std::mutex getSingletonMutex;

        bool isInitialized = false;
        bool FailedToLoadFlags = false;

        FlagMapType loadedFlags = FlagMapType{};
        std::map<std::string, FastFlagType> fastFlagPrefixes =
                std::map<std::string, FastFlagType>{{"FFlag", FastFlagBoolean},
                                                    {"DFlag", FastFlagFloat},
                                                    {"IFlag", FastFlagInteger},
                                                    {"SFlag", FastFlagString}};

    public:
        RbxStu::Miscellaneous::ListenableEvent<OnFlagsReloaded, RbxStu::Miscellaneous::EventArgument<OnFlagsReloaded>>
                OnFastFlagsReloaded;

        static std::shared_ptr<FastFlagsManager> GetSingleton();

        void Initialize();

        FastFlagType ParseFFLagNameToType(const std::string &fflagName);

        void ReloadFlags();

        template<typename T>
        [[nodiscard]] T GetOptionalFastFlagValue(const std::string &flagName, T defaultValue) const {
            if (!this->isInitialized)
                return defaultValue;

            const auto it = this->loadedFlags.find(flagName);
            if (it == this->loadedFlags.end())
                return defaultValue;

            try {
                return std::get<T>(it->second);
            } catch (const std::bad_variant_access &) {
                return defaultValue;
            }
        }

        template<typename T>
        void SetFastFlagValue(const std::string &flagName, const T &newValue) {
            if (!this->isInitialized)
                return;


            if (auto it = this->loadedFlags.find(flagName); it != this->loadedFlags.end())
                this->loadedFlags.erase(it);

            this->loadedFlags.insert({flagName, newValue});
        }

        void WriteFlags();
    };

    template<FastFlagType type, typename T>
    class FastFlagDeclaration final {
        std::string szFastFlagName;
        T m_defaultValue;

    public:
        explicit FastFlagDeclaration(const std::string &szFastFlagName, T defaultValue) {
            this->szFastFlagName = (szFastFlagName);
            this->m_defaultValue = defaultValue;
        };

        [[nodiscard]] bool IsDefined() const {
            return RbxStu::FastFlagsManager::GetSingleton()->GetOptionalFastFlagValue<T>(
                           this->szFastFlagName, this->m_defaultValue) != this->m_defaultValue;
        };


        void SetValue(const T &tNewValue) {
            return RbxStu::FastFlagsManager::GetSingleton()->SetFastFlagValue<T>(this->szFastFlagName, tNewValue);
        }

        [[nodiscard]] T GetValue(const std::shared_ptr<FastFlagsManager> &pManager) const {
            return pManager->GetOptionalFastFlagValue<T>(this->szFastFlagName, this->m_defaultValue);
        }

        [[nodiscard]] T GetValue() const {
            return RbxStu::FastFlagsManager::GetSingleton()->GetOptionalFastFlagValue<T>(this->szFastFlagName,
                                                                                         this->m_defaultValue);
        }
    };

#define RBXSTU_DEFINEFASTFLAG(fastFlagVariableName, szFastFlagName, fastFlagType, cType, defaultFlagValue)             \
    static FastFlagDeclaration<fastFlagType, cType> fastFlagVariableName{szFastFlagName, defaultFlagValue};

    namespace FastFlags {
        RBXSTU_DEFINEFASTFLAG(FFlagIsRobloxInternalEnabled, "FFlagIsRobloxInternalEnabled",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(FFlagEnablePipeCommunication, "FFlagEnablePipeCommunication",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(FFlagEnableDebugLogs, "FFlagEnableDebugLogs", RbxStu::FastFlagType::FastFlagBoolean, bool,
                              false);

        RBXSTU_DEFINEFASTFLAG(IFlagWebsocketPort, "IFlagWebsocketPort", RbxStu::FastFlagType::FastFlagInteger, int,
                              7777);

        RBXSTU_DEFINEFASTFLAG(FFlagUseWebsocketServer, "FFlagUseWebsocketServer", RbxStu::FastFlagType::FastFlagBoolean,
                              bool, false);

        RBXSTU_DEFINEFASTFLAG(FFlagEnableExperimentalLuauFunctions, "FFlagEnableExperimentalLuauFunctions",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(FFlagDisableErrorHandler, "FFlagDisableErrorHandler",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(SFlagRbxCrashKey, "SFlagRbxCrashKey", RbxStu::FastFlagType::FastFlagString, std::string,
                              "__RbxCrash");

        RBXSTU_DEFINEFASTFLAG(FFlagEnableImGui, "FFlagEnableImGui", RbxStu::FastFlagType::FastFlagBoolean, bool, true);
    } // namespace FastFlags

#undef RBXSTU_DEFINEFASTFLAG
} // namespace RbxStu
