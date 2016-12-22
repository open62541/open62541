#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    echo "=== Running static code analysis ==="
    if [ "$CC" = "clang" ]; then
        mkdir -p build
        cd build
        scan-build cmake -G "Unix Makefiles" -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON ..
        scan-build -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf

        mkdir -p build
        cd build
        scan-build-3.7 cmake -G "Unix Makefiles" -DUA_ENABLE_AMALGAMATION=ON ..
        scan-build-3.7 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf
    else
        cppcheck --template "{file}({line}): {severity} ({id}): {message}" \
            --enable=style --force --std=c++11 -j 8 \
            --suppress=duplicateBranch \
            --suppress=incorrectStringBooleanError \
            --suppress=invalidscanf --inline-suppr \
            -I include src plugins 2> cppcheck.txt
        if [ -s cppcheck.txt ]; then
            echo "====== CPPCHECK Static Analysis Errors ======"
            cat cppcheck.txt
            exit 1
        fi
    fi
else
    echo "=== Building ==="

    echo "Documentation and certificate build"
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_DOCUMENTATION=ON -DUA_BUILD_SELFSIGNED_CERTIFICATE=ON ..
    make doc
    make doc_pdf
    make selfsigned
    cp -r doc ../../
    cp -r doc_latex ../../
    cp ./examples/server_cert.der ../../
    cd .. && rm build -rf
    
    echo "Full Namespace 0 Generation"
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_GENERATE_NAMESPACE0=On -DUA_BUILD_EXAMPLES=ON  ..
    make -j
    cd .. && rm build -rf
    
    # cross compilation only with gcc
    if [ "$CC" = "gcc" ]; then
        echo "Cross compile release build for MinGW 32 bit"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        zip -r open62541-win32.zip ../../doc ../../doc_latex/open62541.pdf ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win32.zip ..
        cd .. && rm build -rf

        echo "Cross compile release build for MinGW 64 bit"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        zip -r open62541-win64.zip ../../doc ../../doc_latex/open62541.pdf ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win64.zip ..
        cd .. && rm build -rf

        echo "Cross compile release build for 32-bit linux"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-gcc-m32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        tar -pczf open62541-linux32.tar.gz ../../doc ../../doc_latex/open62541.pdf ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
        cp open62541-linux32.tar.gz ..
        cd .. && rm build -rf
    fi

    echo "Compile release build for 64-bit linux"
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    tar -pczf open62541-linux64.tar.gz ../../doc ../../doc_latex/open62541.pdf ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
    cp open62541-linux64.tar.gz ..
    cp open62541.h ../.. # copy single file-release
    cp open62541.c ../.. # copy single file-release
    cd .. && rm build -rf

    echo "Building the C++ example"
    mkdir -p build && cd build
    cp ../../open62541.* .
    gcc -std=c99 -c open62541.c
    g++ ../examples/server.cpp -I./ open62541.o -lrt -o cpp-server
    cd .. && rm build -rf

    echo "Compile multithreaded version"
    mkdir -p build && cd build
    cmake -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    cd .. && rm build -rf

    echo "Compile without discovery version"
    mkdir -p build && cd build
    cmake -DUA_ENABLE_DISCOVERY=OFF -DUA_BUILD_EXAMPLES=ON ..
    make -j
    cd .. && rm build -rf

    echo "Debug build and unit tests (64 bit)"
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON -DUA_ENABLE_VALGRIND_UNIT_TESTS=ON ..
    make -j && make test ARGS="-V"
    (valgrind --leak-check=yes --error-exitcode=3 ./examples/server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
    # without valgrind
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON ..
    make -j && make test ARGS="-V"
    (./examples/server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
    # only run coveralls on main repo, otherwise it fails uploading the files
    echo "-> Current repo: ${TRAVIS_REPO_SLUG}"
    if [ "$CC" = "gcc" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
        echo "  Building coveralls for ${TRAVIS_REPO_SLUG}"
        coveralls -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../ || true # ignore result since coveralls is unreachable from time to time
    else
        echo "  Skipping coveralls since not gcc and/or ${TRAVIS_REPO_SLUG} is not the main repo"
    fi
    cd .. && rm build -rf
fi
