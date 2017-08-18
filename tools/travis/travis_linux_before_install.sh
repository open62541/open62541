#!/bin/bash
set -ev

if [ -z ${DOCKER+x} ]; then
	# Only on non-docker builds required

	echo "=== Installing from external package sources ===" && echo -en 'travis_fold:start:before_install.external\\r'
	wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
	echo "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main" | sudo tee -a /etc/apt/sources.list
	sudo add-apt-repository -y ppa:lttng/ppa
	sudo apt-get update -qq
	sudo apt-get install -y clang-3.9 clang-tidy-3.9
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
