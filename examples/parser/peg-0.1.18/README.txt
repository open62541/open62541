Building on a Unix-like system
------------------------------

Type 'make' or 'make test'.

The latter builds all the examples and runs them, comparing their
output with the expected output.

Type 'make install' to install the binaries and manual page under
/usr/local.  (Type 'make uninstall' to remove them.)  You may have to
do this using 'sudo' or while logged in as root.

Edit 'Makefile' to change the way things are built and/or the places
where things are installed.


Building on MacOS X
-------------------

Run the 'build-mac.sh' script from a terminal or by double-clicking on
it in the Finder.

You will need Xcode.  The provided project is known to work with Xcode
versions 3.2.6 and 4.3.2.

Modify build-mac.sh and/or peg.xcodeproj to change the way things are
built.


Building on Windows
-------------------

Run the 'build-win.cmd' script.

You will need Visual Studio 2010 Express.

Modify build-win.cmd, leg.vcxproj, leg.vcxproj.filters, peg.gyp,
peg.sln, peg.vcxproj and/or peg.vcxproj.filters to change the way
things are built.

Local implementations of getopt() and basename() are provided in the
'win' directory.
