cmake_minimum_required(VERSION 3.7)

project(animtool C CXX)

add_library(dropframes STATIC core/dropframes.cpp)
find_library(WEBPMUX_LIB webpmux)
target_link_libraries(dropframes ${WEBPMUX_LIB})
target_include_directories(dropframes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include)
target_compile_features(dropframes PRIVATE cxx_std_17)


add_executable(animtool app/animtool.cpp app/dropframes_cmd.cpp)
target_compile_features(animtool PRIVATE cxx_std_17)
target_include_directories(animtool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include)

find_library(WEBP_LIB webp)
target_link_libraries(animtool ${WEBP_LIB} dropframes)