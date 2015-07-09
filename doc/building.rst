Building the Library
====================

Building the Single-File Release
--------------------------------

Using the GCC compiler, the following calls build the library on Linux.
.. code-block:: bash

    gcc -std=c99 -fPIC -c open62541.c
    gcc -shared open62541.o -o libopen62541.so
   

Building with CMake on Ubuntu
-----------------------------

.. code-block:: bash
   
   sudo apt-get install git build-essential gcc cmake python python-lxml

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
   
Build Options
-------------


Building the Examples
=====================
