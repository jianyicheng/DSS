cmake_minimum_required(VERSION 3.0)
#cmake_policy(SET CMP0057 NEW)
set(LLVM_ROOT "" CACHE PATH "Root of LLVM install.")
list(APPEND CMAKE_PREFIX_PATH
     "${LLVM_ROOT}/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DSOURCE_DIR="${PROJECT_SOURCE_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

SET(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(memProfile)
add_subdirectory(loopFinder)
