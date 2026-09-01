#!/usr/bin/bash
source ~/vulkan/1.4.341.1/setup-env.sh
mkdir -p build

cmake -S . -B build -G Ninja
cmake --build build/ -j$(nproc)
