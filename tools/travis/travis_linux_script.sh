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
	build-wrapper-linux-x86-64 --out-dir ../bw-output cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
	    -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_DISCOVERY=ON \
        -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
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
    # First check if the build is successful
    ./tests/fuzz/check_build.sh
    if [ $? -ne 0 ] ; then exit 1 ; fi


    # Test the corpus generator and use new corpus for fuzz test
    ./tests/fuzz/generate_corpus.sh
    if [ $? -ne 0 ] ; then exit 1 ; fi

    cd build_fuzz
    make && make run_fuzzer
    if [ $? -ne 0 ] ; then exit 1 ; fi
    exit 0
fi

# INSTALL build test
if ! [ -z ${INSTALL+x} ]; then
echo "=== Install build, then compile examples ===" && echo -en 'travis_fold:start:script.build.install\\r'
    # Use make install to deploy files and then test if we can build the examples based on the installed files
    mkdir -p build
    cd build
    cmake \
        -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DUA_NAMESPACE_ZERO=FULL \
        -DUA_ENABLE_AMALGAMATION=OFF \
        -DCMAKE_INSTALL_PREFIX=$TRAVIS_BUILD_DIR/open62541_install ..
    make -j install
    if [ $? -ne 0 ] ; then exit 1 ; fi

    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.install\\r'

    echo -en 'travis_fold:start:script.build.mini-example\\r'
    # Now create a simple CMake Project which uses the installed file
    mkdir compile
    cp -r examples compile/examples && cd compile/examples
    cmake \
        -DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/open62541_install \
        -DUA_NAMESPACE_ZERO=FULL .
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd ../..
    rm -rf compile
    echo -en 'travis_fold:end:script.build.mini-example\\r'
    exit 0
fi

if ! [ -z ${CLANG_FORMAT+x} ]; then

    # Only run clang format on Pull requests, not on direct push
    if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then
        echo -en "\\nSkipping clang-format on non-pull request\\n"
        exit 0
    fi

    echo "=== Running clang-format with diff against branch '$TRAVIS_BRANCH' ===" && echo -en 'travis_fold:start:script.clang-format\\r'

    # add clang-format-ci
    curl -Ls "https://raw.githubusercontent.com/llvm-mirror/clang/c510fac5695e904b43d5bf0feee31cc9550f110e/tools/clang-format/git-clang-format" -o "$LOCAL_PKG/bin/git-clang-format"
    chmod +x $LOCAL_PKG/bin/git-clang-format

    # Ignore files in the deps directory diff by resetting them to master
    git diff --name-only $TRAVIS_BRANCH | grep 'deps/*' | xargs git checkout $TRAVIS_BRANCH --

    # clang-format the diff to the target branch of the PR
    difference="$($LOCAL_PKG/bin/git-clang-format --style=file --diff $TRAVIS_BRANCH)"

    if ! case $difference in *"no modified files to format"*) false;; esac; then
        echo "====== clang-format did not find any issues. Well done! ======"
        exit 0
    fi
    if ! case $difference in *"clang-format did not modify any files"*) false;; esac; then
        echo "====== clang-format did not find any issues. Well done! ======"
        exit 0
    fi

    echo "====== clang-format Format Errors ======"

    echo "Uploading the patch to a pastebin page ..."
    pastebinUrl="$($LOCAL_PKG/bin/git-clang-format --style=file --diff $TRAVIS_BRANCH | curl -F 'sprunge=<-' http://sprunge.us)"

    echo "Created a patch file under: $pastebinUrl"

    echo "Please fix the following issues.\n\n"
    echo "You can also use the following command to apply the diff to your source locally:\n-------------\n"
    echo "curl -o format.patch $pastebinUrl && git apply format.patch\n-------------\n"
    echo "=============== DIFFERENCE - START =================\n"
    # We want to get colored diff output into the variable
    git config color.diff always
    $LOCAL_PKG/bin/git-clang-format --binary=clang-format-6.0 --style=file --diff $TRAVIS_BRANCH
    echo "\n============= DIFFERENCE - END ===================="

    echo -en 'travis_fold:start:script.clang-format\\r'
    exit 1
fi

if ! [ -z ${ANALYZE+x} ]; then
    echo "=== Running static code analysis ===" && echo -en 'travis_fold:start:script.analyze\\r'
    if ! case $CC in clang*) false;; esac; then
        mkdir -p build
        cd build
        scan-build-6.0 cmake \
            -DUA_BUILD_EXAMPLES=ON \
            -DUA_BUILD_UNIT_TESTS=ON ..
        scan-build-6.0 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf

        mkdir -p build
        cd build
        scan-build-6.0 cmake \
            -DUA_ENABLE_PUBSUB=ON \
            -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
            -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON ..
        scan-build-6.0 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf

        mkdir -p build
        cd build
        scan-build-6.0 cmake \
            -DUA_ENABLE_AMALGAMATION=OFF ..
        scan-build-6.0 -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j
        cd .. && rm build -rf

        #mkdir -p build && cd build
        #cmake -DUA_ENABLE_STATIC_ANALYZER=REDUCED ..
        ## previous clang-format to reduce to non-trivial warnings
        #make clangformat
        #make
        #cd .. && rm build -rf
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
    exit 0
