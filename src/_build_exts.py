"""
Script for building the extensions from within pyproject.toml.
"""

import os
import sys

from setuptools import Extension
from setuptools.command.build import build as _build


class build(_build):

    def finalize_options(self):
        super().finalize_options()
        if sys.platform == "win32":
            libs = ["odbc32"]
        else:
            libs = ["odbc"]
        base_dir = os.path.join("src", "ceODBC")
        depends = [
            os.path.join(base_dir, n)
            for n in sorted(os.listdir(base_dir))
            if n.endswith(".pyx") and n != "driver.pyx" or n.endswith(".pxd")
        ]
        if self.distribution.ext_modules is None:
            self.distribution.ext_modules = []
        self.distribution.ext_modules.append(
            Extension(
                "ceODBC.driver",
                sources=["src/ceODBC/driver.pyx"],
                depends=depends,
                libraries=libs,
            )
        )
