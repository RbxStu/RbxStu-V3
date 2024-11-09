//
// Created by Pixeluted on 15/10/2024.
//
#pragma once

#include <Logger.hpp>
#include <memory>

#define USE_WEBSOCKET_SERVER false

namespace RbxStu::Communication {
    class WebsocketCommunication {
        static std::shared_ptr<WebsocketCommunication> pInstance;
        bool m_bIsInitialized = false;

        int websocketPort = 7777;
    public:
        static std::shared_ptr<WebsocketCommunication> GetSingleton();

        [[nodiscard]] bool IsInitialized() const;
        void Initialize();
    };
}
