FROM tgagor/centos:stream8
ARG GID
ARG UID
ARG VHLS_PATH
RUN echo "Group ID: $GID"
RUN echo "User ID: $UID"

USER root

# Basic packages
RUN dnf update -y
RUN dnf install -y gcc-c++ sudo vim openssh-clients \
                   wget curl-devel expat-devel gettext-devel zlib-devel \
                   perl-CPAN perl-devel git lld

# Vitis HLS dependences
RUN dnf install -y https://extras.getpagespeed.com/release-latest.rpm \
    && dnf install -y libXext libXrender libXtst gettext libtool \
                   rpm-sign autoconf automake binutils bison \
                   flex make patch pkgconfig redhat-rpm-config \
                   rpm-build gperftools gperftools-devel ncurses-compat-libs
# DASS HLS dependences
RUN dnf install -y llvm graphviz lld clang python3 gcc-gfortran \
                   glibc-devel python3-pip svn openssl openssl-devel \
                   boost boost-devel llvm-devel java-11-openjdk-devel \
                   scala parallel \
    && dnf --enablerepo=powertools install -y \
                   doxygen graphviz-devel glibc-static libstdc++-static

# Build the following packages from source:
RUN mkdir -p /dkrPkgs
# cmake 3.20
RUN cd /dkrPkgs \
    && wget https://github.com/Kitware/CMake/releases/download/v3.20.3/cmake-3.20.3.tar.gz \
    && tar zxvf cmake-3.20.3.tar.gz \
    && cd cmake-3.20.3 \
    && ./bootstrap --prefix=/usr/local \
    && make -j $(grep -c ^processor /proc/cpuinfo) \
    && make install
# python 3.8
RUN dnf install python3 bzip2-devel libffi-devel xz-devel -y
RUN cd /dkrPkgs \
    && wget https://www.python.org/ftp/python/3.8.3/Python-3.8.3.tgz \
    && tar xvf Python-3.8.3.tgz \
    && cd Python-3.8*/ \
    && ./configure --enable-optimizations --prefix=/usr/local \
    && make install
# coinor-cbc
RUN cd /dkrPkgs  \
    && git clone --branch=stable/2.9 https://github.com/coin-or/Cbc Cbc-2.9  \
    && cd Cbc-2.9  \
    && git clone --branch=stable/0.8 https://github.com/coin-or-tools/BuildTools/  \
    && ./BuildTools/get.dependencies.sh fetch  \
    && ./configure --prefix=/usr/local  \
    && make -j $(grep -c ^processor /proc/cpuinfo)  \
    && make install
# sbt
# remove old Bintray repo file
RUN rm -f /etc/yum.repos.d/bintray-rpm.repo
RUN curl -L https://www.scala-sbt.org/sbt-rpm.repo >sbt-rpm.repo
RUN mv sbt-rpm.repo /etc/yum.repos.d/
RUN dnf install -y sbt
RUN pip3 install tabulate
# z3
RUN wget https://github.com/Z3Prover/z3/archive/refs/tags/z3-4.8.12.tar.gz \
    && tar zxvf z3-4.8.12.tar.gz \
    && cd z3-z3-4.8.12 \
    && python3 scripts/mk_make.py \
    && cd build \
    && make -j \
    && make install
RUN pip3 install --upgrade pip
RUN pip3 install pandas dataclasses colorlog pyyaml
# Boogie
RUN dnf install -y dotnet-sdk-5.0

# Append any packages you need here
# RUN yum install -y ...
RUN yum install -y htop 

# LLVM dependences
RUN rm -f /usr/lib/libtinfo.so; ln -s /usr/lib64/libtinfo.so.5 /usr/lib/libtinfo.so

CMD ["bash"]

# Add dev-user
RUN getent group $GID || groupadd -g $GID dev-user
RUN useradd -r -g $GID -u $UID -m -d /home/dev-user -s /sbin/nologin -c "User" dev-user
RUN echo "dev-user     ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER dev-user

# Boogie
RUN dotnet tool install --global boogie --version 2.10.2

# Add environment variables
# Vitis HLS setup
RUN echo 'export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LIBRARY_PATH' >>/home/dev-user/.bashrc
ENV vhls $VHLS_PATH
RUN echo "source ${vhls}/Vitis_HLS/2020.2/settings64.sh" >>/home/dev-user/.bashrc
RUN echo "source /workspace/dass/scripts/setup-vitis-hls-llvm.sh" >>/home/dev-user/.bashrc
# PATH
RUN echo 'export PATH=/workspace/bin:/workspace/llvm/build/bin:$PATH' >>/home/dev-user/.bashrc
# Thread setup
RUN echo 'export nproc=$(grep -c ^processor /proc/cpuinfo)' >>/home/dev-user/.bashrc

WORKDIR workspace
