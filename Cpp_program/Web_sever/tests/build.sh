#!/bin/sh
set -x
if [ ! -d "./build" ]; then
  mkdir ./build
fi
if [ ! -d "./Logfiles" ]; then
  mkdir ./Logfiles
fi
cd ./build
cmake ..
make -B
make install