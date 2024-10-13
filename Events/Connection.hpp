//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <functional>

namespace RbxStu::Events {
    template<typename T>
    class Connection final {
        std::function<T> m_function;

    public:
        explicit Connection(std::function<T(T)> f) : m_function(f) {
        }

        void Fire(T arg) {
            m_function(arg);
        }

        std::function<T> GetBackingFunction() {
            return this->m_function;
        };
    };
} // RbxStu
