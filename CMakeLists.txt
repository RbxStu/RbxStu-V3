cmake_minimum_required(VERSION 3.7.2)
cmake_policy(SET CMP0091 NEW)
project(RbxStuV3)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

add_definitions(-DLUAI_GCMETRICS)   # Force GC metrics on Luau.

option(CAPSTONE_X86_SUPPORT "Enable x86 capstone" ON)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)

if (build_type STREQUAL release)
    message("Build is in Release Mode; Adding custom Compiler options and Linker options.")
    add_compile_options(
            /MT
            /GA
            /EHa
            /bigobj
            /O2
            /Gr
            /GS
            /GF
            # /GL
            /MP
            /cgthreads8
            /arch:AVX2
    )

    add_link_options(
            /LTCG
            # /INCREMENTAL
            /DEBUG
            /DYNAMICBASE
            /HIGHENTROPYVA
            /GUARD:CF
            /PDB:RbxStuV3.pdb
            /VERBOSE
    )
endif ()

add_library(${PROJECT_NAME} SHARED
        Entry.cpp
        ExceptionHandler.cpp
        Logger.cpp
        Utilities.cpp

        # RbxStu V3 Settings for DLL building.
        Settings.hpp

        # RbxStu V3/ROBLOX
        Roblox/TypeDefinitions.hpp
        Roblox/DataModel.cpp
        Roblox/DataModel.hpp

        # RbxStu V3/Scheduling
        Scheduling/TaskScheduler.cpp
        Scheduling/TaskSchedulerOrchestrator.cpp

        Scheduling/Job.cpp

        # RbxStu V3/Scanners
        Scanners/Luau.cpp
        Scanners/Rbx.cpp

        # RbxStu V3/Scanners
        Analysis/Disassembler.cpp
        Analysis/RTTI.cpp
        Analysis/StringSearcher.cpp
        Analysis/StringSearcher.hpp
        Analysis/XrefSearcher.cpp
        Analysis/XrefSearcher.hpp
        Analysis/StringMatcher.cpp
        Analysis/StringMatcher.hpp
        Analysis/SignatureMatcher.cpp
        Analysis/SignatureMatcher.hpp

        Security.cpp
        Roblox/ScriptContext.cpp
        Roblox/ScriptContext.hpp
        Communication/WebsocketCommunication.cpp
        Communication/WebsocketCommunication.hpp
        Communication/PacketManager.cpp
        Communication/PacketManager.hpp
        Communication/PacketBase.hpp
        Communication/Packets/ScheduleExecution.hpp
        StuLuau/ExecutionEngine.cpp
        StuLuau/ExecutionEngine.hpp
        StuLuau/LuauSecurity.cpp
        StuLuau/LuauSecurity.hpp

        Scheduling/Job/InitializeExecutionEngineJob.cpp
        Scheduling/Job/InitializeExecutionEngineJob.hpp
        StuLuau/Environment/EnvironmentContext.cpp
        StuLuau/Environment/EnvironmentContext.hpp
        StuLuau/Environment/Library.cpp
        StuLuau/Environment/Library.hpp
        StuLuau/Environment/UNC/Closures.cpp
        StuLuau/Environment/UNC/Closures.hpp
        StuLuau/Environment/UNC/Globals.cpp
        StuLuau/Environment/UNC/Globals.hpp
        StuLuau/Environment/Custom/Memory.cpp
        StuLuau/Environment/Custom/Memory.hpp
        StuLuau/Environment/UNC/WebSocket.cpp
        StuLuau/Environment/UNC/WebSocket.hpp
        Scheduling/Job/ExecutionEngineStepJob.cpp
        Scheduling/Job/ExecutionEngineStepJob.hpp
        Scheduling/Job/DataModelWatcherJob.cpp
        Scheduling/Job/DataModelWatcherJob.hpp
        StuLuau/Interop/NativeObject.cpp
        StuLuau/Interop/NativeObject.hpp
        StuLuau/Environment/Custom/NewGlobals.cpp
        StuLuau/Environment/Custom/NewGlobals.hpp
        StuLuau/Environment/UNC/Cache.cpp
        StuLuau/Environment/UNC/Cache.hpp
        StuLuau/Environment/UNC/Scripts.cpp
        StuLuau/Environment/UNC/Scripts.hpp
        StuLuau/Environment/UNC/Crypt.cpp
        StuLuau/Environment/UNC/Crypt.hpp
        Scheduling/Job/ImguiRenderJob.cpp
        Scheduling/Job/ImguiRenderJob.hpp
        FastFlags.cpp
        FastFlags.hpp
        Communication/PipeCommunication.cpp
        Communication/PipeCommunication.hpp
        StuLuau/Extensions/luauext.hpp
        StuLuau/Environment/UNC/FileSystem.cpp
        StuLuau/Environment/UNC/FileSystem.hpp
        StuLuau/Environment/UNC/Debug.cpp
        StuLuau/Environment/UNC/Debug.hpp
        StuLuau/Environment/UNC/Miscellaneous.cpp
        StuLuau/Environment/UNC/Miscellaneous.hpp

        Render/Renderable.cpp
        Render/Renderable.hpp
        Render/ImmediateGui/Keycodes.hpp
        Render/UserInterface/UserInterface.cpp
        Render/UserInterface/UserInterface.hpp

        Miscellaneous/Initializable.cpp
        Miscellaneous/Initializable.hpp
        Miscellaneous/ListenableEvent.hpp
        Render/ImmediateGui/PagedWindow.cpp
        Render/ImmediateGui/PagedWindow.hpp
        Render/UserInterface/Pages/ExecutionPage.cpp
        Render/UserInterface/Pages/ExecutionPage.hpp
        Render/UserInterface/Pages/SettingsPage.cpp
        Render/UserInterface/Pages/SettingsPage.hpp
        Render/RenderableStub.cpp
        Render/RenderableStub.hpp
        Render/Color4.hpp
        Roblox/Script.cpp
        Roblox/Script.hpp
        Miscellaneous/OpaqueClass.hpp
        Render/UserInterface/Pages/CreditsPage.cpp
        Render/UserInterface/Pages/CreditsPage.hpp
        StuLuau/Environment/UNC/Instances.cpp
        StuLuau/Environment/UNC/Instances.hpp
        Roblox/Primitive.cpp
        Roblox/Primitive.hpp
        Roblox/World.cpp
        Roblox/World.hpp
)

