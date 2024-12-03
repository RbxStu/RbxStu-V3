//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "Roblox/TypeDefinitions.hpp" // Concepts

namespace RbxStu::Miscellaneous {
    template<typename T>
    struct EventArgument {
        static_assert(sizeof(T) != 0);
        std::unique_ptr<T> value;
    };

    template<typename Y, RbxStu::Concepts::TypeConstraint<EventArgument<Y>> T>
    class ListenableFireableObject final {
        std::function<void(const T &)> m_dispatcher;

    public:
        explicit ListenableFireableObject(std::function<void(const T &)> func) { this->m_dispatcher = func; };

        void Fire(T arg) { this->m_dispatcher(arg); }
    };

    template<typename Y, RbxStu::Concepts::TypeConstraint<EventArgument<Y>> T>

    class ListenableEvent final {
        // address, function [provides a fairly unique manner to map functions]
        std::unordered_map<std::uintptr_t, std::function<void(const T &)>> m_functionList;

        std::uintptr_t CreateAttachId() {
            std::uintptr_t id{};

            for (auto &f: this->m_functionList)
                if (id < f.first)
                    id = f.first;

            return id++;
        }

    public:
        ListenableEvent() = default;

        ListenableFireableObject<Y, T> GetFirableObject() {
            return ListenableFireableObject<Y, T>([this](const T &arg) {
                for (auto &f: this->m_functionList)
                    f.second(arg);
            });
        }

        bool IsAttached(std::uintptr_t attachId) { return this->m_functionList.contains(attachId); }

        std::uintptr_t AttachFunction(std::function<void(const T &)> func) {
            std::uintptr_t attachId = this->CreateAttachId();
            this->m_functionList.insert({attachId, func});
            return attachId;
        }
    };

} // namespace RbxStu::Miscellaneous
