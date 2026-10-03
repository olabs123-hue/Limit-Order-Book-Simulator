cmake_minimum_required(VERSION 3.14)
project(LimitOrderBook CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(order_book STATIC order_book.cpp)
target_include_directories(order_book PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lob_demo main.cpp)
target_link_libraries(lob_demo PRIVATE order_book)

add_executable(lob_benchmark benchmark.cpp)
target_link_libraries(lob_benchmark PRIVATE order_book)

# ---- tests (GoogleTest) ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(order_book_tests tests/order_book_test.cpp)
target_link_libraries(order_book_tests PRIVATE order_book GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(order_book_tests)
