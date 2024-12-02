//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <functional>
#include <unordered_set>

namespace RbxStu::Miscellaneous {
    template<typename T>
    class ListenableFireableObject final {
        std::function<void(const T &)> m_dispatcher;

    public:
        explicit ListenableFireableObject(std::function<void(const T &)> func) { this->m_dispatcher = func; };

        void Fire(T arg) { this->m_dispatcher(arg); }
    };

    template<typename T>
    class ListenableEvent final {
        // address, function [provides a fairly unique manner to map functions]
        std::unordered_map<std::uintptr_t, std::function<void(const T &)>> m_functionList;

    public:
        ListenableEvent() = delete;

        ListenableFireableObject<T> GetFirableObject() {
            return ListenableFireableObject<T>([this](const T &arg) {
                for (auto &f: this->m_functionList)
                    f.second(arg);
            });
        }

        static void AttachFunction(void(__fastcall *func)(T)) {
            if (m_functionList.contains(static_cast<std::uintptr_t>(func)))
                return;

            m_functionList.insert({static_cast<std::uintptr_t>(func), func});
        }
    };
} // namespace RbxStu::Miscellaneous
