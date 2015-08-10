cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif()
add_executable(ssltun main.cpp)
target_link_libraries(ssltun ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
