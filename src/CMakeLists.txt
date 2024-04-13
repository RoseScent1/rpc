cmake_minimum_required(VERSION 3.22)
project(rpclib)




link_libraries(tinyxml)
link_libraries(protobuf)
set(LIBRARY_OUTPUT_DIRECTORY ${LIBPATH})
aux_source_directory(./net SRC)
aux_source_directory(./common SRC)
aux_source_directory(./net/tcp SRC)
aux_source_directory(./net/coder SRC)
aux_source_directory(./net/rpc SRC)
aux_source_directory(./proto SRC)
link_libraries(tinyxml)
link_libraries(protobuf)
add_library(rpc ${SRC})

