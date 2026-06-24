FROM docker.io/debian:12-slim

ENV GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'
WORKDIR /workdir

RUN apt-get update && apt-get install --no-install-recommends --no-install-suggests --yes \
    'build-essential' \
    'ca-certificates' \
    'clang' \
    'flex' \
    'bison' \
    'g++' \
    'gawk' \
    'gcc-multilib' \
    'gettext' \
    'git' \
    'libncurses5-dev' \
    'libssl-dev' \
    'python3-pip' \
    'python3-venv' \
    'libfdt-dev' \
    'device-tree-compiler' \
    'python3-distutils' \
    'python3-libfdt' \
    'rsync' \
    'unzip' \
    'zlib1g-dev' \
    'file' \
    'wget' \
  && \
  rm -f -r '/var/lib/apt/' && \
  rm -f -r '/var/cache/apt/' && \
  useradd -m -s '/bin/bash' -U 'buildbot'


# Set up Python virtual environment for pyelftools
RUN python3 -m pip install --break-system-packages pyelftools

USER buildbot
