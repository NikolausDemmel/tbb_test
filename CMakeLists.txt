cmake_minimum_required(VERSION 3.10)

project(TBBTest)

find_package(TBB REQUIRED)

add_executable(foo foo.cpp)

target_link_libraries(foo TBB::tbb)
target_compile_features(foo PUBLIC cxx_std_17)
