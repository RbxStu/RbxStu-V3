//
// Created by Pixeluted on 10/11/2024.
//

#include "FastFlags.hpp"

#include <fstream>
#include <Utilities.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace RbxStu {
   std::shared_ptr<FastFlags> FastFlags::instance;

   std::shared_ptr<FastFlags> FastFlags::GetSingleton() {
      if (instance == nullptr)
         instance = std::make_shared<FastFlags>();

      if (!instance->isInitialized && !instance->FailedToLoadFlags)
         instance->Initialize();

      return instance;
   }

   FastFlags::FastFlagType FastFlags::ParseFFLagNameToType(const std::string& fflagName) {
      const auto it = fastFlagPrefixes.find(fflagName.substr(0, 5));
      if (it != fastFlagPrefixes.end()) {
         return it->second;
      }

      return FastFlagUnknown;
   }


   void FastFlags::Initialize() {
      if (isInitialized) return;

      const auto DllDirectoryOpt = Utilities::GetDllDir();
      if (!DllDirectoryOpt.has_value()) {
         RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Couldn't find the DLL Directory to initialize FastFlags!");
         this->FailedToLoadFlags = true;
         return;
      }

      const auto& DllDirectory = DllDirectoryOpt.value();
      const auto FastFlagFilePath = DllDirectory / "RbxStuSettings.json";

      if (!std::filesystem::exists(FastFlagFilePath)) {
         RbxStuLog(LogType::Information, RbxStu::Fast_Flags, "No fast flags file found, using default settings...");
         this->FailedToLoadFlags = true;
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
      } catch (const nlohmann::json::parse_error& e) {
         RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Failed to parse fast flags file!");
         this->FailedToLoadFlags = true;
         return;
      }

      if (!parsedJSON.is_object()) {
         RbxStuLog(LogType::Error, RbxStu::Fast_Flags, "Invalid fast flags file, expected an list of fast flags!");
         this->FailedToLoadFlags = true;
         return;
      }

      for (const auto& entry : parsedJSON.items()) {
         const auto& key = entry.key();
         const auto& value = entry.value();

         const auto flagType = ParseFFLagNameToType(key);
         if (flagType == FastFlagUnknown) {
            RbxStuLog(LogType::Error, RbxStu::Fast_Flags, std::format("Unknown fast flag type '{}'", key));
            continue;
         }

         std::optional<FlagValueType> flagValue;
         switch (flagType) {
            case FastFlagBoolean:
               if (!value.is_boolean()) continue;
               flagValue = value.get<bool>();
               break;
            case FastFlagInteger:
               if (!value.is_number_integer()) continue;
               flagValue = value.get<int>();
               break;
            case FastFlagFloat:
               if (!value.is_number_float()) continue;
               flagValue = value.get<float>();
               break;
            case FastFlagString:
               if (!value.is_string()) continue;
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

      RbxStuLog(LogType::Information, RbxStu::Fast_Flags, std::format("Successfully loaded {} FastFlag(s)!", loadedFlags.size()));
      this->isInitialized = true;
   }
} // RbxStu