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

        bool operator==(const RBX::DataModel *dataModel) const;

        bool operator==(RBX::DataModel *dataModel) const;

        bool operator==(const RbxStu::Roblox::DataModel &dataModel) const;

        bool operator==(RbxStu::Roblox::DataModel &dataModel) const;

        bool operator==(const std::shared_ptr<RbxStu::Roblox::DataModel> &dataModel) const;

        bool operator==(std::shared_ptr<RbxStu::Roblox::DataModel> &dataModel) const;

        RBX::DataModel *GetRbxPointer() const;

        static std::shared_ptr<RbxStu::Roblox::DataModel> FromJob(void *robloxJob);

        [[nodiscard]] RBX::DataModelType GetDataModelType() const;
    };
} // RbxStu::Roblox
