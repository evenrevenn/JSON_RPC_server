cmake_minimum_required(VERSION 3.21)

project(JsonRPC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)


qt_add_executable(JsonRPC
main.cpp
json_rpc_server.cpp
)
set_target_properties(JsonRPC PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(ServersCore)
target_link_libraries(JsonRPC PRIVATE ServersCore)


target_link_libraries(JsonRPC PRIVATE Qt6::Widgets)