#!/bin/sh

#  cmake-build.sh
#  SwiftyAnimTool
#
#  Created by Dinghao Zeng on 2023/3/1.
#  

echo ${TARGET_BUILD_DIR}
echo "EFFECTIVE_PLATFORM_NAME $EFFECTIVE_PLATFORM_NAME."

PLATFORM=OS64
if [[ $EFFECTIVE_PLATFORM_NAME == "-iphonesimulator" ]]
then
  PLATFORM=SIMULATOR64
fi

PATH=/opt/homebrew/bin/:$PATH

echo $PATH

if ! [ -x "$(command -v cmake)" ]; then
  echo 'Error: cmake is not installed. Use `brew install cmake` to install.' >&2
  exit 1
fi

mkdir -p ${TARGET_BUILD_DIR}/cmake
mkdir -p ${TARGET_BUILD_DIR}/cmake-products
cmake -S ${PROJECT_DIR}/.. -B ${TARGET_BUILD_DIR}/cmake -GXcode -DCMAKE_TOOLCHAIN_FILE=${PROJECT_DIR}/../cmake/ios.toolchain.cmake -DPLATFORM=$PLATFORM -DENABLE_BITCODE=FALSE
cmake --build ${TARGET_BUILD_DIR}/cmake --config ${CONFIGURATION}

LIB_PATH_PARTS=("" /_deps/webp-build /_deps/jpeg-build)
for i in "${LIB_PATH_PARTS[@]}"
do
    LIB_PATH=${TARGET_BUILD_DIR}/cmake$i/${CONFIGURATION}$EFFECTIVE_PLATFORM_NAME
	echo "$LIB_PATH"
    for FILE_PATH in "$LIB_PATH"/*
    do
        FILE_NAME="$(basename "$FILE_PATH")"
        rm -f ${TARGET_BUILD_DIR}/cmake-products/$FILE_NAME
        ln -s $FILE_PATH ${TARGET_BUILD_DIR}/cmake-products/$FILE_NAME
    done
done
