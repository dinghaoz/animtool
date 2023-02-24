cmake_minimum_required(VERSION 3.7)

project(animtool C CXX)

set(CMAKE_CXX_STANDARD 17)

include(cmake/CPM.cmake)
CPMAddPackage(NAME webp
        VERSION 1.3.0
        URL https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.3.0.tar.gz
        URL_HASH SHA1=49da8782aeedab8779bb2bc1a3f24bf02807e8ee
        OPTIONS
        "WEBP_BUILD_ANIM_UTILS OFF"
        "WEBP_BUILD_CWEBP OFF"
        "WEBP_BUILD_DWEBP OFF"
        "WEBP_BUILD_GIF2WEBP OFF"
        "WEBP_BUILD_IMG2WEBP OFF"
        "WEBP_BUILD_VWEBP OFF"
        "WEBP_BUILD_WEBPINFO OFF"
        "WEBP_BUILD_WEBPMUX OFF"
        "WEBP_BUILD_EXTRAS OFF"
        )

CPMAddPackage(NAME gif
        VERSION 5.2.1
        URL https://onboardcloud.dl.sourceforge.net/project/giflib/giflib-5.2.1.tar.gz
        URL_HASH SHA1=c3f774dcbdf26afded7788979c8081d33c6426dc
        )

CPMAddPackage(NAME jpeg
        VERSION 1.2.1
        URL https://github.com/csparker247/jpeg-cmake/archive/refs/tags/v1.2.1.tar.gz
        URL_HASH SHA1=b7a593d62702c0bba32cb1878ddf04dc10b672f6
        )


add_library(giflib STATIC
        ${gif_SOURCE_DIR}/dgif_lib.c
        ${gif_SOURCE_DIR}/egif_lib.c
        ${gif_SOURCE_DIR}/gifalloc.c
        ${gif_SOURCE_DIR}/gif_err.c
        ${gif_SOURCE_DIR}/gif_hash.c
        ${gif_SOURCE_DIR}/openbsd-reallocarray.c
        ${gif_SOURCE_DIR}/quantize.c
        )


add_definitions(-DWEBP_HAVE_GIF)
add_definitions(-DWEBP_HAVE_JPEG)

set(IMAGE_CODEC_LIBRARIES)
set(IMAGE_CODEC_INCLUDE_DIRS)

find_package(PNG)

if(PNG_FOUND)
    add_definitions(-DWEBP_HAVE_PNG)
    list(APPEND IMAGE_CODEC_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
    list(APPEND IMAGE_CODEC_LIBRARIES ${PNG_LIBRARIES})
endif()


#find_package(JPEG)
#
#if(JPEG_FOUND)
#    add_definitions(-DWEBP_HAVE_JPEG)
#    list(APPEND IMAGE_CODEC_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
#    list(APPEND IMAGE_CODEC_LIBRARIES ${JPEG_LIBRARIES})
#endif()

add_library(webp_imageio STATIC
        ${webp_SOURCE_DIR}/examples/gifdec.c
        ${webp_SOURCE_DIR}/imageio/image_dec.c
        ${webp_SOURCE_DIR}/imageio/image_enc.c
        ${webp_SOURCE_DIR}/imageio/imageio_util.c
        ${webp_SOURCE_DIR}/imageio/jpegdec.c
        ${webp_SOURCE_DIR}/imageio/pngdec.c
        ${webp_SOURCE_DIR}/imageio/tiffdec.c
        ${webp_SOURCE_DIR}/imageio/webpdec.c
        ${webp_SOURCE_DIR}/imageio/wicdec.c
        ${webp_SOURCE_DIR}/imageio/metadata.c
        ${webp_SOURCE_DIR}/imageio/pnmdec.c
        )
target_include_directories(webp_imageio PRIVATE ${webp_SOURCE_DIR}/src ${jpeg_SOURCE_DIR}/libjpeg ${gif_SOURCE_DIR} ${IMAGE_CODEC_INCLUDE_DIRS})


add_library(animtoolcore STATIC
        core/dropframes.cpp
        core/animate.cpp
        core/animenc.cpp
        core/webprun.cpp
        core/filefmt.cpp
        core/animrun.cpp
        core/gifrun.cpp
        core/imgrun.cpp
        core/quantizer.cpp)
target_include_directories(animtoolcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${webp_SOURCE_DIR}/src ${gif_SOURCE_DIR})
target_link_libraries(animtoolcore webp_imageio webp libwebpmux webpdemux giflib jpeg ${IMAGE_CODEC_LIBRARIES})

add_executable(animtool app/animtool.cpp app/dropframes_cmd.cpp app/info_cmd.cpp app/animate_cmd.cpp)
target_include_directories(animtool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${webp_SOURCE_DIR}/src ${gif_SOURCE_DIR})

target_link_libraries(animtool animtoolcore)