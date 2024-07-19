mkdir build
cd build

echo "Building..."

cmake ..
cmake --build .

echo "Installing..."
sudo cmake --install .

cd ..
echo "Don't forget to source params.sh"
