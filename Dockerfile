FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update     && apt-get install -y --no-install-recommends         build-essential         ca-certificates         clang         clang-format         clang-tidy         cmake         curl         git         g++         gcc         ninja-build         pkg-config         unzip         zip     && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
