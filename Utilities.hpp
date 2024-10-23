//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <filesystem>
#include <hex.h>
#include <Logger.hpp>
#include <memory>
#include <regex>
#include <sha.h>
#include <future>
#include <map>
#include <vector>
#include <libhat/Scanner.hpp>

#include <sstream>

#include <Windows.h>

#include "lualib.h"

namespace RbxStu {
    class Utilities {
        static std::shared_ptr<Utilities> pInstance;
        std::atomic_bool m_bIsInitialized;
        std::regex m_luaErrorStringRegex;

        void Initialize();

    public:
        bool IsInitialized();

        static std::shared_ptr<Utilities> GetSingleton();

        std::string FromLuaErrorMessageToCErrorMessage(const std::string &luauMessage) const;

        static std::string WcharToString(const wchar_t *wideStr);

        static std::string ToLower(std::string target);

        static std::string ToUpper(std::string target);

        __forceinline static bool IsWine() {
            return GetProcAddress(GetModuleHandle("ntdll.dll"), "wine_get_version") != nullptr;
        }

        __forceinline static std::optional<const std::string> GetHwid() {
            auto logger = RbxStu::Logger::GetSingleton();
            HW_PROFILE_INFO hwProfileInfo;
            if (!GetCurrentHwProfileA(&hwProfileInfo)) {
                RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous, "Failed to retrieve Hardware ID");
                return {};
            }

            CryptoPP::SHA256 sha256;
            unsigned char digest[CryptoPP::SHA256::DIGESTSIZE];
            sha256.CalculateDigest(digest, reinterpret_cast<unsigned char *>(hwProfileInfo.szHwProfileGuid),
                                   sizeof(hwProfileInfo.szHwProfileGuid));

            CryptoPP::HexEncoder encoder;
            std::string output;
            encoder.Attach(new CryptoPP::StringSink(output));
            encoder.Put(digest, sizeof(digest));
            encoder.MessageEnd();

            return output;
        }

        __forceinline static void GetService(lua_State *L, const std::string &serviceName) {
            lua_getglobal(L, "game");
            lua_getfield(L, -1, "GetService");
            lua_pushvalue(L, -2);
            lua_pushstring(L, serviceName.c_str());
            lua_pcall(L, 2, 1, 0);
            lua_remove(L, -2);
        }

        __forceinline static std::string GetDllDir() {
            HMODULE hModule = nullptr;

            if (char path[MAX_PATH];
                GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  "RbxStuV3.dll", &hModule) &&
                GetModuleFileNameA(hModule, path, sizeof(path))) {
                const std::filesystem::path fullPath(path);
                return fullPath.parent_path().string();
            }

            return "";
        }

        __forceinline static std::vector<std::string> SplitBy(const std::string &target, const char split) {
            std::vector<std::string> splitted;
            std::stringstream stream(target);
            std::string temporal;
            while (std::getline(stream, temporal, split)) {
                splitted.push_back(temporal);
                temporal.clear();
            }

            return splitted;
        }

        __forceinline static std::pair<bool, std::string> getInstanceType(lua_State *L, const int index) {
            luaL_checktype(L, index, LUA_TUSERDATA);

            lua_getglobal(L, "typeof");
            lua_pushvalue(L, index);
            lua_call(L, 1, 1);

            if (const bool isInstance = (strcmp(lua_tostring(L, -1), "Instance") == 0); !isInstance) {
                const auto str = lua_tostring(L, -1);
                lua_pop(L, 1);
                return {false, str};
            }
            lua_pop(L, 1);

            lua_getfield(L, index, "ClassName");

            const auto className = lua_tostring(L, -1);
            lua_pop(L, 1);
            return {true, className};
        }

        __forceinline static void checkInstance(lua_State *L, const int index, const char *expectedClassname) {
            luaL_checktype(L, index, LUA_TUSERDATA);

            lua_getglobal(L, "typeof");
            lua_pushvalue(L, index);
            lua_call(L, 1, 1);
            const bool isInstance = (strcmp(lua_tostring(L, -1), "Instance") == 0);
            lua_pop(L, 1);

            if (!isInstance)
                luaL_argerror(L, index, "expected an Instance");

            if (strcmp(expectedClassname, "ANY") == 0)
                return;

            lua_getfield(L, index, "IsA");
            lua_pushvalue(L, index);
            lua_pushstring(L, expectedClassname);
            lua_call(L, 2, 1);
            const bool isExpectedClass = lua_toboolean(L, -1);
            lua_pop(L, 1);

            if (!isExpectedClass)
                luaL_argerror(L, index, std::format("Expected to be {}", expectedClassname).c_str());
        }

        template<typename T>
        static std::map<T, hat::scan_result> ScanMany(
            std::map<T, hat::signature> signatures,
            const bool parallelScan,
            const char *targetSection) {
            std::vector<std::future<std::pair<T, hat::scan_result> > > futures{};

            for (const auto sig: signatures) {
                futures.emplace_back(std::async(parallelScan ? std::launch::async : std::launch::deferred,
                                                [sig, targetSection]() {
                                                    return std::make_pair(
                                                        sig.first, hat::find_pattern(sig.second, targetSection));
                                                }));
            }

            std::map<T, hat::scan_result> results = {};
            for (auto &future: futures) {
                future.wait();
                auto result = future.get();
                results.emplace(result);
            }

            return results;
        }

        __forceinline static std::string GetCurrentDllName() {
            char modulePath[MAX_PATH];
            HMODULE hModule = nullptr;

            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&GetCurrentDllName),
                                   &hModule) != 0) {
                if (GetModuleFileNameA(hModule, modulePath, sizeof(modulePath)) != 0) {
                    std::string fullPath = modulePath;
                    size_t lastSlash = fullPath.find_last_of("\\/");
                    if (lastSlash != std::string::npos) {
                        return fullPath.substr(lastSlash + 1);
                    }
                    return fullPath;
                }
            }

            return "";
        }
    };
} // RbxStu
