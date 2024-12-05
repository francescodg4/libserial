cmake_minimum_required(VERSION 3.15)

project(serial)

Include(FetchContent)

if(APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif()

# Include headers
include_directories(include)

# Sources
set(serial_SRCS
    include/serial/serial.h
    include/serial/v8stdint.h
)

if(APPLE)
    # If OSX
    list(APPEND serial_SRCS src/impl/unix.cc)
    list(APPEND serial_SRCS src/serial.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
    # If unix
    list(APPEND serial_SRCS src/impl/unix.cc)
    list(APPEND serial_SRCS src/serial.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_linux.cc)
else()
    # If windows
    list(APPEND serial_SRCS src/impl/win.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_win.cc)
endif()

# Add serial library
add_library(${PROJECT_NAME} ${serial_SRCS})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} rt pthread)
else()
    target_link_libraries(${PROJECT_NAME} setupapi)
endif()

# Uncomment for example
add_executable(serial_example examples/serial_example.cc)
add_dependencies(serial_example ${PROJECT_NAME})
target_link_libraries(serial_example ${PROJECT_NAME})

# Test
# ---- catch2 ----
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

FetchContent_MakeAvailable(Catch2)

# ---- FakeIt ----
FetchContent_Declare(
    FakeIt
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.4.1
)

FetchContent_MakeAvailable(FakeIt)

if(WIN32)
    add_executable(${PROJECT_NAME}_test
        test/test_win.cpp
    )
else()
    add_executable(${PROJECT_NAME}_test
        test/test_unix.cpp
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _SETUPAPI_ _ADVAPI32_ _KERNEL32_)

target_compile_definitions(${PROJECT_NAME}_test PRIVATE _SETUPAPI_ _ADVAPI32_ _KERNEL32_)

target_include_directories(${PROJECT_NAME}_test PRIVATE include/)

target_link_libraries(${PROJECT_NAME}_test PRIVATE
    serial
    Catch2::Catch2WithMain
    FakeIt::FakeIt-catch
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_test)
