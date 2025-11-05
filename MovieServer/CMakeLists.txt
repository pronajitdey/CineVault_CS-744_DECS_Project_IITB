cmake_minimum_required(VERSION 3.10)
project(MovieHTTPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(/opt/mysql-connector-c++-9.5.0-linux-glibc2.28-x86-64bit/include)
link_directories(/opt/mysql-connector-c++-9.5.0-linux-glibc2.28-x86-64bit/lib64)

add_executable(MovieHTTPServer main.cpp db.cpp cache.cpp)

target_link_libraries(MovieHTTPServer PRIVATE mysqlcppconn)