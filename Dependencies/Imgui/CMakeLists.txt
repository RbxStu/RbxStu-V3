cmake_minimum_required(VERSION 3.29.6)

project(Imgui LANGUAGES CXX)

set(PROJECT Imgui)
set(CMAKE_CXX_STANDARD 23)

file(GLOB IMGUI_SOURCES
        *.cpp
        *.h
        *.hpp
        backends/*.cpp
        backends/*.h
        backends/*.hpp

        misc/cpp/*.cpp
        misc/cpp/*.h
        misc/cpp/*.hpp

        # misc/debuggers/*.cpp
        # misc/debuggers/*.h
        # misc/debuggers/*.hpp

        # misc/fonts/*.cpp
        # misc/fonts/*.h
        # misc/fonts/*.hpp

        # misc/freetype/*.cpp
        # misc/freetype/*.h
        # misc/freetype/*.hpp

        # misc/single_file/*.cpp
        # misc/single_file/*.h
        # misc/single_file/*.hpp
)

add_library(${PROJECT} STATIC
        ${IMGUI_SOURCES}
)

target_link_libraries(${PROJECT} PUBLIC
        dxgi.lib
        d3d11.lib
)

target_include_directories(${PROJECT} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
