qt_add_library(Server STATIC
    server.cpp
)
target_link_libraries(Server PRIVATE Qt6::Core)
target_include_directories(Server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})