cmake_minimum_required(VERSION 3.14)
project(MatrixProject CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Скачиваем Google Test автоматически
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Для Windows / MSVC (совместимость рантайма)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. Включаем поддержку тестов CTest
enable_testing()

# 3. Объявляем тестовый исполняемый файл
add_executable(test_matrix test/test_matrix.cpp)

# Подключаем папку с вашими заголовками (если матрица лежит в include/)
target_include_directories(test_matrix PRIVATE include)

# Линкуем с Google Test (gtest_main сам предоставляет функцию main())
target_link_libraries(test_matrix PRIVATE GTest::gtest_main)

# Интеграция с CTest
include(GoogleTest)
gtest_discover_tests(test_matrix)