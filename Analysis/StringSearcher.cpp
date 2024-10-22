//
// Created by Dottik on 19/10/2024.
//

#include <sstream>

#include "StringSearcher.hpp"

#include <Utilities.hpp>

#include "XrefSearcher.hpp"

std::shared_ptr<RbxStu::Analysis::StringSearcher> RbxStu::Analysis::StringSearcher::pInstance;

std::shared_ptr<RbxStu::Analysis::StringSearcher> RbxStu::Analysis::StringSearcher::GetSingleton() {
    if (nullptr == RbxStu::Analysis::StringSearcher::pInstance)
        RbxStu::Analysis::StringSearcher::pInstance = std::make_shared<RbxStu::Analysis::StringSearcher>();

    return RbxStu::Analysis::StringSearcher::pInstance;
}

hat::signature RbxStu::Analysis::StringSearcher::ToAOB(const std::string_view szTargetString) {
    std::stringstream stream{};

    auto str = szTargetString.data();
    while (*str != '\0') {
        stream << std::hex << static_cast<int>(*str);

        if (*(str + 1) != '\0')
            stream << " ";

        ++str;
    }

    const auto AOB = RbxStu::Utilities::ToUpper(stream.str());

    auto signature = hat::parse_signature(AOB).value();

    return signature;
}

std::optional<void *> RbxStu::Analysis::StringSearcher::GetStringAddressInTarget(
    const hat::process::module hModule,
    const std::string_view szTargetString) {
    const auto pattern = hat::find_pattern(this->ToAOB(szTargetString), ".rdata", hModule);
    return pattern.has_result() ? pattern.get() : nullptr;
}

std::vector<void *> RbxStu::Analysis::StringSearcher::FindStringXrefsInTarget(
    const hat::process::module hModule, const std::string_view szTargetString) {
    const auto remoteString = this->GetStringAddressInTarget(hModule, szTargetString);
    if (!remoteString.has_value()) return {};
    const auto xrefSearcher = XrefSearcher::GetSingleton();

    return xrefSearcher->GetXrefsForPointer(remoteString.value());
}
