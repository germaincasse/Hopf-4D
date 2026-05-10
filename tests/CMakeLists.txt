add_executable(hopf_tests
    test_math.cpp
    test_geometry.cpp
    test_scene.cpp
)

target_link_libraries(hopf_tests PRIVATE
    Catch2::Catch2WithMain
    glad
)

target_include_directories(hopf_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

set_target_properties(hopf_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Engine sources used by tests (no GL/window dependencies).
target_sources(hopf_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src/geometry/Primitives.cpp"
    "${CMAKE_SOURCE_DIR}/src/geometry/Primitives2D.cpp"
    "${CMAKE_SOURCE_DIR}/src/geometry/Primitives3D.cpp"
    "${CMAKE_SOURCE_DIR}/src/geometry/Primitives4D.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Scene.cpp"
)

include(CTest)
include(Catch)
catch_discover_tests(hopf_tests)
