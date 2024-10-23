//
// Created by Dottik on 20/10/2024.
//

#include <format>

#include "SignatureMatcher.hpp"
#include "Logger.hpp"
#include "Utilities.hpp"


namespace RbxStu {
    std::shared_ptr<RbxStu::SignatureMatcher> RbxStu::SignatureMatcher::pInstance;

    std::shared_ptr<RbxStu::SignatureMatcher> SignatureMatcher::GetSingleton() {
        if (nullptr == RbxStu::SignatureMatcher::pInstance)
            RbxStu::SignatureMatcher::pInstance = std::make_shared<RbxStu::SignatureMatcher>();

        return RbxStu::SignatureMatcher::pInstance;
    }

    void SignatureMatcher::LoadSignaturePack(const std::string &packName,
                                             const std::map<std::string, hat::signature> &signaturePack) {
        if (this->m_signaturePacks.contains(packName)) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Analysis_SignatureMatcher,
                      std::format(
                          "The provided signature pack {} has already been loaded into memory, ignoring call.",
                          packName, signaturePack.size()
                      ));
            return;
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_SignatureMatcher,
                  std::format("Loading signature pack {} with {} functions...", packName, signaturePack.size()));

        this->m_signaturePacks[packName] = signaturePack;
    }

    std::map<std::string, void *> SignatureMatcher::RunMatcher(const std::string_view moduleName,
                                                               const hat::process::module &hModule) {
        std::map<std::string, void *> results{};

        for (const auto &[packName, signaturePack]: this->m_signaturePacks) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_SignatureMatcher,
                      std::format("Matching signature pack {}...", packName));

            auto scanResults = Utilities::ScanMany(signaturePack, true, ".text");

            for (auto it = scanResults.begin(); it != scanResults.end(); ++it) {
                if (!it->second.has_result()) {
                    RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_SignatureMatcher,
                              std::format("- Could not find signature for {} in {}", it->first, moduleName));
                    continue;
                }
                RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_SignatureMatcher,
                          std::format("- Found signature {} in {}+{}", it->first, moduleName, reinterpret_cast<
                              void *>(reinterpret_cast<std::uintptr_t>(it->second.get()) - hModule.
                                  address())));
                results[it->first] = it->second.get();
            }
        }

        return results;
    }
} // RobloxDumper
