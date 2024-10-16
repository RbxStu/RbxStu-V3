//
// Created by Dottik on 15/10/2024.
//

#include "ModManager.hpp"

#include <Logger.hpp>
#include <mutex>
#include <Utilities.hpp>

#include "Settings.hpp"

namespace RbxStu {
    namespace Modding {
        std::shared_ptr<RbxStu::Modding::ModManager> ModManager::pInstance;

        std::mutex RbxStuModdingModManagerGetSingleton;

        bool ModManager::IsInitialized() {
            return this->m_bIsInitialized;
        }

        void ModManager::Initialize() {
            if (this->IsInitialized()) return;

            RbxStuLog(RbxStu::LogType::Information, RbxStu::Modding_ModManager, "Searching for RbxStu V3 Mods...");

            const std::filesystem::path dllDirectory(RbxStu::Utilities::GetDllDir());
            const std::filesystem::path modDirectory = dllDirectory / "Mods";

            if (!std::filesystem::exists(modDirectory)) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Modding_ModManager,
                          "No mods folder located! Creating default one...");
                std::filesystem::create_directory(modDirectory);
                return;
            }

            RbxStuLog(RbxStu::LogType::Information, RbxStu::Modding_ModManager, "Scanning Mods Directory...");

            for (const auto &entry: std::filesystem::recursive_directory_iterator(modDirectory)) {
                auto extension = entry.path().extension().string();
                if (entry.is_regular_file() && RbxStu::Utilities::ToLower(extension) == ".dll") {
                    RbxStuLog(RbxStu::LogType::Information, RbxStu::Modding_ModManager,
                              std::format("- Found Mod -- {}", entry.path().filename().string()));
                    RbxStuLog(RbxStu::LogType::Information, RbxStu::Modding_ModManager,
                              "Bootstrapping Mod...");

                    const HMODULE hLibrary = LoadLibrary(entry.path().string().c_str());

                    if (hLibrary == nullptr || hLibrary == INVALID_HANDLE_VALUE) {
                        RbxStuLog(RbxStu::LogType::Error, RbxStu::Modding_ModManager,
                                  "Error loading RbxStu V3 Mod!");
                        continue;
                    }

                    if (hLibrary == GetModuleHandle(RBXSTU_DLL_NAME)) {
                        RbxStuLog(RbxStu::LogType::Error, RbxStu::Modding_ModManager,
                                  "Invalid RbxStu V3 mod! This is RbxStu V3 ITSELF, what are you even doing? This would crash you if you tried to do this!"
                        );
                        continue;
                    }


                    if (FARPROC const initialize = GetProcAddress(hLibrary, "RbxStuV3Initialize");
                        initialize == nullptr) {
                        RbxStuLog(RbxStu::LogType::Warning, RbxStu::Modding_ModManager,
                                  "Invalid RbxStu V3 mod! There is no function exposed called 'RbxStuV3Initialize'! Export this function on your Module!"
                        );
                        RbxStuLog(RbxStu::LogType::Warning, RbxStu::Modding_ModManager,
                                  "WARNING: Freeing Native Module In 3 Seconds! If you crash, the mod has executed code on DllMain, THIS IS NOT HOW YOU SHOULD WRITE A MOD."
                        );
                        Sleep(3000);
                        FreeLibrary(hLibrary); // clean up
                        continue;
                    } else {
                        RbxStuLog(RbxStu::LogType::Information, RbxStu::Modding_ModManager,
                                  std::format("Invoking __stdcall void {}::Initialize(void)", entry.path().string()));

                        initialize();
                    }
                }
            }

            this->m_bIsInitialized = true;
        }

        std::shared_ptr<RbxStu::Modding::ModManager> ModManager::GetSingleton() {
            if (ModManager::pInstance == nullptr)
                ModManager::pInstance = std::make_shared<RbxStu::Modding::ModManager>();

            if (!ModManager::pInstance->IsInitialized()) {
                std::scoped_lock lock{RbxStuModdingModManagerGetSingleton};
                if (ModManager::pInstance->IsInitialized()) return ModManager::pInstance;

                ModManager::pInstance->Initialize();
            }

            return ModManager::pInstance;
        }
    } // Modding
} // RbxStu
