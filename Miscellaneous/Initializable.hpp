//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <atomic>

namespace RbxStu::Miscellaneous {
    class Initializable abstract {
    protected:
        std::atomic_bool m_bIsInitialized;

    public:
        virtual ~Initializable() = default;

        virtual bool IsInitialized();

        virtual bool Initialize();
    };
}
