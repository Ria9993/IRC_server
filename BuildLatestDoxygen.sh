sudo apt-get update
sudo apt-get install clang++
sudo apt-get install python
sudo apt-get install cmake
sudo apt-get install flex
sudo apt-get install bison
sudo apt-get install graphviz

git clone https://github.com/doxygen/doxygen.git doxygen
cd doxygen
mkdir ./doxygen/build
cd build
cmake -G "Unix Makefiles" ..
make -j$(nproc)

# Move the executable to the root directory
cd ..
cd ..
mv ./doxygen/build/bin/doxygen ./doxygen

# Success message to color
echo -e "\e[32mDoxygen build successful\e[0m"