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

namespace RbxStu {
    enum FastFlagType final : std::int8_t {
        FastFlagString,
        FastFlagBoolean,
        FastFlagFloat,
        FastFlagInteger,

        FastFlagUnknown
    };

    class FastFlagsManager final {
        using FlagValueType = std::variant<std::string, bool, float, int>;
        using FlagMapType = std::map<std::string, FlagValueType>;

        static std::shared_ptr<FastFlagsManager> instance;
        static std::mutex getSingletonMutex;

        bool isInitialized = false;
        bool FailedToLoadFlags = false;

        FlagMapType loadedFlags = FlagMapType{};
        std::map<std::string, FastFlagType> fastFlagPrefixes = std::map<std::string, FastFlagType>{
            {"FFlag", FastFlagBoolean},
            {"DFlag", FastFlagFloat},
            {"IFlag", FastFlagInteger},
            {"SFlag", FastFlagString}
        };

    public:
        static std::shared_ptr<FastFlagsManager> GetSingleton();

        void Initialize();

        FastFlagType ParseFFLagNameToType(const std::string &fflagName);

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
    };

    template<FastFlagType type, typename T, T defaultValue>
    class FastFlagDeclaration final {
        std::string szFastFlagName = "";

    public:
        explicit FastFlagDeclaration(std::string szFastFlagName) : szFastFlagName(std::move(szFastFlagName)) {
            this->szFastFlagName = std::move(szFastFlagName);
        };

        [[nodiscard]] bool IsDefined() const {
            return RbxStu::FastFlagsManager::GetSingleton()->GetOptionalFastFlagValue<T>(
                       this->szFastFlagName, defaultValue)
                   !=
                   defaultValue;
        };

        [[nodiscard]] T GetValue() const {
            return RbxStu::FastFlagsManager::GetSingleton()->GetOptionalFastFlagValue<T>(
                this->szFastFlagName, defaultValue);
        }
    };

#define RBXSTU_DEFINEFASTFLAG(fastFlagVariableName, szFastFlagName, fastFlagType, cType, defaultFlagValue) static FastFlagDeclaration<fastFlagType, cType, defaultFlagValue> fastFlagVariableName{szFastFlagName};

    namespace FastFlags {
        RBXSTU_DEFINEFASTFLAG(FFlagIsRobloxInternalEnabled, "FFlagIsRobloxInternalEnabled",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(FFLagEnablePipeCommunication, "FFLagEnablePipeCommunication",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(FFlagEnableDebugLogs, "FFlagEnableDebugLogs",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);

        RBXSTU_DEFINEFASTFLAG(IFlagWebsocketPort, "IFlagWebsocketPort",
                              RbxStu::FastFlagType::FastFlagInteger, int, 7777);

        RBXSTU_DEFINEFASTFLAG(FFLagUseWebsocketServer, "FFLagUseWebsocketServer",
                              RbxStu::FastFlagType::FastFlagBoolean, bool, false);
    }

#undef RBXSTU_DEFINEFASTFLAG
} // RbxStu
