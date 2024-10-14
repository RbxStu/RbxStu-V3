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

template <typename T>
struct VMValue1
{
public:
    operator const T() const
    {
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(storage) - reinterpret_cast<uintptr_t>(this));
    }

    void operator=(const T& value)
    {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) + reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const
    {
        return operator const T();
    }

private:
    T storage;
};

template <typename T>
struct VMValue2
{
public:
    operator const T() const
    {
        return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(this) - reinterpret_cast<uintptr_t>(storage));
    }

    void operator=(const T& value)
    {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(this) - reinterpret_cast<uintptr_t>(value));
    }

    const T operator->() const
    {
        return operator const T();
    }

private:
    T storage;
};

template <typename T>
struct VMValue4
{
public:
    operator const T() const
    {
        // address value
        return reinterpret_cast<T>(reinterpret_cast <uintptr_t>(storage) ^ reinterpret_cast<uintptr_t>(this));
    }

    void operator=(const T& value)
    {
        storage = reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) ^ reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const
    {
        return operator const T();
    }

private:
    T storage;
};

template <typename T>
struct VMValue3
{
public:
    operator const T() const
    {
        return reinterpret_cast<T>(reinterpret_cast <uintptr_t>(this) + reinterpret_cast<uintptr_t>(storage));
    }

    void operator=(const T& value)
    {
        storage = reinterpret_cast<T>(reinterpret_cast <uintptr_t>(value) - reinterpret_cast<uintptr_t>(this));
    }

    const T operator->() const
    {
        return operator const T();
    }

private:
    T storage;
};

namespace RbxStu
{
    class Security
    {
        static std::shared_ptr<Security> pInstance;
        std::atomic_bool m_bIsInitialized;
        VMValue3<const char*> originalHashedMemory{};

        void Initialize();

    public:
        bool IsInitialized();
        __forceinline const char* GetHashedMemory() const;
        __forceinline static std::string HashModuleSections(LPVOID lpBaseOfDll, DWORD SizeOfImage);
        static std::shared_ptr<Security> GetSingleton();
        __forceinline static std::string HashBytes(const byte* data, size_t length);
    };
}
