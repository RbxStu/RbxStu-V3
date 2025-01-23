//
// Created by Dottik on 14/10/2024.
//

#include "DataModel.hpp"
#include <Utilities.hpp>

#include "ScriptContext.hpp"
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

    void DataModel::SetDataModelLock(const bool newState) const {
        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto offsetWrapper = reinterpret_cast<r_RBX_DataModel_offsetPointerWrapper>(
                offsetContainer->GetOffset(RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper));

        // find write-lock = %s, present in RBX::ScriptContext::validateThreadAccess
        *reinterpret_cast<bool *>(reinterpret_cast<std::uintptr_t>(offsetWrapper(this->GetRbxPointer())) + 0x6C0) = newState;
    }

    bool DataModel::GetDataModelLock() const {
        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto offsetWrapper = reinterpret_cast<r_RBX_DataModel_offsetPointerWrapper>(
                offsetContainer->GetOffset(RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper));

        // find write-lock = %s, present in RBX::ScriptContext::validateThreadAccess
        return *reinterpret_cast<bool *>(reinterpret_cast<std::uintptr_t>(offsetWrapper(this->GetRbxPointer())) + 0x6C0);
    }

    std::shared_ptr<RbxStu::Roblox::DataModel> DataModel::FromPointer(void *dataModel) {
        return std::make_shared<RbxStu::Roblox::DataModel>(static_cast<RBX::DataModel *>(dataModel));
    }

    std::shared_ptr<RbxStu::Roblox::DataModel> DataModel::FromJob(void *robloxJob) {
        // {
        //     auto waiting = RbxStu::Roblox::ScriptContext::FromWaitingHybridScriptsJob(robloxJob);
        //     if (waiting != nullptr) {
        //         return DataModel::FromPointer(waiting->GetDataModel());
        //     }
        // }
        /*
         * Jobs always have a pointer to a fake Datamodel and that fake data model has pointer to a real Datamodel
         * Offset Explanation:
         * Job + 0xB0 - Fake Datamodel (Deleter)
         * Fake Datamodel + 0x198 - Real Datamodel
         */

        try {
            const auto realDataModel = *reinterpret_cast<RBX::DataModel **>(
                *reinterpret_cast<uintptr_t *>(
                    reinterpret_cast<uintptr_t>(robloxJob) + 0xB0) + 0x198);
            return std::make_shared<RbxStu::Roblox::DataModel>(realDataModel);
        } catch (const std::exception &ex) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      std::format("Failed to obtain DataModel {}", ex.what()));
        }

        return std::make_shared<RbxStu::Roblox::DataModel>(nullptr);
    }

    bool DataModel::IsParallel() {
        if (!this->CheckPointer()) return false;

        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto offsetWrapper = reinterpret_cast<r_RBX_DataModel_offsetPointerWrapper>(
                offsetContainer->GetOffset(RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper));

        // find RBX::ScriptContext::validateThreadAccess, bottom dereference with a comparison to 0 in two variables is the offset you're after.
        return *reinterpret_cast<bool *>(reinterpret_cast<std::uintptr_t>(offsetWrapper(this->GetRbxPointer())) + 0x420);
    }

    bool DataModel::IsDataModelOpen() const {
        if (!this->CheckPointer()) return false;

        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto offsetWrapper = reinterpret_cast<r_RBX_DataModel_offsetPointerWrapper>(
                offsetContainer->GetOffset(RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper));

        // Check for strings mentioning a closed DataModel to update offset
        return this->GetDataModelType() != RBX::DataModelType_Null && *reinterpret_cast<bool *>(
                   reinterpret_cast<uintptr_t>(offsetWrapper(this->GetRbxPointer())) + 0x571);
    }

    bool DataModel::CheckPointer() const {
        auto ptr = this->GetRbxPointer() != nullptr && Utilities::IsPointerValid(this->GetRbxPointer());
        return ptr;
    }

    RBX::DataModelType DataModel::GetDataModelType() const {
        if (!this->CheckPointer()) return RBX::DataModelType_Null;

        /*
         *  How to update offset:
         *      - Find RBX::DataModel::getStudioGameStateType via string
         *      - Find a call that utilises RBX::DataModel::getDataModel
         *      - Utilise the subtraction present on - 0x1A0 and sum it with the offset that getStudioGameStateType uses.
         */

        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto offsetWrapper = reinterpret_cast<r_RBX_DataModel_offsetPointerWrapper>(
                offsetContainer->GetOffset(RbxStuOffsets::OffsetKey::RBX_DataModel_offsetPointerWrapper));

        return static_cast<RBX::DataModelType>(
                *reinterpret_cast<std::int32_t *>(reinterpret_cast<uintptr_t>(offsetWrapper(this->GetRbxPointer())) + 0x480));
    }
} // RbxStu::Roblox
