//
// Created by Dottik on 20/10/2024.
//

#pragma once
#include <map>
#include <memory>
#include <string>
#include <libhat/Process.hpp>

#include <libhat/Signature.hpp>

namespace RbxStu {
    class SignatureMatcher {
        static std::shared_ptr<RbxStu::SignatureMatcher> pInstance;
        std::map<std::string, std::map<std::string, hat::signature> > m_signaturePacks;

    public:
        static std::shared_ptr<RbxStu::SignatureMatcher> GetSingleton();

        void LoadSignaturePack(const std::string &packName, const std::map<std::string, hat::signature> &signaturePack);

        std::map<std::string, void *> RunMatcher(std::string_view moduleName, const hat::process::module &hModule);
    };
} // RobloxDumper
