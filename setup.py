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

# define build version
BUILD_VERSION = "HEAD"

# specify whether the cx_Logging module should be used for logging;
# the environment variable CX_LOGGING_INCLUDE_DIR is used to locate the
# include file cx_Logging.h and the environment variable CX_LOGGING_LIB_DIR
# is used to locate the cx_Logging library
WITH_CX_LOGGING = True

# setup link and compile args
includeDirs = []
libraryDirs = []
defineMacros = [("BUILD_VERSION", BUILD_VERSION)]
if sys.platform == "win32":
    libs = ["odbc32"]
else:
    libs = ["odbc"]
if WITH_CX_LOGGING:
    libraryDirs.append(os.environ["CX_LOGGING_LIB_DIR"])
    libs.append("cx_Logging")
    includeDirs = [os.environ["CX_LOGGING_INCLUDE_DIR"]]
    defineMacros.append(("WITH_CX_LOGGING", None))

# setup the extension
extension = Extension(
        name = "ceODBC",
        include_dirs = includeDirs,
        library_dirs = libraryDirs,
        libraries = libs,
        define_macros = defineMacros,
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

