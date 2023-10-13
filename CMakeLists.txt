cmake_minimum_required(VERSION 3.7)

project(animtool C CXX)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_ANIMTOOL_EXECUTABLE "Build animtool executable" ON)

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
        "BUILD_SHARED_LIBS OFF"
        "WEBP_LINK_STATIC ON"
        )

CPMAddPackage(NAME gif
        VERSION 5.2.1
        URL https://onboardcloud.dl.sourceforge.net/project/giflib/giflib-5.2.1.tar.gz
        URL_HASH SHA1=c3f774dcbdf26afded7788979c8081d33c6426dc
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


CPMAddPackage(
        NAME jpegsrc
        VERSION 9e
        URL "https://www.ijg.org/files/jpegsrc.v9e.tar.gz"
        URL_HASH SHA1=ed959b5f3551bc965fe6d0aff6ca48a26f8ce346
        DOWNLOAD_ONLY YES
)

## Use configure_file instead of file(COPY)
## to ensure files are updated in src directory
configure_file(
        dep/jpeg/CMakeLists.txt
        ${jpegsrc_SOURCE_DIR}/CMakeLists.txt
        COPYONLY
)
configure_file(
        dep/jpeg/ConfigureJConfig.cmake
        ${jpegsrc_SOURCE_DIR}/ConfigureJConfig.cmake
        COPYONLY
)
configure_file(
        dep/jpeg/jconfig.h.in
        ${jpegsrc_SOURCE_DIR}/jconfig.h.in
        COPYONLY
)
configure_file(
        dep/jpeg/libjpeg.pc.cmakein
        ${jpegsrc_SOURCE_DIR}/libjpeg.pc.cmakein
        COPYONLY
)
configure_file(
        dep/jpeg/libjpeg.la.in
        ${jpegsrc_SOURCE_DIR}/libjpeg.la.in
        COPYONLY
)
CPMAddPackage(
        NAME jpeg
        VERSION 9e
        SOURCE_DIR ${jpegsrc_SOURCE_DIR}
        OPTIONS
        "BUILD_EXECUTABLES OFF"
        "BUILD_TESTS OFF"
        "BUILD_ALT_UI OFF"
        "BUILD_SHARED_LIBS OFF"
)

CPMAddPackage(
        NAME png
        VERSION 1.6.39
        URL "https://download.sourceforge.net/libpng/libpng-1.6.39.tar.gz"
        URL_HASH SHA256=af4fb7f260f839919e5958e5ab01a275d4fe436d45442a36ee62f73e5beb75ba
        OPTIONS
        "PNG_EXECUTABLES OFF"
        "PNG_TESTS OFF"
        "PNG_SHARED OFF"
)

configure_file(
        ${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt
        ${png_SOURCE_DIR}/pnglibconf.h
        COPYONLY
)


add_definitions(-DWEBP_HAVE_GIF)
add_definitions(-DWEBP_HAVE_JPEG)
add_definitions(-DWEBP_HAVE_PNG)


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
target_include_directories(webp_imageio PRIVATE ${webp_SOURCE_DIR}/src ${jpeg_SOURCE_DIR} ${gif_SOURCE_DIR} ${png_SOURCE_DIR})
target_link_libraries(webp_imageio jpeg_static giflib png_static)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

add_library(animtoolcore STATIC
        core/dropframes.cpp
        core/animate.cpp
        core/animenc.cpp
        core/webprun.cpp
        core/filefmt.cpp
        core/animrun.cpp
        core/gifrun.cpp
        core/imgrun.cpp
        core/quantizer.cpp core/blurutils.cpp core/blurutils.h core/cg.h core/picutils.cpp core/picutils.h core/opacity.cpp core/opacity.h core/decrun.cpp core/decrun.h core/addlayer.cpp core/addlayer.h core/clrparse.h core/mask.cpp core/mask.h
        core/count.cpp
        core/count.h
        utils/parse.h
        core/cluster.cpp
        core/cluster.h
)

target_include_directories(animtoolcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${webp_SOURCE_DIR}/src ${gif_SOURCE_DIR})
target_link_libraries(animtoolcore webp_imageio webp libwebpmux webpdemux)

if(BUILD_ANIMTOOL_EXECUTABLE)
  add_executable(animtool app/animtool.cpp app/dropframes_cmd.cpp app/info_cmd.cpp app/animate_cmd.cpp app/opacity_cmd.cpp app/opacity_cmd.h app/overlay_cmd.cpp app/overlay_cmd.h app/underlay_cmd.cpp app/underlay_cmd.h app/mask_cmd.cpp app/mask_cmd.h app/output_flags.cpp app/output_flags.h
          app/count_cmd.cpp
          app/count_cmd.h
          app/count_cmd.h
          app/cluster_cmd.cpp
          app/cluster_cmd.h)
  target_include_directories(animtool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${webp_SOURCE_DIR}/src ${gif_SOURCE_DIR})

  target_link_libraries(animtool animtoolcore)
endif()
