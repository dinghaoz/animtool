cmake_minimum_required(VERSION 3.7)

project(animtool C CXX)


add_library(imageioutil STATIC imageio/gifdec.c imageio/image_dec.c imageio/image_enc.c imageio/imageio_util.c imageio/jpegdec.c imageio/pngdec.c imageio/tiffdec.c imageio/webpdec.c imageio/wicdec.c imageio/metadata.c imageio/pnmdec.c)
target_include_directories(imageioutil PRIVATE /usr/local/include)

add_library(dropframes STATIC core/dropframes.cpp core/animenc.cpp core/webprun.cpp core/filefmt.cpp core/animrun.cpp core/gifrun.cpp core/imgrun.cpp)
find_library(WEBPMUX_LIB webpmux)
find_library(WEBPDEMUX_LIB webpdemux)

target_link_libraries(dropframes imageioutil ${WEBPMUX_LIB} ${WEBPDEMUX_LIB})
target_include_directories(dropframes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include)
target_compile_features(dropframes PRIVATE cxx_std_17)


add_executable(animtool app/animtool.cpp app/dropframes_cmd.cpp app/info_cmd.cpp)
target_compile_features(animtool PRIVATE cxx_std_17)
target_include_directories(animtool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include)

find_library(WEBP_LIB webp)
target_link_libraries(animtool dropframes imageioutil ${WEBP_LIB} ${WEBPMUX_LIB} ${WEBPDEMUX_LIB})