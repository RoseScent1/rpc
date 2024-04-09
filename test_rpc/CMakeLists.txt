cmake_minimum_required(VERSION 3.22)
project(tinyrpc)

set(CMAKE_CXX_STANDARD 17)

link_directories(${LIBPATH})
set(EXECUTABLE_OUTPUT_PATH ${EXEPATH})
link_libraries(rpc)
add_executable(rpc_client ./order.pb.cc ./test_rpc_client.cpp)
add_executable(rpc_server ./order.pb.cc ./test_rpc_server.cpp)