cmake_minimum_required(VERSION 3.26)
project(oxorany)

set(CMAKE_CXX_STANDARD 23)

add_library(oxorany STATIC oxorany.cpp oxorany.h)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)

if (build_type STREQUAL release)
    add_compile_options(
            /MT
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
            /VERBOSE
    )
endif ()

if (build_type STREQUAL release)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY MultiThreaded
    )
endif ()

target_include_directories(oxorany PUBLIC "include/")
