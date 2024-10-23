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
    class StringMatcher {
        static std::shared_ptr<RbxStu::StringMatcher> pInstance;
        std::map<std::string, std::map<std::string, hat::signature> > m_stringPacks;

    public:
        static std::shared_ptr<RbxStu::StringMatcher> GetSingleton();

        void LoadStringPack(const std::string &packName, const std::map<std::string, std::string_view> &stringPack);

        std::map<std::string, std::vector<void *>> RunMatcher(std::string_view moduleName,
                                                              const hat::process::module &hModule);
    };
} // RobloxDumper
