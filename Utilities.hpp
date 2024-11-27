//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <Windows.h>

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
#include <tlhelp32.h>

#include "lualib.h"
#include "ludata.h"

namespace RbxStu {
    class Utilities {
        static std::shared_ptr<Utilities> pInstance;
        std::atomic_bool m_bIsInitialized;
        std::regex m_luaErrorStringRegex;

        void Initialize();

    public:
        struct ThreadInformation {
            bool bWasSuspended;
            HANDLE hThread;
        };

        enum ThreadSuspensionState {
            SUSPENDED,
            RESUMED,
        };

    public:
        class RobloxThreadSuspension {
            std::vector<ThreadInformation> threadInformation;
            ThreadSuspensionState state;

        public:
            explicit RobloxThreadSuspension(const bool suspendOnCreate) {
                this->state = RESUMED;
                if (suspendOnCreate)
                    this->SuspendThreads();
            }

            ~RobloxThreadSuspension() {
                const auto logger = Logger::GetSingleton();
                for (auto &[_, hThread]: this->threadInformation) {
                    if (state == SUSPENDED)
                        ResumeThread(hThread);

                    CloseHandle(hThread);
                }
            }

            void SuspendThreads() {
                const auto logger = Logger::GetSingleton();
                if (this->state != RESUMED) {
                    return;
                }
                const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

                if (hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == nullptr) {
                    throw std::exception("PauseRobloxThreads failed: Snapshot creation failed!");
                }

                THREADENTRY32 te{0};
                te.dwSize = sizeof(THREADENTRY32);

                if (!Thread32First(hSnapshot, &te)) {
                    CloseHandle(hSnapshot);
                    throw std::exception("PauseRobloxThreads failed: Thread32First failed!");
                }
                const auto currentPid = GetCurrentProcessId();
                std::vector<ThreadInformation> thInfo;
                do {
                    if (te.th32ThreadID != GetCurrentThreadId() && te.th32OwnerProcessID == currentPid) {
                        auto hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);

                        auto th = ThreadInformation{false, nullptr};
                        if (SuspendThread(hThread) > 1)
                            th.bWasSuspended = true;
                        th.hThread = hThread;
                        thInfo.push_back(th);
                    }
                } while (Thread32Next(hSnapshot, &te));

                this->threadInformation = thInfo;
                this->state = SUSPENDED;
            }

            void ResumeThreads() {
                const auto logger = Logger::GetSingleton();
                if (this->state != SUSPENDED) {
                    return;
                }
                for (auto &[bWasSuspended, hThread]: this->threadInformation) {
                    ResumeThread(hThread);
                }
                this->state = RESUMED;
            }
        };

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

        __forceinline static std::optional<std::filesystem::path> GetDllDir() {
            HMODULE hModule = nullptr;

            if (!GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(&GetDllDir),
                &hModule
            )) {
                return std::nullopt;
            }

            char path[MAX_PATH];
            if (!GetModuleFileNameA(hModule, path, sizeof(path))) {
                return std::nullopt;
            }

            try {
                const std::filesystem::path fullPath(path);
                return fullPath.parent_path();
            } catch (...) {
                return std::nullopt;
            }
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
            if (lua_type(L, index) != LUA_TUSERDATA)
                return {false, luaL_typename(L, index)};

            if (lua_touserdatatagged(L, index, UTAG_PROXY) != nullptr)
                return {false, "FAKEINSTANCE::NEWPROXY"};

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

        __forceinline static void createInstance(lua_State *L, const std::string_view className,
                                                 const std::string_view instanceName) {
            lua_getglobal(L, "Instance");
            lua_getfield(L, -1, "new");
            lua_remove(L, -2);
            lua_pushstring(L, className.data());
            lua_call(L, 1, 1);
            lua_pushstring(L, instanceName.data());
            lua_setfield(L, -2, "Name");
        }

        __forceinline static void checkInstance(lua_State *L, const int index, const char *expectedClassname) {
            luaL_checktype(L, index, LUA_TUSERDATA);

            if (lua_touserdatatagged(L, index, UTAG_PROXY) != nullptr)
                luaL_argerror(
                L, index, std::format("expected to be {}, got userdata<newproxy>", expectedClassname).c_str());

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
                luaL_argerror(L, index, std::format("expected to be {}", expectedClassname).c_str());
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

        /// @brief Used to validate a pointer.
        /// @remarks This template does NOT validate ANY data inside the pointer. It just validates that the pointer is
        /// at LEAST of the size of the given type, and that the pointer is allocated in memory.
        template<typename T>
        __forceinline static bool IsPointerValid(T *tValue) {
            // Validate pointers.
            const auto ptr = reinterpret_cast<const void *>(const_cast<const T *>(tValue));
            auto buf = MEMORY_BASIC_INFORMATION{};

            // Query a full page.
            if (const auto read = VirtualQuery(ptr, &buf, sizeof(buf)); read != 0 && sizeof(buf) != read) {
                // I honestly dont care.
            } else if (read == 0) {
                return false;
            }

            if (buf.RegionSize < sizeof(T)) {
                return false; // Allocated region is too small to fit type T inside.
            }

            if (buf.State & MEM_FREE == MEM_FREE) {
                return false; // The memory is not owned by the process, no need to do anything, we can already assume
                // we cannot read it.
            }

            auto validProtections = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                                    PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

            if (buf.Protect & validProtections) {
                return true;
            }
            if (buf.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
                return false;
            }

            return true;
        }
    };
} // RbxStu
