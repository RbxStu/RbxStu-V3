//
// Created by Dottik on 14/10/2024.
//

#pragma once
#include <memory>

#include "TypeDefinitions.hpp"

namespace RbxStu::Roblox {
    class DataModel final {
        RBX::DataModel *m_pDataModel;

    public:
        explicit DataModel(RBX::DataModel *robloxDataModel);

        static std::unique_ptr<RbxStu::Roblox::DataModel> FromJob(void *robloxJob);

        [[nodiscard]] RBX::DataModelType GetDataModelType() const;
    };
} // RbxStu::Roblox
