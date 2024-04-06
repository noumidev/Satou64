# Require CMake 3.16
cmake_minimum_required(VERSION 3.16)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Default to debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(Satou64 CXX)

# Set include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external/plog/include)
include_directories(${SDL2_INCLUDE_DIR})

# SDL2
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# Add subdirectories
add_subdirectory(${PROJECT_SOURCE_DIR}/external/plog)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/SDL)

# Set source files
set(SOURCES
    src/main.cpp
    src/hw/mi.cpp
    src/hw/pi.cpp
    src/hw/ri.cpp
    src/hw/vi.cpp
    src/hw/cpu/cop0.cpp
    src/hw/cpu/cpu.cpp
    src/renderer/renderer.cpp
    src/sys/emulator.cpp
    src/sys/memory.cpp
)

# Set header files
set(HEADERS
    include/common/types.hpp
    include/hw/mi.hpp
    include/hw/pi.hpp
    include/hw/ri.hpp
    include/hw/vi.hpp
    include/hw/cpu/cop0.hpp
    include/hw/cpu/cpu.hpp
    include/renderer/renderer.hpp
    include/sys/emulator.hpp
    include/sys/memory.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE plog SDL2::SDL2-static)
