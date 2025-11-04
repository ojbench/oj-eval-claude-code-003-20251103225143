cmake_minimum_required(VERSION 3.10)
project(icpc_management_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimize for speed
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(code main.cpp)

# Recommended flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(code PRIVATE -O2 -pipe -static-libstdc++ -static-libgcc)
endif()
