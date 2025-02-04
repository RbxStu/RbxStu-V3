//
// Created by Dottik on 16/11/2024.
//

#include "FileSystem.hpp"
#include <Utilities.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_set>

#include "Luau/Compiler.h"
#include "StuLuau/Extensions/luauext.hpp"
#include "StuLuau/LuauSecurity.hpp"
#include "lbuffer.h"
#include "ldebug.h"

const auto s_bannedExtensions = std::unordered_set<std::string_view>{
        ".exe", ".dll", ".sys", ".py",    ".js",  ".jsx", ".drv", ".ws",  ".wmf", ".dev", ".jar", ".scr", ".swf",
        ".lnk", ".vbs", ".bin", ".class", ".shs", ".chm", ".vxd", ".pif", ".xlm", ".vbe", ".scr", ".vba", ".hlp",
        ".vb",  ".vbx", ".wsc", ".wsh",   ".xlv", ".bat", ".cmd", ".ocx", ".com", ".bin", ".wmf", ".pyw", ".iso",
        ".ps1"
}; // TODO: https://github.com/dobin/badfiles/blob/main/info.yaml

static std::filesystem::path s_workspaceRoot{};

static bool IsFileExtensionSafe(std::filesystem::path path) {
    return !s_bannedExtensions.contains(RbxStu::Utilities::ToLower(path.extension().string()));
}

static bool IsWorkspaceAvailable() { return !s_workspaceRoot.empty() && std::filesystem::exists(s_workspaceRoot); }

static bool IsPathSafe(const std::filesystem::path &path) {
    if (!IsWorkspaceAvailable()) {
        const auto currentDirectory = RbxStu::Utilities::GetDllDir();
        if (currentDirectory->empty()) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "FAILED TO DETERMINE RbxStuV3's LUAU WORKSPACE! THIS WILL RESULT IN NO IO FUNCTION WORKING!");
            return false;
        }

        const auto &dllDir = currentDirectory.value();
        s_workspaceRoot = std::filesystem::absolute(dllDir / "workspace").lexically_normal();

        if (!std::filesystem::exists(s_workspaceRoot))
            std::filesystem::create_directory(s_workspaceRoot);
    }

    const auto absolutePath = std::filesystem::absolute(s_workspaceRoot / path).lexically_normal();

    return absolutePath.string().starts_with(s_workspaceRoot.string());
}

static std::filesystem::path GetNormalizedPath(const std::filesystem::path &path) {
    return std::filesystem::absolute(s_workspaceRoot / path).lexically_normal();
}

namespace RbxStu::StuLuau::Environment::UNC {
    /*
     *  The UNC spec never defines any errors for filesystem functions.
     */

    int FileSystem::writefile(lua_State *L) {
        luaL_checkstring(L, 1);
        luaL_checkstring(L, 2);
        lua_normalisestack(L, 2);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);
        std::size_t contentSize{};
        const auto content = luaL_tolstring(L, 2, &contentSize);

        const std::filesystem::path pathToWrite(std::string(path, pathSize));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (!IsFileExtensionSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Illegal extension '" + RbxStu::Utilities::ToLower(pathToWrite.extension().string()) + "'").c_str());

        std::ofstream file(GetNormalizedPath(pathToWrite), std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            luaL_error(L, "Failed to open file handle");

        file << std::string(content, contentSize);
        file.flush();
        file.close();

        return 0;
    }

    int FileSystem::appendfile(lua_State *L) {
        luaL_checkstring(L, 1);
        luaL_checkstring(L, 2);
        lua_normalisestack(L, 2);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);
        std::size_t contentSize{};
        const auto content = luaL_tolstring(L, 2, &contentSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (!std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)))
            luaG_runerrorL(L, "This file doesn't exist");

        if (!IsFileExtensionSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Illegal extension '" + RbxStu::Utilities::ToLower(pathToWrite.extension().string()) + "'").c_str());

        std::ofstream file(GetNormalizedPath(pathToWrite), std::ios::binary | std::ios::app);

        if (!file.is_open())
            luaL_error(L, "Failed to open file handle");

        const auto contentToAppend = std::string(content, contentSize);

        file << contentToAppend;
        file.flush();
        file.close();

        return 0;
    }

