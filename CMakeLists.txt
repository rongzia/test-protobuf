cmake_minimum_required(VERSION 2.8.4)
project(test_protobuf)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
link_directories(lib)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_executable(myrpc_client myrpc/test_myrpc_client.cpp myrpc/echo.pb.cc myrpc/rpc_meta.pb.cc myrpc/myrpc.h)
add_executable(myrpc_server myrpc/test_myrpc.cpp myrpc/echo.pb.cc myrpc/rpc_meta.pb.cc myrpc/myrpc.h)
target_link_libraries(myrpc_client libprotobuf.a libboost_system.a)
target_link_libraries(myrpc_server libprotobuf.a libboost_system.a)

add_executable(client boost_socket/client.cpp)
add_executable(server boost_socket/server.cpp)
target_link_libraries(client pthread libboost_system.a)
target_link_libraries(server pthread libboost_system.a)