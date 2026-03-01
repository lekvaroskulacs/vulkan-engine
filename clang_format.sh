#!/bin/bash

find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} +

echo "Clang-format applied to all .cpp, .h, and .hpp files."