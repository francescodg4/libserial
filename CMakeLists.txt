cmake_minimum_required(VERSION 3.5)

project(serial)

if(APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif()

# Include headers
include_directories(include)

if(APPLE)
    # If OSX
    list(APPEND serial_SOURCES src/impl/unix.cc)
    list(APPEND serial_SOURCES src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
    list(APPEND serial_SOURCES src/serial_linux.cpp)
else()
    list(APPEND serial_SOURCES src/serial_windows.cpp)
endif()

# Add serial library
add_library(${PROJECT_NAME} ${serial_SOURCES})

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
