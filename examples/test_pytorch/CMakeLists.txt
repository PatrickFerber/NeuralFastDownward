cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(custom_ops)

IF (DEFINED ENV{PATH_TORCH})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{PATH_TORCH}")
ENDIF()

find_package(Torch REQUIRED)

add_executable(example-app example-app.cpp)
target_link_libraries(example-app "${TORCH_LIBRARIES}")
set_property(TARGET example-app PROPERTY CXX_STANDARD 14)
