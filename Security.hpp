//
// Created by Pixeluted on 14/10/2024.
//
#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <sha.h>
#include <hex.h>
#include <Psapi.h>
#include "Settings.hpp"
#include <Logger.hpp>
#include <Utilities.hpp>
#include <Dependencies/libhat/include/libhat.hpp>
#include "oxorany/oxorany.h"

#define oxorany_converted(x) oxorany(static_cast<LPCSTR>(x))

template<typename T>
struct VMValue1 {
public:
    operator const T() const {
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(storage) - reinterpret_cast<uintptr_t>(this));
    }

    void operator=(const T &value) {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) + reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const {
        return operator const T();
    }

private:
    T storage;
};

template<typename T>
struct VMValue2 {
public:
    operator const T() const {
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(this) - reinterpret_cast<uintptr_t>(storage));
    }

    void operator=(const T &value) {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(this) - reinterpret_cast<uintptr_t>(value));
    }

    const T operator->() const {
        return operator const T();
    }

private:
    T storage;
};

template<typename T>
struct VMValue4 {
public:
    operator const T() const {
        // address value
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(storage) ^ reinterpret_cast<uintptr_t>(this));
    }

    void operator=(const T &value) {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) ^ reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const {
        return operator const T();
    }

private:
    T storage;
};

template<typename T>
struct VMValue3 {
public:
    operator const T() const {
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(this) + reinterpret_cast<uintptr_t>(storage));
    }

    void operator=(const T &value) {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) - reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const {
        return operator const T();
    }

private:
    T storage;
};

#define FILLER_GROUP_1 VMValue2
#define FILLER_GROUP_2 VMValue3
#define FILLER_GROUP_3 VMValue1
#define FILLER_GROUP_4 VMValue4

#define HASHED_MEMORY_VMVALUE VMValue3
#define LAST_RAN_VMVALUE VMValue1
#define SECURITY_CONTEXT_POINTER_VMVALUE VMValue4

#define SECURITY_CONTEXT_SHUFFLE(s, a1, a2, a3, a4, a5, a6, a7, a8) a6 s a2 s a7 s a5 s a4 s a3 s a8 s a1

struct SecurityContext {
    SECURITY_CONTEXT_SHUFFLE(
        ;,
        FILLER_GROUP_1<const char*> filler_1{},
        FILLER_GROUP_2<const char*> filler_2{},
        FILLER_GROUP_3<const char*> filler_3{},
        HASHED_MEMORY_VMVALUE<const char*> originalHashedMemory{},
        LAST_RAN_VMVALUE<time_t*> lastRan{},
        FILLER_GROUP_4<const char*> filler_4{},
        FILLER_GROUP_2<const char*> filler_5{},
        FILLER_GROUP_1<const char*> filler_6{}
    );
};

namespace RbxStu {
    class Security {
        static std::shared_ptr<Security> pInstance;
        std::atomic_bool m_bIsInitialized;

        void Initialize();

    public:
        SECURITY_CONTEXT_POINTER_VMVALUE<SecurityContext*> securityContext{};

        bool IsInitialized();

        __forceinline const char *GetHashedMemory() const;

        __forceinline static std::string HashModuleSections(LPVOID lpBaseOfDll, DWORD SizeOfImage);

        static std::shared_ptr<Security> GetSingleton();

        __forceinline static std::string HashBytes(const byte *data, size_t length);
    };
}
