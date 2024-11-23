//
// Created by Dottik on 16/11/2024.
//

#include "FileSystem.hpp"
#include <unordered_set>
#include <string_view>
#include <string>
#include <Utilities.hpp>

const auto s_bannedExtensions = std::unordered_set<std::string_view>{
    "exe", "dll", "sys", "py", "js", "jsx",
    "drv", "ws", "wmf", "dev", "jar", "scr",
    "swf", "lnk", "vbs", "bin", "class", "shs",
    "chm", "vxd", "pif", "xlm", "vbe", "scr",
    "vba", "hlp", "vb", "vbx", "wsc", "wsh",
    "xlv", "bat", "cmd", "ocx", "com", "bin",
    "wmf",
};

static std::filesystem::path s_workspaceRoot{};

static bool IsFileExtensionSafe(std::filesystem::path path) {
    return !s_bannedExtensions.contains(RbxStu::Utilities::ToLower(path.extension().string()));
}

static bool IsWorkspaceAvailable() {
    return !s_workspaceRoot.empty() && std::filesystem::exists(s_workspaceRoot);
}

static bool IsPathSafe(const std::filesystem::path &path) {
    /*
     *  In play, it is super simple, we have approximately three approaches towards handling this scenario.
     *      - Approach 0: Grab the base path we want to check subdirectory off, and simply compare it using strstr to the path we want to check if it is a subdirectory of.
     *      - Approach 1: Evaluate the path using an API and check for the subdirectory.
     *      - Approach 2: Check if the path is not a subdirectory of workspace.
     *
     *  Might I remind we only need to check if the path is SAFE, we do not care about extensions on this function. The ONLY need in this function is to check for the paths' validity.
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

namespace RbxStu::StuLuau::Environment::UNC {
    /*
     *  The UNC spec never defines any errors for filesystem functions.
     */

    int FileSystem::writefile(lua_State *L) {
        std::size_t pathSize{};
        const auto path = luaL_tolstring(L, 1, &pathSize);
        std::size_t contentSize{};
        const auto content = luaL_tolstring(L, 2, &contentSize);

        const std::filesystem::path pathToWrite(std::string(path, pathSize));

        if (!IsPathSafe(pathToWrite))
            return 0;

        if (!std::filesystem::is_regular_file(pathToWrite))
            return 0;

        if (!IsFileExtensionSafe(pathToWrite))
            return 0;


        return 0;
    }

    const luaL_Reg *FileSystem::GetFunctionRegistry() {
        static luaL_Reg closuresLib[] = {
            // {"writefile", RbxStu::StuLuau::Environment::UNC::FileSystem::writefile},
            {nullptr, nullptr},
        };
        return closuresLib;
    }

    bool FileSystem::PushToGlobals() { return true; }

    const char *FileSystem::GetLibraryName() { return "filesystem"; }
} // namespace RbxStu::StuLuau::Environment::UNC
