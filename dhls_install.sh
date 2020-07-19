#!/bin/sh
# This is the installation script for Dynamatic

DHLS=dynamatic

echo "============================================="
echo "         Install Dynamatic..."
echo "============================================="

# Install required packages
sudo apt-get update
sudo apt install graphviz clang cmake graphviz-dev pkg-config coinor-cbc g++ llvm libxtst6 xdg-utils desktop-file-utils default-jdk git make autoconf g++ flex bison  
echo "deb https://dl.bintray.com/sbt/debian /" | sudo tee -a /etc/apt/sources.list.d/sbt.list
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 642AC823
sudo apt-get update
sudo apt-get install sbt

# Download LLVM for Dynamatic 
git clone http://llvm.org/git/llvm.git --branch release_60 --depth 1
cd llvm/tools
git clone http://llvm.org/git/clang.git --branch release_60 --depth 1
git clone http://llvm.org/git/polly.git --branch release_60 --depth 1
cd ..
mkdir _build
cd _build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_INSTALL_UTILS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../../llvm-6.0
make -j4
make install

# install elastic pass
cd ../../$DHLS/elastic-circuits
mkdir _build && cd _build
cmake .. -DLLVM_ROOT=../../../llvm-6.0
make -j4

# install buffering tool
cd ../../Buffers
mkdir bin
make

# install the VHDL code generator
cd ../dot2vhdl
mkdir bin
make

cd ../..

echo "============= Installation Success ================"


