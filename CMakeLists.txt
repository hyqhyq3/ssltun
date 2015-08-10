find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "-std=c++11")
add_executable(ssltun main.cpp)
target_link_libraries(ssltun boost_system ${OPENSSL_SSL_LIBRARIES})
