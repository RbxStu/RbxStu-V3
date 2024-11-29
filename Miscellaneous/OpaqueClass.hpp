//
// Created by Dottik on 29/11/2024.
//

#pragma once

namespace RbxStu::Miscellaneous {

    class OpaqueClass abstract {
        void *m_pNativeObject;

    public:
        explicit OpaqueClass(void *pNative) { this->m_pNativeObject = pNative; }

        [[nodiscard]] void *GetRealStructure() const { return this->m_pNativeObject; }
    };

} // namespace RbxStu::Miscellaneous
