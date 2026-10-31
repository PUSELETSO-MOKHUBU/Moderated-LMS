cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${configuration} "${CMAKE_BINARY_DIR}")
endforeach()

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

add_library(lms_core
    src/Database.cpp
    src/FileStorage.cpp
    src/LatePenaltyStrategy.cpp
    src/PasswordHasher.cpp
    src/models/DomainModels.cpp
)
target_include_directories(lms_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(TARGET SQLite3::SQLite3)
    target_link_libraries(lms_core PUBLIC SQLite3::SQLite3 OpenSSL::Crypto)
else()
    target_link_libraries(lms_core PUBLIC SQLite::SQLite3 OpenSSL::Crypto)
endif()
if(WIN32)
    target_link_libraries(lms_core PUBLIC shell32)
endif()

if(MSVC)
    target_compile_options(lms_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(lms_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ModeratedLMS src/main.cpp)
target_link_libraries(ModeratedLMS PRIVATE lms_core)
if(MSVC)
    target_compile_options(ModeratedLMS PRIVATE /W4 /permissive-)
else()
    target_compile_options(ModeratedLMS PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(lms_tests tests/test_lms.cpp)
    target_link_libraries(lms_tests PRIVATE lms_core Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(lms_tests DISCOVERY_MODE PRE_TEST)
endif()

option(ENABLE_COVERAGE "Instrument the business-logic library for line coverage" OFF)
option(ENABLE_GCC_ANALYZER "Run GCC's static analyzer and fail on other diagnostics" OFF)

if(ENABLE_GCC_ANALYZER)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "ENABLE_GCC_ANALYZER requires GCC.")
    endif()
    target_compile_options(lms_core PRIVATE -fanalyzer -Werror)
    target_compile_options(ModeratedLMS PRIVATE -fanalyzer -Werror)
    target_compile_options(lms_core PRIVATE -Wno-error=analyzer-use-of-uninitialized-value)
    target_compile_options(ModeratedLMS PRIVATE -Wno-error=analyzer-use-of-uninitialized-value)
endif()

if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE requires GCC or Clang.")
    endif()
    if(NOT BUILD_TESTING)
        message(FATAL_ERROR "ENABLE_COVERAGE requires BUILD_TESTING=ON.")
    endif()

    target_compile_options(lms_core PRIVATE --coverage -O0 -g)
    target_compile_options(lms_tests PRIVATE --coverage -O0 -g)
    target_link_options(lms_tests PRIVATE --coverage)
    target_link_options(ModeratedLMS PRIVATE --coverage)
endif()
