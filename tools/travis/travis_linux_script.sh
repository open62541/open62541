#!/bin/bash
set -ev

# Set the build dir to a disctinct value to avoid problems between parallel builds
BUILD_DIR=${TRAVIS_COMMIT}_${TRAVIS_JOB_NUMBER}_build

if [ $ANALYZE = "true" ]; then
    echo "=== Running static code analysis ===" && echo -en 'travis_fold:start:script.analyze\\r'
    if [ "$CC" = "clang" ]; then
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        scan-build-3.9 cmake -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON ..
        scan-build-3.9 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm $BUILD_DIR -rf

        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        scan-build-3.9 cmake -DUA_ENABLE_AMALGAMATION=ON ..
        scan-build-3.9 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm $BUILD_DIR -rf

        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        cmake -DUA_BUILD_EXAMPLES=ON ..
        make -j
        make lint
        cd .. && rm $BUILD_DIR -rf
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
    echo -en 'travis_fold:end:script.analyze\\r'
else
    echo "=== Building ==="

    echo "Documentation and certificate build"  && echo -en 'travis_fold:start:script.build.doc\\r'
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_DOCUMENTATION=ON -DUA_BUILD_SELFSIGNED_CERTIFICATE=ON ..
    make doc
    make doc_pdf
    make selfsigned
    cp -r doc ../../
    cp -r doc_latex ../../
    cp ./examples/server_cert.der ../../
    cd .. && rm $BUILD_DIR -rf
    echo -en 'travis_fold:end:script.build.doc\\r'
    
    echo "Full Namespace 0 Generation"  && echo -en 'travis_fold:start:script.build.ns0\\r'
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_GENERATE_NAMESPACE0=On -DUA_BUILD_EXAMPLES=ON  ..
    make -j
    cd .. && rm $BUILD_DIR -rf
    echo -en 'travis_fold:end:script.build.ns0\\r'
    
    # cross compilation only with gcc
    if [ "$CC" = "gcc" ]; then
        echo "Cross compile release build for MinGW 32 bit"  && echo -en 'travis_fold:start:script.build.cross_mingw32\\r'
        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        zip -r open62541-win32.zip ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win32.zip ..
        cd .. && rm $BUILD_DIR -rf
    	echo -en 'travis_fold:end:script.build.cross_mingw32\\r'

        echo "Cross compile release build for MinGW 64 bit"  && echo -en 'travis_fold:start:script.build.cross_mingw64\\r'
        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        zip -r open62541-win64.zip ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md examples/server.exe examples/client.exe libopen62541.dll.a open62541.h open62541.c
        cp open62541-win64.zip ..
        cd .. && rm $BUILD_DIR -rf
    	echo -en 'travis_fold:end:script.build.cross_mingw64\\r'

        echo "Cross compile release build for 32-bit linux"  && echo -en 'travis_fold:start:script.build.cross_linux\\r'
        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-gcc-m32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        tar -pczf open62541-linux32.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
        cp open62541-linux32.tar.gz ..
        cd .. && rm $BUILD_DIR -rf
    	echo -en 'travis_fold:end:script.build.cross_linux\\r'

        echo "Cross compile release build for RaspberryPi"  && echo -en 'travis_fold:start:script.build.cross_raspi\\r'
        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        git clone https://github.com/raspberrypi/tools
        export PATH=$PATH:./tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-rpi64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        tar -pczf open62541-raspberrypi.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
        cp open62541-raspberrypi.tar.gz ..
        cd .. && rm $BUILD_DIR -rf
    	echo -en 'travis_fold:end:script.build.cross_raspi\\r'
    fi

    echo "Compile release build for 64-bit linux"  && echo -en 'travis_fold:start:script.build.linux_64\\r'
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    tar -pczf open62541-linux64.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md examples/server examples/client libopen62541.a open62541.h open62541.c
    cp open62541-linux64.tar.gz ..
    cp open62541.h ../.. # copy single file-release
    cp open62541.c ../.. # copy single file-release
    cd .. && rm $BUILD_DIR -rf
	echo -en 'travis_fold:end:script.build.linux_64\\r'

    echo "Building the C++ example"  && echo -en 'travis_fold:start:script.build.example\\r'
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    cp ../../open62541.* .
    gcc -std=c99 -c open62541.c
    g++ ../examples/server.cpp -I./ open62541.o -lrt -o cpp-server
    cd .. && rm $BUILD_DIR -rf
	echo -en 'travis_fold:end:script.build.example\\r'

    echo "Compile multithreaded version" && echo -en 'travis_fold:start:script.build.multithread\\r'
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    cmake -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    cd .. && rm $BUILD_DIR -rf
	echo -en 'travis_fold:end:script.build.multithread\\r'

    echo "Debug build and unit tests (64 bit)" && echo -en 'travis_fold:start:script.build.unit_test_valgrind\\r'
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON -DUA_ENABLE_VALGRIND_UNIT_TESTS=ON ..
    make -j && make test ARGS="-V"
    (valgrind --leak-check=yes --error-exitcode=3 ./examples/server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
	echo -en 'travis_fold:end:script.build.unit_test_valgrind\\r'

    # without valgrind
    echo "Debug build and unit tests without valgrind" && echo -en 'travis_fold:start:script.build.unit_test\\r'
    cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON -DUA_ENABLE_VALGRIND_UNIT_TESTS=OFF ..
    make -j && make test ARGS="-V"
    (./examples/server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
	echo -en 'travis_fold:end:script.build.unit_test\\r'

    # only run coveralls on main repo, otherwise it fails uploading the files
    echo "-> Current repo: ${TRAVIS_REPO_SLUG}"
    if [ "$CC" = "gcc" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
        echo "  Building coveralls for ${TRAVIS_REPO_SLUG}" && echo -en 'travis_fold:start:script.build.coveralls\\r'
        coveralls -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../ || true # ignore result since coveralls is unreachable from time to time
        echo -en 'travis_fold:end:script.build.coveralls\\r'
    fi
    cd .. && rm $BUILD_DIR -rf
fi
