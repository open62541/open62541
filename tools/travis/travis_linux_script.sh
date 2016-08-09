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
          make -j 8
        cd .. && rm build -rf

        mkdir -p build
        cd build
        scan-build cmake -G "Unix Makefiles" -DUA_ENABLE_AMALGAMATION=ON ..
        scan-build -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j 8
        cd .. && rm build -rf
    else
        cppcheck --template "{file}({line}): {severity} ({id}): {message}" \
            --enable=style --force --std=c++11 -j 8 \
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
    make selfsigned
    cp -r doc ../../
    cp ./examples/server_cert.der ../../
    cd .. && rm build -rf

    # cross compilation only with gcc
    if [ "$CC" = "gcc" ]; then
        echo "Cross compile release build for MinGW 32 bit"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j8
        zip -r open62541-win32.zip ../../doc ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win32.zip ..
        cd .. && rm build -rf

        echo "Cross compile release build for MinGW 64 bit"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j8
        zip -r open62541-win64.zip ../../doc ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win64.zip ..
        cd .. && rm build -rf

        echo "Cross compile release build for 32-bit linux"
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-gcc-m32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j8
        tar -pczf open62541-linux32.tar.gz ../../doc ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
        cp open62541-linux32.tar.gz ..
        cd .. && rm build -rf
    fi

    echo "Compile release build for 64-bit linux"
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j8
    tar -pczf open62541-linux64.tar.gz ../../doc ../../server_cert.der ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
    cp open62541-linux64.tar.gz ..
    cp open62541.h .. # copy single file-release
    cp open62541.c .. # copy single file-release
    cd .. && rm build -rf

    if [ "$CC" = "gcc" ]; then
        echo "Upgrade to gcc 4.8"
        export CXX="g++-4.8" CC="gcc-4.8"
    fi

    echo "Building the C++ example"
    mkdir -p build && cd build
    cp ../open62541.* .
    gcc-4.8 -std=c99 -c open62541.c
    g++-4.8 ../examples/server.cpp -I./ open62541.o -lrt -o cpp-server
    cd .. && rm build -rf

    echo "Compile multithreaded version"
    mkdir -p build && cd build
    cmake -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j8
    cd .. && rm build -rf

    #this run inclides full examples and methodcalls
    echo "Debug build and unit tests (64 bit)"
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON ..
    make -j8 && make test ARGS="-V"
    echo "Run valgrind to see if the server leaks memory (just starting up and closing..)"
    (valgrind --leak-check=yes --error-exitcode=3 ./examples/server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
    # only run coveralls on main repo, otherwise it fails uploading the files
    echo "-> Current repo: ${TRAVIS_REPO_SLUG}"
    if ([ "$CC" = "gcc-4.8" ] || [ "$CC" = "gcc" ]) && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
        echo "  Building coveralls for ${TRAVIS_REPO_SLUG}"
        coveralls --gcov /usr/bin/gcov-4.8 -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../
    else
        echo "  Skipping coveralls since not gcc and/or ${TRAVIS_REPO_SLUG} is not the main repo"
    fi
    cd .. && rm build -rf
fi
