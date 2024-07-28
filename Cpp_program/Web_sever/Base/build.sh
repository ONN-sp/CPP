#!/bin/sh
set -x
rm -rf build
if [ ! -d "./build" ]; then
  mkdir ./build
fi
cd ./build
cmake ..
make clean
make -B
make install