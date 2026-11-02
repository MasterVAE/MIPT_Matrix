cmake_minimum_required(VERSION 3.16)
project(MatrixProject VERSION 1.0 LANGUAGES CXX)

# Checked integer arithmetic uses GCC/Clang overflow builtins.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Use GCC or Clang (on Windows, build in WSL).")
endif()

add_library(matrix INTERFACE)
target_include_directories(matrix INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(matrix INTERFACE cxx_std_20)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(matrix_cli src/main.cpp)
target_link_libraries(matrix_cli PRIVATE matrix)
target_compile_options(matrix_cli PRIVATE -Wall -Wextra)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_matrix test/test_matrix.cpp)
    target_link_libraries(test_matrix PRIVATE matrix)
    target_compile_options(test_matrix PRIVATE -Wall -Wextra)
    add_test(NAME matrix_unit COMMAND test_matrix)

    find_package(Python3 3.10 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME matrix_e2e
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/test_e2e.py
                    --cli $<TARGET_FILE:matrix_cli> --no-color)
    else()
        message(STATUS "Python 3.10+ not found: E2E tests disabled")
    endif()
endif()
