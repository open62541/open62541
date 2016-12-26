
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

setup(
    ext_modules=cythonize([
        Extension(
            name="openua",
            sources=["openua.pyx"],
            include_dirs=["../include", "../plugins", "../build/src_generated/", "../deps"],
            libraries=["open62541"],
            library_dirs=["../build"]
        )

    ])
)
