//
// Created by Dottik on 12/10/2024.
//

#include "Utilities.hpp"

#include <future>
#include <mutex>
#include <regex>

std::shared_ptr<RbxStu::Utilities> RbxStu::Utilities::pInstance;

void RbxStu::Utilities::Initialize() {
    this->m_luaErrorStringRegex =
            std::regex(R"(.*:(\d)*: )", std::regex::optimize | std::regex::icase);
}

bool RbxStu::Utilities::IsInitialized() {
    return this->m_bIsInitialized;
}

std::mutex RbxStuUtilsInitialize;

std::shared_ptr<RbxStu::Utilities> RbxStu::Utilities::GetSingleton() {
    if (RbxStu::Utilities::pInstance == nullptr)
        RbxStu::Utilities::pInstance = std::make_shared<RbxStu::Utilities>();

    if (!RbxStu::Utilities::pInstance->IsInitialized()) {
        std::scoped_lock lock(RbxStuUtilsInitialize);
        // Locking is fairly expensive...
        if (RbxStu::Utilities::pInstance->IsInitialized())
            return RbxStu::Utilities::pInstance;

        RbxStu::Utilities::pInstance->Initialize();
    }
    return RbxStu::Utilities::pInstance;
}

std::string RbxStu::Utilities::FromLuaErrorMessageToCErrorMessage(const std::string &luauMessage) const {
    if (std::regex_search(luauMessage.begin(), luauMessage.end(), this->m_luaErrorStringRegex)) {
        const auto fixed = std::regex_replace(luauMessage, this->m_luaErrorStringRegex, "");

        return fixed;
    }

    return luauMessage;
}

std::string RbxStu::Utilities::WcharToString(const wchar_t *wideStr) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded == 0) {
        return "CONVERSION FAILED";
    }

    std::string result(sizeNeeded, 0);

    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &result[0], sizeNeeded, nullptr, nullptr);
    result.pop_back();

    return result;
}


std::string RbxStu::Utilities::ToLower(std::string target) {
    for (auto &x: target)
        x = std::tolower(x); // NOLINT(*-narrowing-conversions)

    return target;
}

std::string RbxStu::Utilities::ToUpper(std::string target) {
    for (auto &x: target)
        x = std::toupper(x); // NOLINT(*-narrowing-conversions)

    return target;
}
