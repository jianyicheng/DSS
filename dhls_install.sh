#!/bin/sh
# This is the installation script for Dynamatic

. env.tcl

echo "============================================="
echo "         Install Dynamatic..."
echo "============================================="

# Install required packages
sudo apt-get update
sudo apt install graphviz clang cmake graphviz-dev pkg-config coinor-cbc g++ llvm libxtst6 xdg-utils desktop-file-utils libboost-regex1.65.1 libxml2

# Download LLVM for Dynamatic 
git clone http://llvm.org/git/llvm.git --branch release_60 --depth 1
cd llvm/tools
git clone http://llvm.org/git/clang.git --branch release_60 --depth 1
git clone http://llvm.org/git/polly.git --branch release_60 --depth 1
cd ..
mkdir _build
cd _build
cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DLLVM_INSTALL_UTILS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../../llvm-6.0
make
make install

# install elastic pass
cd $DHLS/elastic-circuits
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

# install chisel lsq
sudo apt-get install default-jdk
echo "deb https://dl.bintray.com/sbt/debian /" | sudo tee -a /etc/apt/sources.list.d/sbt.list
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 642AC823
sudo apt-get update
sudo apt-get install sbt

cd $DHLS/chisel_lsq/lsq
sbt clean compile assembly
echo "java -jar -Xmx7G $DHLS/chisel_lsq/lsq/output/lsq.jar --target-dir . --spec-file \$1" > $DHLS/bin/lsq_generate

# Here you need to manually add $DHLS/bin in your $PATH variable
# e.g. PATH=$PATH:$DHLS/bin

# todo install tb

echo "============= Installation Success ================"
echo "Please add dynamatic in your environment variables if you have not done it"
echo "append the following line in your ~/.bashrc:"
echo "PATH=\$PATH:$DHLS/bin"
