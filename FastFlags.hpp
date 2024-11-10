//
// Created by Pixeluted on 10/11/2024.
//
#pragma once

#include <Logger.hpp>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace RbxStu {
    class FastFlags {
        enum FastFlagType {
            FastFlagString,
            FastFlagBoolean,
            FastFlagFloat,
            FastFlagInteger,

            FastFlagUnknown
        };

        using FlagValueType = std::variant<std::string, bool, float, int>;
        using FlagMapType = std::map<std::string, FlagValueType>;

        static std::shared_ptr<FastFlags> instance;

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
        static std::shared_ptr<FastFlags> GetSingleton();

        void Initialize();
        FastFlagType ParseFFLagNameToType(const std::string& fflagName);

        template<typename T>
        T GetOptionalFastFlagValue(const std::string& flagName, T defaultValue) {
            if (!this->isInitialized) {
                return defaultValue;
            }

            const auto it = this->loadedFlags.find(flagName);
            if (it == this->loadedFlags.end()) {
                return defaultValue;
            }

            try {
                return std::get<T>(it->second);
            } catch (const std::bad_variant_access&) {
                return defaultValue;
            }
        }
    };
} // RbxStu
