cmake_minimum_required(VERSION 3.24)

project(cpparg CXX)

add_library(cpparg INTERFACE cpparg.hpp)
target_include_directories(cpparg INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)
target_compile_features(cpparg INTERFACE cxx_std_23)

add_executable(cpparg_example cpparg_example.cpp)
target_link_libraries(cpparg_example cpparg)
target_compile_features(cpparg_example PRIVATE cxx_std_23)

if(BUILD_TESTING)
  include(CTest)

  add_executable(test_cpparg test/test_main.cpp test/test_cpparg.cpp)
  target_link_libraries(test_cpparg PRIVATE cpparg)
  target_compile_features(test_cpparg PRIVATE cxx_std_23)

  add_test(test_cpparg test_cpparg)
endif()
