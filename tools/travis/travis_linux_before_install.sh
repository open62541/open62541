#!/bin/bash
set -ev


if [ -z ${DOCKER+x} ]; then
	# Only on non-docker builds required

	echo "=== Installing from external package sources ===" && echo -en 'travis_fold:start:before_install.external\\r'

	if [ "$CC" = "clang" ]; then
		# the ubuntu repo has a somehow broken clang-3.9 compiler. We want to use the one from the llvm repo
		# See https://github.com/openssl/openssl/commit/404c76f4ee1dc51c0d200e2b60a6340aadb44e38
		sudo cp .travis-apt-pin.preferences /etc/apt/preferences.d/no-ubuntu-clang
		curl -sSL "http://apt.llvm.org/llvm-snapshot.gpg.key" | sudo -E apt-key add -
		echo "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main" | sudo tee -a /etc/apt/sources.list > /dev/null
		sudo -E apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
		sudo -E apt-get -yq update
		sudo -E apt-get -yq --no-install-suggests --no-install-recommends --force-yes install clang-3.9 clang-tidy-3.9 libfuzzer-3.9-dev
	fi

	if [ "$CC" = "tcc" ]; then
		mkdir tcc_install && cd tcc_install
		wget https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2
		tar xf tcc-0.9.27.tar.bz2
		cd tcc-0.9.27
		./configure
		make
		sudo make install
		cd ../..
		rm -rf tcc_install
	fi

	wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.7.1.tar.gz
	tar xf mbedtls-2.7.1.tar.gz
	cd mbedtls-mbedtls-2.7.1
	cmake -DENABLE_TESTING=Off .
	make -j
	sudo make install

	echo -en 'travis_fold:end:script.before_install.external\\r'

	echo "=== Installing python packages ===" && echo -en 'travis_fold:start:before_install.python\\r'
	pip install --user cpp-coveralls
	pip install --user 'sphinx==1.7.9'
	pip install --user sphinx_rtd_theme
	pip install --user cpplint
	echo -en 'travis_fold:end:script.before_install.python\\r'

	echo "=== Installed versions are ===" && echo -en 'travis_fold:start:before_install.versions\\r'
	clang --version
	g++ --version
	cppcheck --version
	valgrind --version
	echo -en 'travis_fold:end:script.before_install.versions\\r'

fi
