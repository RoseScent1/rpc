cmake_minimum_required(VERSION 3.22)
project(tinyrpc)

set(CMAKE_CXX_STANDARD 17)
set(EXEPATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBPATH ${PROJECT_SOURCE_DIR}/lib)
add_compile_options(-std=c++17 -O2 -Wall)
include_directories(
				${PROJECT_SOURCE_DIR}/include/common
				${PROJECT_SOURCE_DIR}/include/net
				${PROJECT_SOURCE_DIR}/include/net/tcp
				${PROJECT_SOURCE_DIR}/include/net/coder
				${PROJECT_SOURCE_DIR}/include/net/rpc

				)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(test_rpc)