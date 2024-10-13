//
// Created by Dottik on 12/10/2024.
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Connection.hpp"

namespace RbxStu::Events {
    template<typename T>
    class Event final {
        std::vector<std::shared_ptr<RbxStu::Events::Connection<T> > > m_connections;

    public:
        std::shared_ptr<RbxStu::Events::Connection<T> > Connect(std::function<T> callback) {
            auto pConnection = std::make_shared<RbxStu::Events::Connection<T> >(callback);
            m_connections.emplace_back(pConnection);

            return pConnection;
        }

        void Disconnect(const RbxStu::Events::Connection<T> &connection) {
            m_connections.erase(std::find(m_connections.begin(), m_connections.end(), connection));
        }

        void Fire(T args) {
            for (auto pConnection: m_connections) {
                pConnection->Fire(args);
            }
        }
    };
} // RbxStu
