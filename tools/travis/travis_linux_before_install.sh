#!/bin/bash
set -ev


if ! [ -z ${FUZZER+x} ]; then
	# we need libfuzzer 5.0, all the older versions do not work on travis.
	sudo apt-get --yes install git
	git clone https://github.com/google/fuzzer-test-suite.git FTS
	./FTS/tutorial/install-deps.sh  # Get deps
	./FTS/tutorial/install-clang.sh # Get fresh clang binaries
	# Get libFuzzer sources and build it
	svn co http://llvm.org/svn/llvm-project/llvm/trunk/lib/Fuzzer
	Fuzzer/build.sh
	exit 0
fi

if [ -z ${DOCKER+x} ]; then
	# Only on non-docker builds required

	echo "=== Installing from external package sources ===" && echo -en 'travis_fold:start:before_install.external\\r'
	sudo add-apt-repository -y ppa:lttng/ppa
	sudo apt-get update -qq
	sudo apt-get install -y liburcu4 liburcu-dev
	echo -en 'travis_fold:end:script.before_install.external\\r'

	echo "=== Installing python packages ===" && echo -en 'travis_fold:start:before_install.python\\r'
	pip install --user cpp-coveralls
	pip install --user sphinx
	pip install --user sphinx_rtd_theme
	echo -en 'travis_fold:end:script.before_install.python\\r'

	echo "=== Installed versions are ===" && echo -en 'travis_fold:start:before_install.versions\\r'
	clang --version
	g++ --version
	cppcheck --version
	valgrind --version
	echo -en 'travis_fold:end:script.before_install.versions\\r'

fi
