
echo "args = $1 $2"
mkdir -p $2
cmake -S $1 -B $2  -GXcode -DCMAKE_TOOLCHAIN_FILE=$1/cmake/ios.toolchain.cmake -DPLATFORM=OS64COMBINED
