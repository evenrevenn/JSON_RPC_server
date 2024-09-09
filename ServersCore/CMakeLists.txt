add_library(ServersCore STATIC
    server.cpp
    servers_wrapper.cpp
)
target_include_directories(ServersCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(ServersCore PRIVATE ${CMAKE_SOURCE_DIR}/tests)