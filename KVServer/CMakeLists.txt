cmake_minimum_required(VERSION 3.10)
project(KVHTTPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(/opt/mysql-connector-c++-9.5.0-linux-glibc2.28-x86-64bit/include)
link_directories(/opt/mysql-connector-c++-9.5.0-linux-glibc2.28-x86-64bit/lib64)

add_executable(KVHTTPServer main.cpp db.cpp)

target_link_libraries(KVHTTPServer PRIVATE mysqlcppconn)