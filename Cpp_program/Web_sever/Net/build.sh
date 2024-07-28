#!/bin/sh
set -x
if [ ! -d "./build" ]; then
  mkdir ./build
fi
cd ./build
cmake ..
make -B
make install