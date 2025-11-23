cmake_minimum_required(VERSION 3.10)
project(LoadGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include cpp-httplib header
include_directories(${CMAKE_SOURCE_DIR}/../MovieServer/include)

add_executable(LoadGenerator load_generator.cpp)

# Link pthread for multi-threading
target_link_libraries(LoadGenerator PRIVATE pthread)
