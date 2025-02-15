//
// Created by Pixeluted on 10/11/2024.
//

#include "FastFlags.hpp"

#include <Utilities.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace RbxStu {
    std::shared_ptr<FastFlagsManager> FastFlagsManager::instance;
    std::mutex FastFlagsManager::getSingletonMutex;

    std::shared_ptr<FastFlagsManager> FastFlagsManager::GetSingleton() {
        std::lock_guard guard(getSingletonMutex);

        if (instance == nullptr)
            instance = std::make_shared<FastFlagsManager>();

        if (!instance->isInitialized && !instance->FailedToLoadFlags)
            instance->Initialize();

        return instance;
    }

    RbxStu::FastFlagType FastFlagsManager::ParseFFLagNameToType(const std::string &fflagName) {
        if (fflagName.length() < 5)
            return FastFlagUnknown;
        const auto it = fastFlagPrefixes.find(fflagName.substr(0, 5));
        if (it != fastFlagPrefixes.end()) {
            return it->second;
        }

        return FastFlagUnknown;
    }

    void FastFlagsManager::ReloadFlags() {
        std::lock_guard guard(getSingletonMutex);
        RbxStuLog(Information, Fast_Flags, "Reloading FastFlags!");

        this->isInitialized = false;
        this->FailedToLoadFlags = false;
        this->loadedFlags.clear();
        this->Initialize();
        this->OnFastFlagsReloaded.GetFirableObject().Fire(
                {std::make_unique<OnFlagsReloaded>(FastFlagsManager::instance)});
    }

    void FastFlagsManager::WriteFlags() {
        const auto DllDirectoryOpt = Utilities::GetDllDir();
        if (!DllDirectoryOpt.has_value()) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Couldn't find the DLL Directory to save FastFlags!");
            return;
        }

        const auto &DllDirectory = DllDirectoryOpt.value();
        const auto fastFlagPath = DllDirectory / "RbxStuSettings.json";

        nlohmann::json json{};

        for (const auto &entry: this->loadedFlags) {
            switch (this->ParseFFLagNameToType(entry.first)) {
                case FastFlagString:
                    json[entry.first] = std::get<std::string>(entry.second);
                    break;
                case FastFlagBoolean:
                    json[entry.first] = std::get<bool>(entry.second);
                    break;
                case FastFlagFloat:
                    json[entry.first] = std::get<float>(entry.second);
                    break;
                case FastFlagInteger:
                    json[entry.first] = std::get<std::int32_t>(entry.second);
                    break;
                case FastFlagUnknown:
                    json[entry.first] = std::get<std::string>(entry.second);
                    break;
            }
        }

        const auto out = json.dump(1);
        std::ofstream file(fastFlagPath.string(), std::ios::out | std::ios::trunc);

        file.clear();
        file << out;
        file.flush();
        file.close();
    }

    void FastFlagsManager::Initialize() {
        if (this->isInitialized)
            return;

        const auto DllDirectoryOpt = Utilities::GetDllDir();
        if (!DllDirectoryOpt.has_value()) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Couldn't find the DLL Directory to initialize FastFlags!");
            this->FailedToLoadFlags = true;
            return;
        }

        const auto &DllDirectory = DllDirectoryOpt.value();
        const auto FastFlagFilePath = DllDirectory / "RbxStuSettings.json";

        if (!std::filesystem::exists(FastFlagFilePath)) {
            RbxStuLog(LogType::Information, RbxStu::Fast_Flags, "No fast flags file found, using default settings...");
            this->isInitialized = true;
            return;
        }

        std::ifstream file(FastFlagFilePath.string());
        if (!file.is_open()) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Failed to open fast flags file!");
            this->FailedToLoadFlags = true;
            return;
        }

        const auto FastFlagsFileContent = std::string(std::istreambuf_iterator(file), {});
        nlohmann::json parsedJSON;
        try {
            parsedJSON = nlohmann::json::parse(FastFlagsFileContent);
        } catch (const nlohmann::json::parse_error &e) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Failed to parse fast flags file!");
            this->FailedToLoadFlags = true;
            return;
        }

        if (!parsedJSON.is_object()) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Invalid fast flags file, expected an list of fast flags!");
            this->FailedToLoadFlags = true;
            return;
        }

        for (const auto &entry: parsedJSON.items()) {
            const auto &key = entry.key();
            const auto &value = entry.value();

            const auto flagType = ParseFFLagNameToType(key);
            if (flagType == FastFlagUnknown) {
                RbxStuLog(LogType::Error, RbxStu::Fast_Flags, std::format("Unknown fast flag type '{}'", key));
                continue;
            }

            std::optional<FlagValueType> flagValue;
            switch (flagType) {
                case FastFlagBoolean:
                    if (!value.is_boolean())
                        continue;
                    flagValue = value.get<bool>();
                    break;
                case FastFlagInteger:
                    if (!value.is_number_integer())
                        continue;
                    flagValue = value.get<int>();
                    break;
                case FastFlagFloat:
                    if (!value.is_number_float())
                        continue;
                    flagValue = value.get<float>();
                    break;
                case FastFlagString:
                    if (!value.is_string())
                        continue;
                    flagValue = value.get<std::string>();
                    break;
                default:
                    flagValue = std::nullopt;
                    break;
            }

            if (flagValue.has_value()) {
                loadedFlags.insert({key, flagValue.value()});
            }
        }

        RbxStuLog(LogType::Information, RbxStu::Fast_Flags,
                  std::format("Successfully loaded {} FastFlag(s)!", loadedFlags.size()));

        for (const auto &fastFlag: this->loadedFlags) {
            RbxStuLog(LogType::Information, RbxStu::Fast_Flags, std::format("- {}", fastFlag.first))
        }

        this->isInitialized = true;
    }
} // namespace RbxStu
