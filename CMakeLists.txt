cmake_minimum_required(VERSION 3.10)

project(TBBTest)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules/")

find_package(TBB REQUIRED)
message(STATUS "TBB_VERSION: ${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}")
message(STATUS "TBB_INTERFACE_VERSION: ${TBB_INTERFACE_VERSION}")
message(STATUS "TBB_INCLUDE_DIRS: ${TBB_INCLUDE_DIRS}")
message(STATUS "TBB_LIBRARIES: ${TBB_LIBRARIES}")

add_executable(foo foo.cpp)

target_link_libraries(foo TBB::tbb)
target_compile_features(foo PUBLIC cxx_std_17)
