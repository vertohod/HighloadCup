cmake_minimum_required(VERSION 2.8)

project(highloadcup)

find_package(Git REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Ofast -march=sandybridge -flto")

include_directories(${CMAKE_SOURCE_DIR})

file(MAKE_DIRECTORY "libs")

set(SOURCE_EXE 
	include/sp_http_proto.cpp
	include/http_proto.cpp
	include/request.cpp
	include/format.cpp
    include/stream.cpp
	include/types.cpp
	include/util.cpp
	include/log.cpp
	hl_server.cpp
	processor.cpp
    base.cpp
	highloadcup.cpp
)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost COMPONENTS system regex thread date_time filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#message(STATUS "*****")

# RapidJSON =======================================
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/rapidjson")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone https://github.com/miloyip/rapidjson.git
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/libs"
    )
endif()
add_subdirectory(libs/rapidjson)
include_directories(${CMAKE_SOURCE_DIR}/libs/rapidjson/include)
#=================================================

# dense_hash_map =======================================
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/sparsehash")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone https://github.com/sparsehash/sparsehash.git
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/libs"
    )
endif()
#=================================================

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
