"""
Script for building the extensions from within pyproject.toml.
"""

import os
import sys

from setuptools import Extension
from setuptools.command.build_py import build_py as _build_py

class build_py(_build_py):

    def run(self):
        self.run_command("build_ext")
        return super().run()

    def initialize_options(self):
        super().initialize_options()
        if sys.platform == "win32":
            libs = ["odbc32"]
        else:
            libs = ["odbc"]
        base_dir = os.path.join("src", "ceODBC")
        depends = [os.path.join(base_dir, n) \
                   for n in sorted(os.listdir(base_dir))
                   if n.endswith(".pyx") and n != "driver.pyx" \
                           or n.endswith(".pxd")]
        if self.distribution.ext_modules is None:
            self.distribution.ext_modules = []
        self.distribution.ext_modules.append(
            Extension(
                "ceODBC.driver",
                sources=["src/ceODBC/driver.pyx"],
                depends=depends,
                libraries=libs
            )
        )
