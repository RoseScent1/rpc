cmake_minimum_required(VERSION 3.22)
project(tinyrpc)

set(CMAKE_CXX_STANDARD 17)
set(EXEPATH ${PROJECT_SOURCE_DIR}/bin)
include_directories(
				${PROJECT_SOURCE_DIR}/include/common
				${PROJECT_SOURCE_DIR}/include/net
				${PROJECT_SOURCE_DIR}/include/net/tcp

				)
# set(LIBPATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)
add_subdirectory(test)