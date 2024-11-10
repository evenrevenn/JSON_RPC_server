cmake_minimum_required(VERSION 3.21)

project(JsonRPC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


## Qt enabling
find_package(Qt6 REQUIRED COMPONENTS Widgets)
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)


## Files configuration
configure_file(jsonrpc.html.in ${CMAKE_BINARY_DIR}/jsonrpc.html @ONLY)
file (SIZE ${CMAKE_BINARY_DIR}/jsonrpc.html JSON_HTML_SIZE)
configure_file(config.h.in ${CMAKE_SOURCE_DIR}/config.h @ONLY)


## Executable settings
qt_add_executable(JsonRPC
main.cpp
json_rpc_server.cpp
database_obj.cpp
html_server.cpp
)
set_target_properties(JsonRPC PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


## Externals linking
add_subdirectory(ServersCore)
target_link_libraries(JsonRPC PRIVATE ServersCore)

target_link_libraries(JsonRPC PRIVATE Qt6::Widgets)


## Installing procedure
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
install(TARGETS JsonRPC RUNTIME DESTINATION "bin")
install(FILES ${CMAKE_BINARY_DIR}/jsonrpc.html DESTINATION "bin")
install(DIRECTORY "DatabaseRoot" DESTINATION "bin")


## Documentation
set(DOCS_PATH ${CMAKE_SOURCE_DIR}/docs)

get_property(DOXYGEN_DIRS DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY SUBDIRECTORIES)
list(APPEND DOXYGEN_DIRS ${CMAKE_SOURCE_DIR})

configure_file(doxygen.conf.in ${CMAKE_BINARY_DIR}/doxygen.conf @ONLY)