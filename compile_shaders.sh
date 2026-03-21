#!/bin/bash

SHADER_DIR_PATH="/mnt/0514e0b2-284f-4b10-ab29-eb34a44ce20a/Vulkan/Engine/shaders/"
SHADER_BIN_PATH="/mnt/0514e0b2-284f-4b10-ab29-eb34a44ce20a/Vulkan/Engine/shaders/bin"

glslc "$SHADER_DIR_PATH/triangle.vert" -o "$SHADER_BIN_PATH/vert.spv"
glslc "$SHADER_DIR_PATH/triangle.frag" -o "$SHADER_BIN_PATH/frag.spv"