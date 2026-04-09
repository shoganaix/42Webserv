FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    netcat \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app