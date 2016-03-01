#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
  	g++ --version
  	cppcheck --version
else
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



	pip install --user cpp-coveralls
	pip install --user sphinx
	pip install --user sphinx_rtd_theme
fi