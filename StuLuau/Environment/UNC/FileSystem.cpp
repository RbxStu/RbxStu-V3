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

const auto s_bannedExtensions = std::unordered_set<std::string_view>{
        "exe", "dll", "sys", "py",    "js",  "jsx", "drv", "ws",  "wmf", "dev", "jar", "scr", "swf",
        "lnk", "vbs", "bin", "class", "shs", "chm", "vxd", "pif", "xlm", "vbe", "scr", "vba", "hlp",
        "vb",  "vbx", "wsc", "wsh",   "xlv", "bat", "cmd", "ocx", "com", "bin", "wmf",
};

static std::filesystem::path s_workspaceRoot{};

static bool IsFileExtensionSafe(std::filesystem::path path) {
    return !s_bannedExtensions.contains(RbxStu::Utilities::ToLower(path.extension().string()));
}

static bool IsWorkspaceAvailable() { return !s_workspaceRoot.empty() && std::filesystem::exists(s_workspaceRoot); }

static bool IsPathSafe(const std::filesystem::path &path) {
    /*
     *  In play, it is super simple, we have approximately three approaches towards handling this scenario.
     *      - Approach 0: Grab the base path we want to check subdirectory off, and simply compare it using strstr to
     * the path we want to check if it is a subdirectory of.
     *      - Approach 1: Evaluate the path using an API and check for the subdirectory.
     *      - Approach 2: Check if the path is not a subdirectory of workspace.
     *
     *  Might I remind we only need to check if the path is SAFE, we do not care about extensions on this function. The
     * ONLY need in this function is to check for the paths' validity.
     *
     */

    if (!IsWorkspaceAvailable()) {
        // initialize workspace.
        // Calculate workspace path.
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

    if (!absolutePath.string().contains(s_workspaceRoot.string()))
        return false; // Path does not contain workspace.

    if (absolutePath.string().contains(".."))
        return false;

    return true;
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
            return 0;

        if (!std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)))
            return 0;

        if (!IsFileExtensionSafe(pathToWrite))
            return 0;


        std::ofstream file(GetNormalizedPath(pathToWrite), std::ios::trunc | std::ios::out);

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
            return 0;

        if (!std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)))
            return 0;

        if (!IsFileExtensionSafe(pathToWrite))
            return 0;


        std::ofstream file(GetNormalizedPath(pathToWrite), std::ios::ate | std::ios::out);

        file << std::string(content, contentSize);
        file.flush();
        file.close();

        return 0;
    }

    int FileSystem::readfile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            return 0;

        if (!std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)))
            return 0;

        if (!IsFileExtensionSafe(pathToWrite))
            return 0;


        std::ifstream file(GetNormalizedPath(pathToWrite), std::ios::in);

        std::string content{};

        file >> content;
        file.close();

        lua_pushlstring(L, content.c_str(), content.size());
        return 1;
    }

    int FileSystem::makefolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            return 0;

        if (std::filesystem::is_directory(pathToWrite))
            return 0;

        std::filesystem::create_directory(pathToWrite);

        return 0;
    }

    int FileSystem::delfolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            return 0;

        if (std::filesystem::is_directory(pathToWrite))
            return 0;

        std::filesystem::remove_all(pathToWrite);

        return 0;
    }

    int FileSystem::delfile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = GetNormalizedPath(std::filesystem::path(std::string(path, pathSize)));

        if (!IsPathSafe(pathToWrite))
            return 0;

        if (!std::filesystem::is_regular_file(pathToWrite))
            return 0;

        std::filesystem::remove(pathToWrite);

        return 0;
    }

    int FileSystem::isfile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        lua_pushboolean(L, IsPathSafe(pathToWrite) && std::filesystem::is_regular_file(GetNormalizedPath(pathToWrite)));
        return 1;
    }

    int FileSystem::isfolder(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        lua_pushboolean(L, IsPathSafe(pathToWrite) && std::filesystem::is_directory(GetNormalizedPath(pathToWrite)));
        return 1;
    }

    int FileSystem::listfiles(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);

        const auto pathToWrite = (std::filesystem::path(std::string(path, pathSize)));

        lua_newtable(L);

        if (!IsPathSafe(pathToWrite) || !std::filesystem::is_directory(GetNormalizedPath(pathToWrite))) {
            return 1;
        }

        auto idx = 1;
        for (const auto &entry: std::filesystem::directory_iterator(GetNormalizedPath(pathToWrite))) {
            lua_pushstring(L, entry.path().string().c_str());
            lua_rawseti(L, -2, idx++);
        }

        return 1;
    }

    int FileSystem::loadfile(lua_State *L) {
        luaL_checkstring(L, 1);
        const auto chunkName = luaL_optstring(L, 2, "=loadfile");
        lua_normalisestack(L, 1);

        lua_pushcclosure(L, RbxStu::StuLuau::Environment::UNC::FileSystem::readfile, nullptr, 0);
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        std::size_t codeSize{};
        auto code = std::string(lua_tolstring(L, 1, &codeSize), codeSize);

        Luau::CompileOptions compileOpts{};
        compileOpts.optimizationLevel = 1;
        compileOpts.debugLevel = 2;

        const auto bytecode = Luau::compile(code, compileOpts);

        if (luau_load(L, chunkName, bytecode.data(), bytecode.size(), 0) != LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        LuauSecurity::GetSingleton()->ElevateClosure(lua_toclosure(L, -1),
                                                     RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);

        return 1;
    }
    int FileSystem::dofile(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_pushcclosure(L, RbxStu::StuLuau::Environment::UNC::FileSystem::loadfile, nullptr, 0);

        lua_pushvalue(L, 1);
        lua_call(L, 1, 2);

        if (lua_type(L, -2) == ::lua_Type::LUA_TFUNCTION) {
            lua_pop(L, 1);
            lua_remove(L, 1); // Remove filename.
            lua_insert(L, 1);

            auto nL = lua_newthread(L);

            lua_xmove(L, nL, lua_gettop(L) - 1);

            const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

            task_defer(nL);

            return 0;
        }

        lua_error(L); // Error string at stack top.
    }

    const luaL_Reg *FileSystem::GetFunctionRegistry() {
        static luaL_Reg closuresLib[] = {
                {"readfile", RbxStu::StuLuau::Environment::UNC::FileSystem::readfile},
                {"appendfile", RbxStu::StuLuau::Environment::UNC::FileSystem::appendfile},
                {"writefile", RbxStu::StuLuau::Environment::UNC::FileSystem::writefile},
                {"isfile", RbxStu::StuLuau::Environment::UNC::FileSystem::isfile},
                {"isfile", RbxStu::StuLuau::Environment::UNC::FileSystem::isfolder},
                {"listfiles", RbxStu::StuLuau::Environment::UNC::FileSystem::makefolder},
                {"dofile", RbxStu::StuLuau::Environment::UNC::FileSystem::dofile},

                {"delfile", RbxStu::StuLuau::Environment::UNC::FileSystem::delfile},
                {"delfolder", RbxStu::StuLuau::Environment::UNC::FileSystem::delfolder},

                {"makefolder", RbxStu::StuLuau::Environment::UNC::FileSystem::makefolder},
                {nullptr, nullptr},
        };
        return closuresLib;
    }

    bool FileSystem::PushToGlobals() { return true; }

    const char *FileSystem::GetLibraryName() {
        IsPathSafe(""); // Stub to create workspace.
        return "filesystem";
    }
} // namespace RbxStu::StuLuau::Environment::UNC
