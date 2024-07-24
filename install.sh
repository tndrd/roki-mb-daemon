#!/bin/bash
set -e

# Installing dependencies
sudo apt-get install git cmake python3

mkdir -p build
cd build

echo "Building..."

cmake ..
cmake --build .

echo "Installing..."
sudo cmake --install .

cd ..
echo "Don't forget to source params.sh"
