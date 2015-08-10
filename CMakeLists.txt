find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "-std=c++11")
add_executable(ssl main.cpp)
target_link_libraries(ssl boost_system ${OPENSSL_SSL_LIBRARIES})
