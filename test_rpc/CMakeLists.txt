cmake_minimum_required(VERSION 3.22)
project(test_rpc)


project(tinyrpc)


include_directories(
				${PROJECT_SOURCE_DIR}/include/common
				${PROJECT_SOURCE_DIR}/include/net
				${PROJECT_SOURCE_DIR}/include/net/tcp
				${PROJECT_SOURCE_DIR}/include/net/coder
				${PROJECT_SOURCE_DIR}/include/net/rpc
				${PROJECT_SOURCE_DIR}/src/proto
				)

set(EXECUTABLE_OUTPUT_PATH ${EXEPATH})


link_directories($LIBPATH)

link_libraries(rpc)
add_executable(rpc_client ./test_rpc_client.cpp ${SRC})
add_executable(rpc_server ./test_rpc_server.cpp ${SRC})

