cmake_minimum_required(VERSION 3.17)
CMAKE_POLICY( SET CMP0003 OLD )
project(rb3converter)

set(CMAKE_CXX_STANDARD 14)
add_compile_definitions(DEBUG)

include_directories(.)

add_executable(rb3converter
        ConvertMid.cpp
        ConvertMid.hpp
        ConvertMogg.cpp
        ConvertMogg.hpp
        main.cpp NPD.cpp NPD.hpp)

link_directories(rb3converter /usr/local/lib)
target_LINK_LIBRARIES(rb3converter /usr/local/lib/libgeneral.so pthread)

find_package(OpenSSL REQUIRED)
target_link_libraries(rb3converter OpenSSL::Crypto)