//
// Created by Dottik on 14/10/2024.
//

#pragma once
#include <memory>

namespace RBX {
    struct DataModel;

    enum DataModelType : std::int32_t;
}

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

        [[nodiscard]] RBX::DataModel *GetRbxPointer() const;

        void SetDataModelLock(bool newState) const;

        bool GetDataModelLock() const;

        static std::shared_ptr<RbxStu::Roblox::DataModel> FromPointer(void *dataModel);

        static std::shared_ptr<RbxStu::Roblox::DataModel> FromJob(void *robloxJob);

        bool IsParallel();

        bool IsDataModelOpen() const;

        bool CheckPointer() const;

        [[nodiscard]] RBX::DataModelType GetDataModelType() const;
    };
} // RbxStu::Roblox
