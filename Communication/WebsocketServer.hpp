//
// Created by Pixeluted on 15/10/2024.
//
#pragma once

#include <Logger.hpp>
#include <memory>


namespace RbxStu::Communication {
    class WebsocketServer {
        static std::shared_ptr<WebsocketServer> pInstance;
        bool m_bIsInitialized = false;

    public:
        static std::shared_ptr<WebsocketServer> GetSingleton();


        bool IsInitialized() const;
        void Initialize();
    };
}
