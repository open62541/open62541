Building the Library
====================

Building the Single-File Release
--------------------------------

Using the GCC compiler, the following calls build the library on Linux.

.. code-block:: bash

   gcc -std=c99 -fPIC -c open62541.c
   gcc -shared open62541.o -o libopen62541.so
   

Building with CMake on Ubuntu or Debian
-----------------------------

.. code-block:: bash
   
   sudo apt-get install git build-essential gcc pkg-config cmake python python-lxml

   # enable additional features
   sudo apt-get install libexpat1-dev # for XML-encodingi
   sudo apt-get install liburcu-dev # for multithreading
   sudo apt-get install check # for unit tests
   sudo apt-get install graphviz doxygen # for documentation generation

   cd open62541
   mkdir build
   cd build
   cmake ..
   make

   # select additional features
   ccmake ..
   make

Building with CMake on Windows (Visual Studio)
----------------------------------------------

- Get and install Python 2.7.x (Python 3.x should work, too) and CMake: https://python.org/downloads, http://www.cmake.org/cmake/resources/software.html
- Get and install Visual Studio 2015 Preview: https://www.visualstudio.com/downloads/visual-studio-2015-ctp-vs
- Download the open62541 sources (using git or as a zipfile from github)
- Open a command shell (cmd) with Administrator rights and run

.. code-block:: bat

   <path-to-python>\Scripts\pip.exe install lxml

- Open a command shell (cmd) and run

.. code-block:: bat

   cd <path-to>\open62541
   mkdir build
   cd build
   <path-to>\cmake.exe .. -G "Visual Studio 14 2015"
   :: You can use use cmake-gui for a graphical user-interface to select single features

- Then open "build\open62541.sln" in Visual Studio 2015 and build as usual
   
Build Options
-------------

