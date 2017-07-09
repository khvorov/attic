cmake_minimum_required(VERSION 2.8.9)

project (interpolator)

set(CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++14 -g -O0")

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Boost 1.36.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(interpolator_test interpolator_test.cpp)
target_link_libraries(interpolator_test ${GTEST_BOTH_LIBRARIES} pthread)

add_test(AllTests interpolator_test)
