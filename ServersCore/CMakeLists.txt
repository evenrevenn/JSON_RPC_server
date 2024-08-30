add_library(ServersCore STATIC
    server.cpp
    servers_wrapper.cpp
)
target_include_directories(ServersCore INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})