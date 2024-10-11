//
// Created by Dottik on 3/4/2024.
//

#include "StudioOffsets.h"
#include "memory"
#include "shared_mutex"

std::shared_ptr<RbxStuOffsets> RbxStuOffsets::ptr;
std::shared_mutex RbxStuOffsets::__rbxstuoffsets__sharedmutex__;

__declspec(dllexport) std::shared_ptr<RbxStuOffsets> RbxStuOffsets::GetSingleton()
{
    std::lock_guard lock{__rbxstuoffsets__sharedmutex__};
    if (ptr == nullptr)
        ptr = std::make_shared<RbxStuOffsets>();
    return ptr;
}

__declspec(dllexport) void* RbxStuOffsets::GetOffset(OffsetKey key)
{
    auto it = this->offsets.find(key);
    if (it != this->offsets.end())
        return it->second;

    throw std::runtime_error(std::format("{} is not valid offset key", static_cast<int>(key)));
}

__declspec(dllexport) void RbxStuOffsets::SetOffset(OffsetKey key, void* func)
{
    this->offsets[key] = func;
}
