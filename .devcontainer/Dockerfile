#-------------------------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See https://go.microsoft.com/fwlink/?linkid=2090316 for license information.
#-------------------------------------------------------------------------------------------------------------

# To fully customize the contents of this image, use the following Dockerfile as a base and add the RUN statement from this file:
# https://github.com/microsoft/vscode-dev-containers/tree/main/containers/debian/.devcontainer
ARG VARIANT=bullseye
FROM mcr.microsoft.com/vscode/devcontainers/base:${VARIANT}
ARG VARIANT
# This Dockerfile's base image has a non-root user with sudo access. Use the "remoteUser"
# property in devcontainer.json to use it. On Linux, the container user's GID/UIDs
# will be updated to match your local UID/GID (when using the dockerFile property).
# See https://aka.ms/vscode-remote/containers/non-root-user for details.
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

ARG LLVM_VERSION=14
ARG LLVM_GPG_FINGERPRINT=6084F3CF814B57C1CF12EFD515CF4D18AF4F7421

# [Optional] Install CMake version different from what base image has already installed. 
# CMake reinstall choices: none, 3.21.5, 3.22.2, or versions from https://cmake.org/download/
ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Configure apt and install packages
RUN apt-get update \
    #
    # Install C++ tools
    && apt-get -y install \
        build-essential \
        cmake \
        git \
        git-lfs \
        ninja-build \
        ccache \
        zsh \
    #
    # [Optional] Update UID/GID if needed
    && if [ "$USER_GID" != "1000" ] || [ "$USER_UID" != "1000" ]; then \
        groupmod --gid $USER_GID $USERNAME \
        && usermod --uid $USER_UID --gid $USER_GID $USERNAME \
        && chown -R $USER_UID:$USER_GID /home/$USERNAME; \
    fi
    
COPY ./reinstall-cmake.sh /tmp/
RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

RUN apt-get update \
    && wget -O- https://apt.llvm.org/llvm-snapshot.gpg.key| apt-key add - \
    && echo "deb http://apt.llvm.org/${VARIANT}/ llvm-toolchain-${VARIANT}-${LLVM_VERSION} main" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get -y install --no-install-recommends \
        llvm-${LLVM_VERSION} \
        clang-${LLVM_VERSION} \
        lldb-${LLVM_VERSION} \
        libc++-${LLVM_VERSION}-dev \
        libc++abi-${LLVM_VERSION}-dev \
        clang-tidy-${LLVM_VERSION} \
        clangd-${LLVM_VERSION} \
    && ln -s /usr/bin/clang-tidy-${LLVM_VERSION} /usr/bin/clang-tidy \
    && ln -s /usr/bin/lldb-${LLVM_VERSION} /usr/bin/lldb \
    && ln -s /lib/llvm-${LLVM_VERSION}/bin/clangd /usr/bin/clangd \
    && ln -sf /usr/bin/lldb-server-${LLVM_VERSION} /usr/lib/llvm-${LLVM_VERSION}/bin/lldb-server-${LLVM_VERSION} \
    # Fixes clangd
    && ln -sf /usr/lib/llvm-${LLVM_VERSION}/include/c++/v1 /usr/include/c++/v1

RUN apt-get autoremove -y \
    && apt-get clean -y

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog

ENV CC="/usr/bin/clang-${LLVM_VERSION}" \
    CXX="/usr/bin/clang++-${LLVM_VERSION}" \
    COV="/usr/bin/llvm-cov-${LLVM_VERSION}" \
    LLDB="/usr/bin/lldb-${LLVM_VERSION}"
