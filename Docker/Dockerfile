FROM ubuntu:22.04
ARG UID
ARG GID
ARG VHLS_PATH
RUN echo "Group ID: $GID"
RUN echo "User ID: $UID"

USER root
RUN apt-get update -y && apt-get install apt-utils -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata

# Install basic packages
RUN apt-get upgrade -y
RUN apt-get update -y \
    && apt-get install -y gcc-riscv64-unknown-elf \
                          libgmp-dev python3 python3-pip g++\
                          clang llvm lld clang-tidy clang-format \
                          gcc-multilib gcc sudo wget vim \
                          curl tmux git bc locales libncurses5 \
                          help2man

RUN apt-get update -y \
    && apt-get install -y autoconf automake libtool pkg-config \
                          clang bison cmake mercurial ninja-build \
                          samba flex texinfo time libglib2.0-dev \
                          libpixman-1-dev libarchive-dev libarchive-tools \
                          libbz2-dev libattr1-dev libcap-ng-dev \
                          libexpat1-dev libgmp-dev libelf-dev \
                          coinor-cbc coinor-libcbc-dev
RUN apt-get update -y \
    && apt-get install -y libncurses5 gcc g++ clang llvm \
                          graphviz make libgraphviz-dev pkg-config \
                          parallel default-jre default-jdk scala g++-multilib \
                          z3 build-essential libboost-all-dev libssl-dev \
                          scala 

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

RUN locale-gen en_US.UTF-8

RUN pip3 install --upgrade pip
RUN pip3 install pandas dataclasses colorlog pyyaml
# Boogie
RUN apt-get update -y \
    && apt install -y dotnet-sdk-7.0

# Append any packages you need here
# RUN yum install -y ...
RUN apt-get update -y \
    && apt install -y htop

CMD ["bash"]

# Add dev-user
RUN getent group $GID || groupadd -g $GID dev-user
RUN useradd -r -g $GID -u $UID -m -d /home/dev-user -s /sbin/nologin -c "User" dev-user
RUN echo "dev-user     ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER dev-user

# Boogie
RUN dotnet tool install --global boogie --version 2.10.2

# Add environment variables
ENV vhls $VHLS_PATH
RUN printf "\
\nexport LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:\$LIBRARY_PATH \
\n# Vitis HLS setup \
\nexport VHLS=${VHLS_PATH} \
\nsource ${VHLS_PATH}/Vitis_HLS/2020.2/settings64.sh \
\n# source /workspace/dass/scripts/setup-vitis-hls-llvm.sh \
\n# Basic PATH setup \
\nexport PATH=/dkrPkgs/cmake-3.20.3/bin:/workspace/bin:/workspace/llvm/build/bin:$PATH \
\n# Thread setup \
\nexport nproc=\$(grep -c ^processor /proc/cpuinfo) \
\n# Terminal color... \
\nexport PS1=\"[\\\\\\[\$(tput setaf 3)\\\\\\]\\\t\\\\\\[\$(tput setaf 2)\\\\\\] \\\u\\\\\\[\$(tput sgr0)\\\\\\]@\\\\\\[\$(tput setaf 2)\\\\\\]\\\h \\\\\\[\$(tput setaf 7)\\\\\\]\\\w \\\\\\[\$(tput sgr0)\\\\\\]] \\\\\\[\$(tput setaf 6)\\\\\\]$ \\\\\\[\$(tput sgr0)\\\\\\]\" \
\nexport LS_COLORS='rs=0:di=01;96:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01' \
\nalias ls='ls --color' \
\nalias grep='grep --color'\n" >> /home/dev-user/.bashrc
#Add vim environment
RUN printf "\
\nset autoread \
\nset hlsearch \
\nautocmd BufWritePost *.cpp silent! !clang-format -i <afile> \
\nautocmd BufWritePost *.c   silent! !clang-format -i <afile> \
\nautocmd BufWritePost *.h   silent! !clang-format -i <afile> \
\nautocmd BufWritePost *.hpp silent! !clang-format -i <afile> \
\nautocmd BufWritePost *.cc  silent! !clang-format -i <afile> \
\nautocmd BufWritePost *.py  silent! set tabstop=4 shiftwidth=4 expandtab \
\nautocmd BufWritePost *.py  silent! !python3 -m black <afile> \
\nautocmd BufWritePost *.sv  silent! !verible-verilog-format --inplace <afile> \
\nautocmd BufWritePost *.v  silent! !verible-verilog-format --inplace <afile> \
\nau BufNewFile,BufRead Jenkinsfile setf groovy \
\nautocmd BufWritePost * redraw! \
\ncall plug#begin() \
\nPlug 'mtikekar/vim-bsv' \
\ncall plug#end() \
\n" >> /home/dev-user/.vimrc

USER dev-user

CMD ["bash"]

WORKDIR workspace
