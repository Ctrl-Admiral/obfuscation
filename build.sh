#!/bin/bash

cur_dir=`dirname "$0"`

[[ -d $cur_dir/build ]] && rm -r $cur_dir/build

mkdir build

cmake \
    -S $cur_dir \
    -B $cur_dir/build \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_CXX_FLAGS="-static"

cmake --build $cur_dir/build --target clean
cmake --build $cur_dir/build --target all

binary_name="obfuscated_change_password"
strip -s  $cur_dir/build/$binary_name
upx -9 -v $cur_dir/build/$binary_name