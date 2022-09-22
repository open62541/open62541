#!/bin/bash

function build_install_open62541() {

	### Install dependencies ###
	sudo apt-get install git build-essential gcc pkg-config cmake python
	sudo apt-get install cmake-curses-gui # for the ccmake graphical interface
	sudo apt-get install libmbedtls-dev # for encryption support
	sudo apt-get install check libsubunit-dev # for unit tests
	sudo apt-get install sphinx-doc sphinx-common graphviz # for documentation generation
	sudo apt-get install python3-sphinx-rtd-theme # documentation style
	sudo apt-get install python-dev python3-dev

	### Check if XML2 version 2.9.14 exists, if not install it ###
	var=$(find /usr/ -iname "*libxml2.so.2.9.14" | wc -l)
	if [ "$var" = "0" ]; then
		echo "Build \"libxml2\" from source"
		wget https://download.gnome.org/sources/libxml2/2.9/libxml2-2.9.14.tar.xz -P deps/libxml2
		cd deps/libxml2/
		tar xvf libxml2-2.9.14.tar.xz
		rm libxml2-2.9.14.tar.xz
		cd libxml2-2.9.14

		./configure
		make
		make check
		make install
		sudo make install
	fi

	### Initialize submodules ###
	git submodule update --remote --init --recursive

	### Build open62541 library ###
  	mkdir -p build && cd build

	cmake \
		-DBUILD_SHARED_LIBS=ON \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DUA_NAMESPACE_ZERO=$1 \
		-DUA_BUILD_UNIT_TESTS=$2 \
		..

	make -j$(nproc --all)

	### Install open62541 library ###
	sudo make install
	cd ..
}

function build_open62541_examples() {
	cd examples
	mkdir -p build
	cd build
	rm -rf *

	cmake \
		-DUA_NAMESPACE_ZERO=$1 \
		..
	
	make -j$(nproc --all)
	
	cd ../..
}

namespaceZero=FULL
enableTests=OFF

if [ "$1" = "examples" ]; then
	build_open62541_examples $namespaceZero
else
	if [ "$1" = "reduced" ]; then
		namespaceZero=REDUCED
	elif [ "$1" = "minimal" ]; then
		namespaceZero=MINIMAL
	elif [ "$1" = "tests" ]; then
		enableTests=ON
	fi

	build_install_open62541 $namespaceZero $enableTests

	if [ "$2" = "examples" ]; then
		build_open62541_examples $namespaceZero
	fi
fi
