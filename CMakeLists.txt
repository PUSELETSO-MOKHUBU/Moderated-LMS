cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

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

include(CTest)
if(BUILD_TESTING)
    add_executable(lms_tests tests/test_lms.cpp)
    target_link_libraries(lms_tests PRIVATE lms_core)
    add_test(NAME lms_workflows COMMAND lms_tests)
endif()
