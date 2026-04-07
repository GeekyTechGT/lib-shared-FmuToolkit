FROM ubuntu:24.04

LABEL org.opencontainers.image.title="your_library dev container"
LABEL org.opencontainers.image.description="Ubuntu-based development image for the your_library C++23 scaffold"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update     && apt-get install -y --no-install-recommends         build-essential         ca-certificates         clang         clang-format         clang-tidy         cmake         curl         g++         gcc         git         ninja-build         pkg-config         python3         python3-pip         unzip         zip     && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