if (build_type STREQUAL release)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY MultiThreaded
    )
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")
# Dependencies


# Curl For People
# FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
#         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8)
# FetchContent_MakeAvailable(cpr)

#rcmp
# FetchContent_Declare(rcmp GIT_REPOSITORY https://github.com/Smertig/rcmp.git
#         GIT_TAG f5f75ae00a57c67fe41f79fd59c4b7f6997b999e)
# FetchContent_MakeAvailable(rcmp)

# libhat
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/libhat")

# cpr
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/cpr")

# Luau
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Luau")

# minhook
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Minhook")

# IXWebSocket
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/IXWebSocket")

# cryptopp
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/cryptopp-cmake")

# capstone
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Capstone")

# oxorany
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/oxorany")

# kiero hook
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/kiero")

# imgui
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Imgui")


find_package(lz4 CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        # libhat -- https://github.com/BasedInc/libhat
        libhat

        # minhook
        minhook
        # Curl For People
        cpr::cpr

        # Luau
        Luau.Compiler
        Luau.Ast
        Luau.Analysis
        Luau.VM
        Luau.VM.Internals
        Luau.EqSat
        Luau.CodeGen

        ixwebsocket

        lz4::lz4

        cryptopp::cryptopp

        capstone

        nlohmann_json::nlohmann_json

        oxorany

        # kiero
        kiero

        # Imgui
        Imgui

        Dbghelp.lib
        libcmt.lib
        libcpmt.lib
)

