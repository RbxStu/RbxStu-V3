cmake_minimum_required(VERSION 3.29.6)
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
            /GA
            /bigobj
            /O2
            /Gr
            /GS
            /cgthreads8
            /arch:AVX2
    )

    add_link_options(
            /LTCG
            /INCREMENTAL
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

        # RbxStu V3/Events
        Events/Connection.hpp
        Events/Event.hpp

        # RbxStu V3/Scheduling
        Scheduling/TaskScheduler.cpp
        Scheduling/TaskSchedulerOrchestrator.cpp

        Scheduling/Job/ResumeYieldedThreadsJob.cpp

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
        Scheduling/Job/ExecuteScriptJob.cpp
        Scheduling/Job/ExecuteScriptJob.hpp
        Roblox/ScriptContext.cpp
        Roblox/ScriptContext.hpp
        Communication/WebsocketServer.cpp
        Communication/WebsocketServer.hpp
        Communication/PacketManager.cpp
        Communication/PacketManager.hpp
        Communication/PacketBase.hpp
        Communication/Packets/ScheduleExecution.hpp
        StuLuau/ExecutionEngine.cpp
        StuLuau/ExecutionEngine.hpp
)

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

        Dbghelp.lib

        oxorany
)

