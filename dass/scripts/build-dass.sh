#!/bin/bash

sed -i "/DASS=/c\export DASS=${PWD}" ./bin/env.tcl
. ./bin/env.tcl

# Avoid system crashes
th=$(($(grep -c ^processor /proc/cpuinfo) / 2))
echo "Building DASS using $th threads..."

# Build LLVM
mkdir -p $DASS/llvm/build
cd $DASS/llvm/build
cmake ../llvm -DLLVM_ENABLE_PROJECTS="clang;polly" \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DCMAKE_C_COMPILER=$(which gcc) \
    -DCMAKE_CXX_COMPILER=$(which g++) \
    -DCMAKE_BUILD_TYPE=DEBUG
make -j $th

# Build Dynamatic
$DASS/bin/build-dynamatic
# Build DASS
$DASS/bin/build-dass

# Build buffering tool in Dynamatic
mkdir -p $DASS/dhls/Buffers/bin
cd $DASS/dhls/Buffers
make clean
make -j
# Build dot2vhdl tool in DASS
mkdir -p $DASS/dass/tools/dot2vhdl/bin
cd $DASS/dass/tools/dot2vhdl
make clean
make -j
# Build simulator tool in DASS
cd $DASS/dass/tools/HlsVerifier
make clean
make -j
# Build IP libraries
# cd $DASS/dass/xlibs
# bash install.sh

# Build PRISM
cd $DASS/prism/prism
make -j $th
