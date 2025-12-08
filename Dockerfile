FROM ubuntu:latest

ARG TOOLCHAIN_URL="https://github.com/DragonMinded/libdragon/releases/download/toolchain-continuous-prerelease/gcc-toolchain-mips64-x86_64.deb"
ARG TINY3D_REPO_URL="https://github.com/HailToDodongo/tiny3d.git"

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libpng-dev \
    wget \
    git \
    xz-utils \
    jq

# Install libdragon toolchain
RUN wget $TOOLCHAIN_URL -O toolchain.deb &&\
    dpkg -i toolchain.deb &&\
    rm toolchain.deb

# Install libdragon
ENV N64_INST=/opt/libdragon
COPY libdragon /libdragon
RUN cd /libdragon &&\
    make install \
    clobber \
    tools \
    tools-install \
    tools-clean \
    clobber &&\
    cd / &&\
    rm -r /libdragon

# Install tiny3d
ENV T3D_INST=/opt/tiny3d
RUN cd /opt &&\
    git clone $TINY3D_REPO_URL &&\
    cd tiny3d &&\
    make &&\
    cd /

# Install Blender
ENV BLENDER_4=/opt/blender/blender
RUN wget https://ftp.nluug.nl/pub/graphics/blender/release/Blender4.3/blender-4.3.2-linux-x64.tar.xz
RUN apt-get install xz-utils -y &&\
    tar -xvf blender-4.3.2-linux-x64.tar.xz &&\
    mv blender-4.3.2-linux-x64 /opt/blender &&\
    rm blender-4.3.2-linux-x64.tar.xz

# Install Blender Dependencies
RUN apt-get install -y \
    libx11-6 \
    libxi6 \
    libxrender1 \
    libgl1 \
    libgl1-mesa-dri \
    libglu1-mesa \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxfixes3 \
    libxcursor1 \
    libxinerama1 \
    libxxf86vm1 \
    libxrandr2 \
    libxkbcommon0 \
    python3 \
    && apt-get clean

WORKDIR /spellcraft