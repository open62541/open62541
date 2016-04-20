#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    cd $LOCAL_PKG
    # travis caches the $LOCAL_PKG dir. If it is loaded, we don't need to reinstall the packages
    if [ "$CC" = "clang" ]; then
        clang --version
    else
        if [ ! -f $LOCAL_PKG/.cached_analyze ]; then
            # Install newer cppcheck
            wget https://github.com/danmar/cppcheck/archive/1.73.tar.gz -O cppcheck-1.73.tar.gz
            tar xf cppcheck-1.73.tar.gz
            cd $LOCAL_PKG/cppcheck-1.73
            make SRCDIR=build CFGDIR="$LOCAL_PKG/cppcheck-1.73/cfg" HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function" -j8
            ln -s $LOCAL_PKG/cppcheck-1.73/cppcheck $LOCAL_PKG/cppcheck
            # create cached flag
            touch $LOCAL_PKG/.cached_analyze
        else
            echo "\n## Using local packages from cache\n"
        fi
        g++ --version
        cppcheck --version
    fi
else
	cd $LOCAL_PKG

	# travis caches the $LOCAL_PKG dir. If it is loaded, we don't need to reinstall the packages
	if [ ! -f $LOCAL_PKG/.cached ]; then

		# Install newer valgrind
		mkdir -p $LOCAL_PKG/package && cd $LOCAL_PKG/package
		wget http://valgrind.org/downloads/valgrind-3.11.0.tar.bz2
		tar xf valgrind-3.11.0.tar.bz2
		cd valgrind-3.11.0
		./configure --prefix=$LOCAL_PKG
		make -s -j8 install
		echo "\n### Installed valgrind version: $(valgrind --version)"
		cd $LOCAL_PKG

		# Install specific check version which is not yet in the apt package
	 	wget http://ftp.de.debian.org/debian/pool/main/c/check/check_0.10.0-3_amd64.deb
	 	dpkg -x check_0.10.0-3_amd64.deb $LOCAL_PKG/package
	 	# change pkg-config file path
		sed -i "s|prefix=/usr|prefix=${LOCAL_PKG}|g" $LOCAL_PKG/package/usr/lib/x86_64-linux-gnu/pkgconfig/check.pc
		sed -i 's|libdir=.*|libdir=${prefix}/lib|g' $LOCAL_PKG/package/usr/lib/x86_64-linux-gnu/pkgconfig/check.pc
		# move files to globally included dirs
		cp -R $LOCAL_PKG/package/usr/lib/x86_64-linux-gnu/* $LOCAL_PKG/lib/
		cp -R $LOCAL_PKG/package/usr/include/* $LOCAL_PKG/include/
		cp -R $LOCAL_PKG/package/usr/bin/* $LOCAL_PKG/

		# Install specific liburcu version which is not yet in the apt package
		wget https://launchpad.net/ubuntu/+source/liburcu/0.8.5-1ubuntu1/+build/6513813/+files/liburcu2_0.8.5-1ubuntu1_amd64.deb
		wget https://launchpad.net/ubuntu/+source/liburcu/0.8.5-1ubuntu1/+build/6513813/+files/liburcu-dev_0.8.5-1ubuntu1_amd64.deb
		dpkg -x liburcu2_0.8.5-1ubuntu1_amd64.deb $LOCAL_PKG/package
		dpkg -x liburcu-dev_0.8.5-1ubuntu1_amd64.deb $LOCAL_PKG/package
		# move files to globally included dirs
		cp -R $LOCAL_PKG/package/usr/lib/x86_64-linux-gnu/* $LOCAL_PKG/lib/
		cp -R $LOCAL_PKG/package/usr/include/* $LOCAL_PKG/include/

		# create cached flag
		touch $LOCAL_PKG/.cached

	else
		echo "\n## Using local packages from cache\n"
	fi



	pip install --user cpp-coveralls
	pip install --user sphinx
	pip install --user sphinx_rtd_theme
fi
