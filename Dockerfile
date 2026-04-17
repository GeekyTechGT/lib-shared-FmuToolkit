# Dockerfile — lib-fmu-toolkit builder image
#
# Build the image once:
#   docker build -t lib-fmu-toolkit-builder .
#
# Then use project_manager.py → preset docker-mingw-debug/release,
# or run cmake directly:
#   docker run --rm -v "D:/GeekyTechRepos/lib-fmu-toolkit":/workspace \
#       -w /workspace lib-fmu-toolkit-builder \
#       cmake --preset docker-mingw-debug

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    gcc \
    ninja-build \
    cmake \
    pkg-config \
    libxml2-dev \
    libzip-dev \
    python3 \
    python3-venv \
    python3-pip \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Default command: launch an interactive shell
CMD ["/bin/bash"]
