#!/bin/bash
set -e


# Sonar code quality
if ! [ -z ${SONAR+x} ]; then
    if ([ "${TRAVIS_REPO_SLUG}" != "open62541/open62541" ] ||
        [ "${TRAVIS_PULL_REQUEST}" != "false" ] ||
        [ "${TRAVIS_BRANCH}" != "master" ]); then
        echo "Skipping Sonarcloud on forks"
        # Skip on forks
        exit 0;
    fi
    git fetch --unshallow
	mkdir -p build && cd build
	build-wrapper-linux-x86-64 --out-dir ../bw-output cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
    -DUA_ENABLE_ENCRYPTION .. \
    && make -j
	cd ..
	sonar-scanner
	exit 0
fi

# Docker build test
if ! [ -z ${DOCKER+x} ]; then
    docker build -t open62541 .
    docker run -d -p 127.0.0.1:80:80 --name open62541 open62541 /bin/sh
    docker ps | grep -q open62541
    # disabled since it randomly fails
    # docker ps | grep -q open62541
    exit 0
fi

# Cpplint checking
if ! [ -z ${LINT+x} ]; then
    mkdir -p build
    cd build
    cmake ..
    make cpplint
    if [ $? -ne 0 ] ; then exit 1 ; fi
    exit 0
fi

# Fuzzer build test
if ! [ -z ${FUZZER+x} ]; then
    # Test the corpus generator and use new corpus for fuzz test
    ./tests/fuzz/generate_corpus.sh
    if [ $? -ne 0 ] ; then exit 1 ; fi

    cd build_fuzz
    make && make run_fuzzer
    if [ $? -ne 0 ] ; then exit 1 ; fi
    exit 0
fi

if [ $ANALYZE = "true" ]; then
    echo "=== Running static code analysis ===" && echo -en 'travis_fold:start:script.analyze\\r'
    if [ "$CC" = "clang" ]; then
        mkdir -p build
        cd build
        scan-build cmake -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_UNIT_TESTS=ON ..
        scan-build -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf

        mkdir -p build
        cd build
        scan-build cmake -DUA_ENABLE_AMALGAMATION=ON ..
        scan-build -enable-checker security.FloatLoopCounter \
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
            -I include src plugins examples 2> cppcheck.txt
        if [ -s cppcheck.txt ]; then
            echo "====== CPPCHECK Static Analysis Errors ======"
            cat cppcheck.txt
            # flush output
            sleep 5
            exit 1
        fi
    fi
    echo -en 'travis_fold:end:script.analyze\\r'
