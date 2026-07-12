# Image used for CI builds to reduce build time

FROM ubuntu:26.04

RUN <<EOF
    apt update
    ln -fs /usr/share/zoneinfo/America/Los_Angeles /etc/localtime
    DEBIAN_FRONTEND=noninteractive apt install -y tzdata
    dpkg-reconfigure --frontend noninteractive tzdata
    apt install -y \
        sudo lsb-release software-properties-common gnupg wget cmake build-essential curl git \
        python3 python3-pkg-resources libglib2.0-dev ninja-build libnspr4 libnss3 \
        libatk1.0-0 libatk-bridge2.0-0 libcups2 libcairo2 libgtk-3-0 liboss4-salsa-asound2
    curl https://apt.llvm.org/llvm.sh >/tmp/llvm.sh
    chmod +x /tmp/llvm.sh
    /tmp/llvm.sh 23
    sudo ln -s `which python3` /usr/local/bin/python
    sudo ln -s `which clang-23` /usr/local/bin/clang
    sudo ln -s `which clang++-23` /usr/local/bin/clang++
EOF