fi

echo -en "\r\n=== Building ===\r\n"

echo -e "\r\n== Documentation build =="  && echo -en 'travis_fold:start:script.build.doc\\r'
mkdir -p build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON ..
make doc
make doc_pdf
cp -r doc ../../
cp -r doc_latex ../../
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.doc\\r'

# cross compilation only with gcc
if ! [ -z ${MINGW+x} ]; then
    echo -e "\r\n== Cross compile release build for MinGW 32 bit =="  && echo -en 'travis_fold:start:script.build.cross_mingw32\\r'
    mkdir -p build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-win32 \
        -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw32.cmake \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_AMALGAMATION=OFF ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    make install
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd ..
    zip -r open62541-win32.zip ../doc_latex/open62541.pdf LICENSE AUTHORS README.md open62541-win32/*
    rm build -rf
    echo -en 'travis_fold:end:script.build.cross_mingw32\\r'

    echo -e "\r\n== Cross compile release build for MinGW 64 bit =="  && echo -en 'travis_fold:start:script.build.cross_mingw64\\r'
    mkdir -p build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-win64 \
        -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-mingw64.cmake \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_AMALGAMATION=OFF ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    make install
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd ..
    zip -r open62541-win64.zip ../doc_latex/open62541.pdf LICENSE AUTHORS README.md open62541-win64/*
    rm build -rf
    echo -en 'travis_fold:end:script.build.cross_mingw64\\r'

    echo -e "\r\n== Cross compile release build for 32-bit linux =="  && echo -en 'travis_fold:start:script.build.cross_linux\\r'
    mkdir -p build && cd build
    cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-linux32 \
        -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-gcc-m32.cmake \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_AMALGAMATION=OFF ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    make install
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd ..
    tar -pczf open62541-linux32.tar.gz ../doc_latex/open62541.pdf LICENSE AUTHORS README.md open62541-linux32/*
    rm build -rf
    echo -en 'travis_fold:end:script.build.cross_linux\\r'

    echo -e "\r\n== Cross compile release build for RaspberryPi =="  && echo -en 'travis_fold:start:script.build.cross_raspi\\r'
    mkdir -p build && cd build
    git clone https://github.com/raspberrypi/tools
    export PATH=$PATH:${TRAVIS_BUILD_DIR}/build/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/
    cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-raspberrypi \
        -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-rpi64.cmake \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_AMALGAMATION=OFF ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    make install
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd ..
    tar -pczf open62541-raspberrypi.tar.gz ../doc_latex/open62541.pdf LICENSE AUTHORS README.md open62541-raspberrypi/*
    rm build -rf
    echo -en 'travis_fold:end:script.build.cross_raspi\\r'
fi

echo -e "\r\n== Compile release build for 64-bit linux =="  && echo -en 'travis_fold:start:script.build.linux_64\\r'
mkdir -p build && cd build
cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-linux64 \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_AMALGAMATION=OFF ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
make install
if [ $? -ne 0 ] ; then exit 1 ; fi
cd ..
tar -pczf open62541-linux64.tar.gz ../doc_latex/open62541.pdf LICENSE AUTHORS README.md open62541-linux64/*
rm build -rf
echo -en 'travis_fold:end:script.build.linux_64\\r'

echo -e "\r\n== Compile with amalgamation =="  && echo -en 'travis_fold:start:script.build.amalgamate\\r'
mkdir -p build && cd build
cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-linux64 \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=OFF \
    -DUA_ENABLE_AMALGAMATION=ON \
    -DUA_ENABLE_HISTORIZING=ON  ..
make -j
cp open62541.h ../.. # copy single file-release
cp open62541.c ../.. # copy single file-release
gcc -std=c99 -c open62541.c
g++ ../examples/server.cpp -I./ open62541.o -lrt -o cpp-server
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.amalgamate\\r'

echo "Compile as shared lib version" && echo -en 'travis_fold:start:script.build.shared_libs\\r'
mkdir -p build && cd build
cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_AMALGAMATION=OFF ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.shared_libs\\r'

if [ "$CC" != "tcc" ]; then
    echo -e "\r\n==Compile multithreaded version==" && echo -en 'travis_fold:start:script.build.multithread\\r'
    mkdir -p build && cd build
    cmake \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_MULTITHREADING=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.multithread\\r'
fi

echo -e "\r\n== Compile with encryption ==" && echo -en 'travis_fold:start:script.build.encryption\\r'
mkdir -p build && cd build
cmake \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_ENCRYPTION=ON ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.encryption\\r'

echo -e "\r\n== Compile without discovery version ==" && echo -en 'travis_fold:start:script.build.discovery\\r'
mkdir -p build && cd build
cmake \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_DISCOVERY=OFF \
    -DUA_ENABLE_DISCOVERY_MULTICAST=OFF ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.discovery\\r'

if [ "$CC" != "tcc" ]; then
    echo -e "\r\n== Compile multithreaded version with discovery ==" && echo -en 'travis_fold:start:script.build.multithread_discovery\\r'
    mkdir -p build && cd build
    cmake \
        -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_ENABLE_DISCOVERY=ON \
        -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
        -DUA_ENABLE_MULTITHREADING=ON ..
    make -j
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.multithread_discovery\\r'
fi

echo -e "\r\n== Compile JSON encoding ==" && echo -en 'travis_fold:start:script.build.json\\r'
mkdir -p build && cd build
cmake \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_ENABLE_JSON_ENCODING=ON ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.json\\r'

echo -e "\r\n== Unit tests (full NS0) ==" && echo -en 'travis_fold:start:script.build.unit_test_ns0_full\\r'
mkdir -p build && cd build
# Valgrind cannot handle the full NS0 because the generated file is too big. Thus run NS0 full without valgrind
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_BUILD_UNIT_TESTS=ON \
    -DUA_ENABLE_COVERAGE=OFF \
    -DUA_ENABLE_DA=ON \
    -DUA_ENABLE_DISCOVERY=ON \
    -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
    -DUA_ENABLE_ENCRYPTION=ON \
    -DUA_ENABLE_JSON_ENCODING=ON \
    -DUA_ENABLE_PUBSUB=ON \
    -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
    -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
    -DUA_ENABLE_SUBSCRIPTIONS=ON \
    -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=OFF \
    -DUA_NAMESPACE_ZERO=FULL ..
make -j && make test ARGS="-V"
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm build -rf
echo -en 'travis_fold:end:script.build.unit_test_ns0_full\\r'

if ! [ -z ${DEBIAN+x} ]; then
    echo -e "\r\n== Building the Debian package =="  && echo -en 'travis_fold:start:script.build.debian\\r'
    dpkg-buildpackage -b
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cp ../open62541*.deb .
    # Copy for github release script
    cp ../open62541*.deb ../..
    echo -en 'travis_fold:end:script.build.debian\\r'
fi


if [ "$CC" != "tcc" ]; then
    echo -e "\r\n== Unit tests (minimal NS0) ==" && echo -en 'travis_fold:start:script.build.unit_test_ns0_minimal\\r'
    mkdir -p build && cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_BUILD_UNIT_TESTS=ON \
        -DUA_ENABLE_COVERAGE=ON \
        -DUA_ENABLE_DA=ON \
        -DUA_ENABLE_DISCOVERY=ON \
        -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
        -DUA_ENABLE_ENCRYPTION=ON \
        -DUA_ENABLE_JSON_ENCODING=ON \
        -DUA_ENABLE_PUBSUB=ON \
        -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
        -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
        -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
        -DUA_NAMESPACE_ZERO=MINIMAL ..
    make -j && make test ARGS="-V"
    if [ $? -ne 0 ] ; then exit 1 ; fi
    cd .. && rm build -rf
    echo -en 'travis_fold:end:script.build.unit_test_ns0_minimal\\r'

    echo -e "\r\n== Unit tests (reduced NS0) ==" && echo -en 'travis_fold:start:script.build.unit_test_ns0_reduced\\r'
    mkdir -p build && cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
        -DUA_BUILD_EXAMPLES=ON \
        -DUA_BUILD_UNIT_TESTS=ON \
        -DUA_ENABLE_COVERAGE=ON \
        -DUA_ENABLE_DA=ON \
        -DUA_ENABLE_DISCOVERY=ON \
        -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
        -DUA_ENABLE_ENCRYPTION=ON \
        -DUA_ENABLE_JSON_ENCODING=ON \
        -DUA_ENABLE_PUBSUB=ON \
        -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
        -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
        -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
        -DUA_NAMESPACE_ZERO=REDUCED ..
    make -j && make test ARGS="-V"
    if [ $? -ne 0 ] ; then exit 1 ; fi
    echo -en 'travis_fold:end:script.build.unit_test_ns0_reduced\\r'

    # only run coveralls on main repo and when MINGW=true
    # We only want to build coveralls once, so we just take the travis run where MINGW=true which is only enabled once
    echo -e "\r\n== -> Current repo: ${TRAVIS_REPO_SLUG} =="
    if [ $MINGW = "true" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
        echo -en "\r\n==   Building codecov.io for ${TRAVIS_REPO_SLUG} ==" && echo -en 'travis_fold:start:script.build.codecov\\r'
        cd ..
        /bin/bash -c "bash <(curl -s https://codecov.io/bash)"
        if [ $? -ne 0 ] ; then exit 1 ; fi
        cd build
        echo -en 'travis_fold:end:script.build.codecov\\r'

        echo -en "\r\n==   Building coveralls for ${TRAVIS_REPO_SLUG} ==" && echo -en 'travis_fold:start:script.build.coveralls\\r'
        coveralls -E '.*/build/CMakeFiles/.*' -E '.*/examples/.*' -E '.*/tests/.*' -E '.*\.h' -E '.*CMakeCXXCompilerId\.cpp' -E '.*CMakeCCompilerId\.c' -r ../ || true # ignore result since coveralls is unreachable from time to time
        echo -en 'travis_fold:end:script.build.coveralls\\r'
    fi
    cd .. && rm build -rf
fi