    int FileSystem::readfile(lua_State *L) {
        luaL_checkstring(L, 1);
        auto readAsBuffer = luaL_optboolean(L, 2, false);
        lua_normalisestack(L, 2);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (!std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)))
            luaG_runerrorL(L, "This file doesn't exist");

        if (std::filesystem::file_size(GetNormalizedPath(pathToWrite)) >
            MAX_BUFFER_SIZE) { // MAX_BUFFER_SIZE == MAXSIZE (max string size)
            luaL_error(L, "File content too big.");
        }

        std::ifstream file(GetNormalizedPath(pathToWrite), std::ios::in | (readAsBuffer ? std::ios::binary : 0));

        const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        file.close();

        if (readAsBuffer) {
            const auto bufferData = lua_newbuffer(L, content.size());
            memcpy(bufferData, content.data(), content.size());
        } else {
            lua_pushlstring(L, content.c_str(), content.size());
        }

        return 1;
    }

    int FileSystem::makefolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (std::filesystem::is_directory(pathToWrite))
            return 0;

        std::filesystem::create_directories(pathToWrite);

        return 0;
    }

    int FileSystem::delfolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (!std::filesystem::is_directory(pathToWrite))
            luaG_runerrorL(L, "This folder doesn't exist");

        std::error_code err{};

        if (!std::filesystem::remove_all(pathToWrite, err))
            luaL_error(L, "failed to delete folder and all subfolders with error: '%s'", err.message().c_str());

        return 0;
    }

    int FileSystem::delfile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        if (!std::filesystem::is_regular_file(pathToWrite))
            luaG_runerrorL(L, "This file doesn't exist");

        std::error_code err{};

        if (!std::filesystem::remove(pathToWrite, err))
            luaL_error(L, "failed to delete file with error: '%s'", err.message().c_str());

        return 0;
    }

    int FileSystem::isfile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        lua_pushboolean(L, std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)));
        return 1;
    }

    int FileSystem::isfolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        lua_pushboolean(L, std::filesystem::is_directory(GetNormalizedPath(pathToWrite)));
        return 1;
    }

    int FileSystem::listfiles(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

            const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            luaL_argerrorL(L, 1, ("Escaping workspace folder '" + RbxStu::Utilities::ToLower(pathToWrite.string()) + "'").c_str());

        lua_newtable(L);

        if (!std::filesystem::is_directory(GetNormalizedPath(pathToWrite)))
            return 1;

        auto idx = 1;
        for (const auto &entry: std::filesystem::directory_iterator(GetNormalizedPath(pathToWrite))) {
            lua_pushstring(L, entry.path().lexically_relative(s_workspaceRoot).string().c_str());
            lua_rawseti(L, -2, idx++);
        }

        return 1;
    }

    int FileSystem::loadfile(lua_State *L) {
        luaL_checkstring(L, 1);
        const auto chunkName = luaL_optstring(L, 2, "=RbxStuV3");
        lua_normalisestack(L, 1);

        lua_pushcclosure(L, RbxStu::StuLuau::Environment::UNC::FileSystem::readfile, nullptr, 0);
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        std::size_t codeSize{};
        const auto codePtr = lua_tolstring(L, -1, &codeSize);
        auto code = std::string(codePtr, codeSize);

        Luau::CompileOptions compileOpts{};
        compileOpts.optimizationLevel = 1;
        compileOpts.debugLevel = 2;

        const auto bytecode = Luau::compile(code, compileOpts);

        if (luau_load(L, chunkName, bytecode.data(), bytecode.size(), 0) != LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false); // Env is no longer safe.

        auto currentCi = L->ci;
        while (currentCi != L->base_ci) {
            if (!clvalue(currentCi->func)->isC) {
                const auto masterProto = clvalue(currentCi->func)->l.p;
                const auto capabilities = *(std::uintptr_t *) masterProto->userdata;
                LuauSecurity::GetSingleton()->ElevateClosureWithExplicitCapabilities(
                        lua_toclosure(L, -1),
                        capabilities); // Grab the capabilities from the parent closure.
            }
            currentCi--;
        }

        return 1;
    }

    int FileSystem::dofile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        lua_pushcclosure(L, RbxStu::StuLuau::Environment::UNC::FileSystem::loadfile, nullptr, 0);

        lua_pushvalue(L, 1);
        lua_call(L, 1, LUA_MULTRET);

        if (lua_type(L, -1) == ::lua_Type::LUA_TFUNCTION) {
            lua_getglobal(L, "task");
            lua_getfield(L, -1, "spawn");
            lua_remove(L, -2); // Remove task table

            lua_pushvalue(L, -2);
            const auto callResults = lua_pcall(L, 1, LUA_MULTRET, 0);
            if (callResults != LUA_OK) {
                lua_error(L);
            }

            return 0;
        }

        lua_error(L); // Error string at stack top.
    }

    const luaL_Reg *FileSystem::GetFunctionRegistry() {
        static luaL_Reg filesystemLib[] = {
                {"readfile", RbxStu::StuLuau::Environment::UNC::FileSystem::readfile},
                {"writefile", RbxStu::StuLuau::Environment::UNC::FileSystem::writefile},
                {"appendfile", RbxStu::StuLuau::Environment::UNC::FileSystem::appendfile},
                {"isfile", RbxStu::StuLuau::Environment::UNC::FileSystem::isfile},
                {"isfolder", RbxStu::StuLuau::Environment::UNC::FileSystem::isfolder},

                {"listfiles", RbxStu::StuLuau::Environment::UNC::FileSystem::listfiles},
                {"dofile", RbxStu::StuLuau::Environment::UNC::FileSystem::dofile},
                {"loadfile", RbxStu::StuLuau::Environment::UNC::FileSystem::loadfile},
                {"delfile", RbxStu::StuLuau::Environment::UNC::FileSystem::delfile},
                {"delfolder", RbxStu::StuLuau::Environment::UNC::FileSystem::delfolder},
                {"makefolder", RbxStu::StuLuau::Environment::UNC::FileSystem::makefolder},
                {nullptr, nullptr},
        };

        return filesystemLib;
    }

    bool FileSystem::PushToGlobals() { return true; }

    const char *FileSystem::GetLibraryName() {
        IsPathSafe(""); // Stub to create workspace.
        return "filesystem";
    }
} // namespace RbxStu::StuLuau::Environment::UNC
