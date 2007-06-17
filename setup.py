"""Distutils script for ceODBC.

Windows platforms:
    python setup.py build --compiler=mingw32 install

Unix platforms
    python setup.py build install
"""

import os
import sys

from distutils.core import setup
from distutils.errors import DistutilsSetupError
from distutils.extension import Extension

# define build constants
BUILD_VERSION = "HEAD"

# setup link and compile args
if sys.platform == "win32":
    import win32api
    libs = ["odbc32"]
    extraLinkArgs = [win32api.GetModuleFileName(sys.dllhandle)]
    extraCompileArgs = ["-DBUILD_VERSION=%s" % BUILD_VERSION]
else:
    libs = ["odbc"]
    extraLinkArgs = []
    extraCompileArgs = []

# setup the extension
extension = Extension(
        name = "ceODBC",
        libraries = libs,
        extra_compile_args = extraCompileArgs,
        extra_link_args = extraLinkArgs,
        sources = ["ceODBC.c"])

# perform the setup
setup(
        name = "ceODBC",
        version = BUILD_VERSION,
        description = "Python interface to ODBC",
        license = "See LICENSE.txt",
        long_description = \
            "Python interface to ODBC conforming to the Python DB API 2.0 "
            "specification.\n"
            "See http://www.python.org/topics/database/DatabaseAPI-2.0.html.",
        author = "Anthony Tuininga",
        author_email = "anthony.tuininga@gmail.com",
        url = "http://starship.python.net/crew/atuining",
        ext_modules = [extension])

