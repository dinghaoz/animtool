cmake_minimum_required(VERSION 3.7)

project(animtool C CXX)

add_executable(animtool
        app/animtool.cpp
        )

target_compile_features(animtool PRIVATE cxx_std_17)
target_include_directories(
        animtool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})