#!/bin/bash
set -ev

echo "Documentation and certificate build"
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_DOCUMENTATION=ON -DUA_BUILD_SELFSIGNED_CERTIFICATE=ON ..
make doc
make selfsigned
cp -r doc ..
cp server_cert.der ..

echo "Testing builds"
cd .. && rm build -rf && mkdir -p build && cd build
echo "Cross compile release build for MinGW 32 bit"
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON -DUA_BUILD_EXAMPLES=ON ..
make
cp ../README.md .
cp ../LICENSE .
cp ../AUTHORS .
cp -r ../doc .
cp ../server_cert.der .
zip -r open62541-win32.zip doc server_cert.der LICENSE AUTHORS README.md server_static.exe server.exe client.exe client_static.exe libopen62541.dll libopen62541.dll.a open62541.h open62541.c
cp open62541-win32.zip ..
cd .. && rm build -rf && mkdir -p build && cd build

echo "Cross compile release build for MinGW 64 bit"
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON -DUA_BUILD_EXAMPLES=ON ..
make
cp ../README.md .
cp ../LICENSE .
cp ../AUTHORS .
cp -r ../doc .
cp ../server_cert.der .
zip -r open62541-win64.zip doc server_cert.der LICENSE AUTHORS README.md server_static.exe server.exe client.exe client_static.exe libopen62541.dll libopen62541.dll.a open62541.h open62541.c
cp open62541-win64.zip ..
cd .. && rm build -rf && mkdir -p build && cd build

echo "Cross compile release build for 32-bit linux"
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-gcc-m32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON ..
make
tar -pczf open62541-linux32.tar.gz ../doc ../server_cert.der ../LICENSE ../AUTHORS ../README.md server_static server client_static client libopen62541.so open62541.h open62541.c
cp open62541-linux32.tar.gz ..
cd .. && rm build -rf && mkdir -p build && cd build

echo "Compile release build for 64-bit linux"
cmake -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON ..
make
tar -pczf open62541-linux64.tar.gz ../doc ../server_cert.der ../LICENSE ../AUTHORS ../README.md server_static server client_static client libopen62541.so open62541.h open62541.c
cp open62541-linux64.tar.gz ..
cp open62541.h .. #copy single file-release
cp open62541.c .. #copy single file-release

echo "Upgrade to gcc 4.8"
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt-get update -qq
sudo apt-get install -qq gcc-4.8 g++-4.8 valgrind
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
sudo update-alternatives --config gcc

echo "Building the C++ example"
gcc -std=c99 -c open62541.c
g++-4.8 ../examples/server.cpp -I./ open62541.o -o cpp-server
cd .. && rm build -rf && mkdir -p build && cd build

echo "Compile multithreaded version"
cmake -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLESERVER=ON ..
make
cd .. && rm build -rf && mkdir -p build && cd build

echo "Debug build and unit tests (64 bit)"
cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_UNIT_TESTS=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_ENABLE_COVERAGE=ON ..
make && make test ARGS="-V"
echo "Run valgrind to see if the server leaks memory (just starting up and closing..)"
if [[ ! ( ${TRAVIS_OS_NAME} == "linux" && ${CC} == "clang") ]]; then
  (valgrind --error-exitcode=3 ./server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
  (coveralls --gcov /usr/bin/gcov-4.8 -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../ ; exit 0);
fi
cd ..
