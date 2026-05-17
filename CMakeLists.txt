cmake_minimum_required(VERSION 3.20)
project(Hopf LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /MP /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

option(HOPF_BUILD_TESTS "Build unit tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/cmake")
include(Dependencies)

set(HOPF_SOURCES
    src/main.cpp

    src/core/Application.cpp
    src/core/Logger.cpp

    src/geometry/Primitives.cpp
    src/geometry/Primitives2D.cpp
    src/geometry/Primitives3D.cpp
    src/geometry/Primitives4D.cpp

    src/scene/Entity.cpp
    src/scene/Scene.cpp
    src/scene/SceneSerializer.cpp

    src/physics/Physics.cpp

    src/audio/AudioEngine.cpp

    src/ui/UIAnchor.cpp
    src/ui/UIRenderer.cpp

    src/scripting/Script.cpp
    src/scripting/ScriptRegistry.cpp

    src/examples/Minesweeper4D.cpp

    src/render/Shader.cpp
    src/render/Slicer.cpp
    src/render/Projector.cpp
    src/render/ViewportRenderer.cpp

    src/editor/Editor.cpp
    src/editor/EditorContext.cpp
    src/editor/panels/ViewportPanel.cpp
    src/editor/panels/GameViewPanel.cpp
    src/editor/panels/HierarchyPanel.cpp
    src/editor/panels/InspectorPanel.cpp
    src/editor/panels/ConsolePanel.cpp
    src/editor/panels/FilesPanel.cpp

    src/platform/Window.cpp
)

file(GLOB_RECURSE HOPF_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h")

add_executable(Hopf ${HOPF_SOURCES} ${HOPF_HEADERS})

target_include_directories(Hopf PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(Hopf PRIVATE
    glfw
    glad
    imgui
    miniaudio
    opengl32
)

source_group(TREE "${CMAKE_SOURCE_DIR}/include"     PREFIX "include" FILES ${HOPF_HEADERS})
source_group(TREE "${CMAKE_SOURCE_DIR}/src"          PREFIX "src"     FILES ${HOPF_SOURCES})

set_target_properties(Hopf PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY      "${CMAKE_BINARY_DIR}/bin"
)

if(HOPF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