else
    echo -en "\r\n=== Building ===\r\n"

    echo -e "\r\n== Documentation and certificate build =="  && echo -en 'travis_fold:start:script.build.doc\\r'
    mkdir -p build
    cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_SELFSIGNED_CERTIFICATE=ON ..
    make doc
    make doc_pdf
    make selfsigned
    cp -r doc ../../
    cp -r doc_latex ../../
    cp ./examples/server_cert.der ../../
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.doc\\r'

    echo -e "\r\n== Full Namespace 0 Generation ==" && echo -en 'travis_fold:start:script.build.ns0\\r'
    mkdir -p build
    cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_FULL_NS0=ON -DUA_BUILD_EXAMPLES=ON  \
    -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.ns0\\r'
    # cross compilation only with gcc
    if [ "$CC" = "gcc" ]; then
        echo -e "\r\n== Cross compile release build for MinGW 32 bit =="  && echo -en 'travis_fold:start:script.build.cross_mingw32\\r'
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        zip -r open62541-win32.zip ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md ./bin/examples/server_ctt.exe ./bin/examples/client.exe ./bin/libopen62541.dll.a open62541.h open62541.c
        cp open62541-win32.zip ..
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.cross_mingw32\\r'

        echo -e "\r\n== Cross compile release build for MinGW 64 bit =="  && echo -en 'travis_fold:start:script.build.cross_mingw64\\r'
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        zip -r open62541-win64.zip ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md ./bin/examples/server_ctt.exe ./bin/examples/client.exe ./bin/libopen62541.dll.a open62541.h open62541.c
        cp open62541-win64.zip ..
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.cross_mingw64\\r'

        echo -e "\r\n== Cross compile release build for 32-bit linux =="  && echo -en 'travis_fold:start:script.build.cross_linux\\r'
        mkdir -p build && cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-gcc-m32.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        tar -pczf open62541-linux32.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md ./bin/examples/server_ctt ./bin/examples/client ./bin/libopen62541.a open62541.h open62541.c
        cp open62541-linux32.tar.gz ..
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.cross_linux\\r'

        echo -e "\r\n== Cross compile release build for RaspberryPi =="  && echo -en 'travis_fold:start:script.build.cross_raspi\\r'
        mkdir -p build && cd build
        git clone https://github.com/raspberrypi/tools
        export PATH=$PATH:./tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/
        cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-rpi64.cmake -DUA_ENABLE_AMALGAMATION=ON -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        tar -pczf open62541-raspberrypi.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md ./bin/examples/server_ctt ./bin/examples/client ./bin/libopen62541.a open62541.h open62541.c
        cp open62541-raspberrypi.tar.gz ..
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.cross_raspi\\r'
    fi

    echo -e "\r\n== Compile release build for 64-bit linux =="  && echo -en 'travis_fold:start:script.build.linux_64\\r'
    mkdir -p build && cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    tar -pczf open62541-linux64.tar.gz ../../doc_latex/open62541.pdf ../LICENSE ../AUTHORS ../README.md ./bin/examples/server_ctt ./bin/examples/client ./bin/libopen62541.a open62541.h open62541.c
    cp open62541-linux64.tar.gz ..
    cp open62541.h ../.. # copy single file-release
    cp open62541.c ../.. # copy single file-release
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.linux_64\\r'

    echo -e "\r\n== Building the C++ example =="  && echo -en 'travis_fold:start:script.build.example\\r'
    mkdir -p build && cd build
    cp ../../open62541.* .
    gcc -std=c99 -c open62541.c
    g++ ../examples/server.cpp -I./ open62541.o -lrt -o cpp-server
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.example\\r'

    echo "Compile as shared lib version" && echo -en 'travis_fold:start:script.build.shared_libs\\r'
    mkdir -p build && cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DBUILD_SHARED_LIBS=ON -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.shared_libs\\r'

    if [ "$CC" != "tcc" ]; then
        echo -e "\r\n==Compile multithreaded version==" && echo -en 'travis_fold:start:script.build.multithread\\r'
        mkdir -p build && cd build
        cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.multithread\\r'
    fi

    echo -e "\r\n== Compile with encryption ==" && echo -en 'travis_fold:start:script.build.encryption\\r'
    mkdir -p build && cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_ENCRYPTION=ON -DUA_BUILD_EXAMPLES=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.encryption\\r'

    echo -e "\r\n== Compile without discovery version ==" && echo -en 'travis_fold:start:script.build.discovery\\r'
    mkdir -p build && cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_DISCOVERY=OFF -DUA_ENABLE_DISCOVERY_MULTICAST=OFF -DUA_BUILD_EXAMPLES=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.discovery\\r'

    echo -e "\r\n== Compile discovery without multicast version ==" && echo -en 'travis_fold:start:script.build.multicast\\r'
    mkdir -p build && cd build
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=OFF -DUA_BUILD_EXAMPLES=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.multicast\\r'

    if [ "$CC" != "tcc" ]; then
        echo -e "\r\n== Compile multithreaded version with discovery ==" && echo -en 'travis_fold:start:script.build.multithread_discovery\\r'
        mkdir -p build && cd build
        cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_MULTITHREADING=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_BUILD_EXAMPLES=ON ..
        make -j
        if [ $? -ne 0 ] ; then exit 1 ; fi
        cd .. && rm build -rf
        echo -en 'travis_fold:end:script.build.multithread_discovery\\r'
    fi

    echo -e "\r\n== Unit tests (full NS0) ==" && echo -en 'travis_fold:start:script.build.unit_test_ns0_full\\r'
    mkdir -p build && cd build
    # Valgrind cannot handle the full NS0 because the generated file is too big. Thus run NS0 full without valgrind
    cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON -DUA_ENABLE_FULL_NS0=ON \
    -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_ENCRYPTION=ON -DUA_ENABLE_DISCOVERY=ON \
    -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=OFF \
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON ..
    make -j && make test ARGS="-V"
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.unit_test_ns0_full\\r'

    if [ "$CC" != "tcc" ]; then
        echo -e "\r\n== Unit tests (minimal NS0) ==" && echo -en 'travis_fold:start:script.build.unit_test_ns0_minimal\\r'
        mkdir -p build && cd build
        cmake -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
        -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_ENCRYPTION=ON -DUA_ENABLE_DISCOVERY=ON \
        -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_COVERAGE=ON \
        -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON ..
        make -j && make test ARGS="-V"
        if [ $? -ne 0 ] ; then exit 1 ; fi
        echo -en 'travis_fold:end:script.build.unit_test_ns0_minimal\\r'

        # only run coveralls on main repo, otherwise it fails uploading the files
        echo -e "\r\n== -> Current repo: ${TRAVIS_REPO_SLUG} =="
        if [ "$CC" = "gcc" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
            echo -en "\r\n==   Building coveralls for ${TRAVIS_REPO_SLUG} ==" && echo -en 'travis_fold:start:script.build.coveralls\\r'
            coveralls -E '.*/build/CMakeFiles/.*' -E '.*/examples/.*' -E '.*/tests/.*' -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../ || true # ignore result since coveralls is unreachable from time to time
            echo -en 'travis_fold:end:script.build.coveralls\\r'
        fi
        cd .. && rm build -rf
    fi
fi
