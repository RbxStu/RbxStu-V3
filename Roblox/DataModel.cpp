//
// Created by Dottik on 14/10/2024.
//

#include "DataModel.hpp"

#include "TypeDefinitions.hpp"

namespace RbxStu::Roblox {
    DataModel::DataModel(RBX::DataModel *robloxDataModel) {
        this->m_pDataModel = robloxDataModel;
    }

    bool DataModel::operator==(const RBX::DataModel *dataModel) const {
        return this->m_pDataModel == dataModel;
    }

    bool DataModel::operator==(RBX::DataModel *dataModel) const {
        return this->m_pDataModel == dataModel;
    }

    bool DataModel::operator==(const RbxStu::Roblox::DataModel &dataModel) const {
        return this->m_pDataModel == dataModel.m_pDataModel;
    }

    bool DataModel::operator==(RbxStu::Roblox::DataModel &dataModel) const {
        return this->m_pDataModel == dataModel.m_pDataModel;
    }

    bool DataModel::operator==(const std::shared_ptr<RbxStu::Roblox::DataModel> &dataModel) const {
        return this->m_pDataModel == dataModel->m_pDataModel;
    }

    bool DataModel::operator==(std::shared_ptr<RbxStu::Roblox::DataModel> &dataModel) const {
        return this->m_pDataModel == dataModel->m_pDataModel;
    }

    RBX::DataModel *DataModel::GetRbxPointer() const {
        return this->m_pDataModel;
    }

    std::shared_ptr<RbxStu::Roblox::DataModel> DataModel::FromJob(void *robloxJob) {
        /*
         * Jobs always have a pointer to a fake Datamodel and that fake data model has pointer to a real Datamodel
         * Offset Explanation:
         * Job + 0xB8 - Fake Datamodel (Deleter)
         * Fake Datamodel + 0x18 - Real Datamodel
         */

        const auto realDataModel = *reinterpret_cast<RBX::DataModel **>(
            *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(robloxJob) + 0xB8) + 0x18);
        return std::make_shared<RbxStu::Roblox::DataModel>(realDataModel);
    }

    RBX::DataModelType DataModel::GetDataModelType() const {
        return this->m_pDataModel->m_dwDataModelType;
    }
} // RbxStu::Roblox
