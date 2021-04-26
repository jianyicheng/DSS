FROM ubuntu:20.04
ARG GID
ARG UID
RUN echo "Group ID: $GID"
RUN echo "User ID: $UID"

USER root
RUN dpkg --add-architecture i386
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata --assume-yes

# Install Dynamatic/DASS Essentials
RUN apt-get install graphviz clang cmake graphviz-dev --assume-yes
RUN apt-get install pkg-config coinor-cbc g++ llvm --assume-yes
RUN apt-get install libxtst6 xdg-utils desktop-file-utils libboost-all-dev --assume-yes
RUN apt-get install gcc libncurses5 lld default-jdk doxygen --assume-yes
RUN apt-get install software-properties-common --assume-yes

# Install Python
RUN apt-get install python3 python3-pip --assume-yes

# Install Clang
RUN apt-get install clang-tools libclang-dev ninja-build --assume-yes

# Install Others
RUN apt-get install build-essential libssl-dev git vim tmux --assume-yes
RUN apt-get install wget sudo --assume-yes

# Install Modelsim Essentials
RUN apt-get install libxft2 libxft2:i386 lib32ncurses6 libncurses5:i386 --assume-yes
RUN apt-get install libbz2-1.0:i386 libxtst6:i386 --assume-yes

# Append any packages you need here
# RUN apt-get ...

CMD ["bash"]

# Add dev-user
RUN groupadd -g $GID dev-user
RUN useradd -r -g $GID -u $UID -m -d /home/dev-user -s /sbin/nologin -c "User" dev-user
RUN echo "dev-user     ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER dev-user

RUN echo 'DHLS_INSTALL_DIR=/workspace/dhls' >> /home/dev-user/.bashrc
RUN echo 'PATH=$PATH:/workspace/llvm/build/bin:/workspace/dhls/Buffer/bin:/workspace/dhls/dot2vhdl/bin:/workspace/dhls/Regression_test/hls_verifier/HlsVerifier/build:/workspace/dhls/bin:/workspace/prism/prism/bin' >> /home/dev-user/.bashrc

WORKDIR workspace
