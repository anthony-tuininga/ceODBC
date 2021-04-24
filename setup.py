"""
Setup script for ceODBC driver.
"""

import os
import sys

from setuptools import setup, Extension

if sys.platform == "win32":
    libs = ["odbc32"]
else:
    libs = ["odbc"]

base_dir = os.path.join("src", "ceODBC")
depends = [os.path.join(base_dir, n) \
           for n in sorted(os.listdir(base_dir))
           if n.endswith(".pyx") and n != "driver.pyx" or n.endswith(".pxd")]

setup(
    ext_modules=[
        Extension("ceODBC.driver",
                  sources=["src/ceODBC/driver.pyx"],
                  depends=depends,
                  libraries=libs)
    ]
)
