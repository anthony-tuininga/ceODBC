"""Distutils script for ceODBC.

Windows platforms:
    python setup.py build --compiler=mingw32 install

Unix platforms
    python setup.py build install
"""

import distutils.command.build_ext
import distutils.command.bdist_rpm
import os
import sys

from distutils.core import setup
from distutils.errors import DistutilsSetupError
from distutils.extension import Extension

# define build version
BUILD_VERSION = "2.0.1"

# define class to allow building the module with or without cx_Logging
class build_ext(distutils.command.build_ext.build_ext):
    global distutils
    global os
    user_options = distutils.command.build_ext.build_ext.user_options + [
        ('with-cx-logging', None,
         'include logging with the cx_Logging module'),
        ('cx-logging', None,
         'specify the location of the cx_Logging sources')
    ]

    def _build_cx_logging(self):
        sourceDir = self.cx_logging
        origDir = os.getcwd()
        scriptArgs = ["build"]
        command = self.distribution.get_command_obj("build")
        if command.compiler is not None:
            scriptArgs.append("--compiler=%s" % command.compiler)
        os.chdir(sourceDir)
        sys.stdout.write("building cx_Logging in %s\n" % sourceDir)
        distribution = distutils.core.run_setup("setup.py", scriptArgs)
        module, = distribution.ext_modules
        command = distribution.get_command_obj("build_ext")
        command.ensure_finalized()
        if command.compiler is None:
            command.run()
        else:
            command.build_extensions()
        os.chdir(origDir)
        if sys.platform == "win32":
            return os.path.join(sourceDir, command.importLibraryName)
        else:
            return os.path.join(sourceDir, command.build_lib,
                    command.get_ext_filename(module.name))

    def build_extension(self, ext):
        if self.with_cx_logging:
            os.environ["LD_RUN_PATH"] = "${ORIGIN}"
            ext.define_macros.append(("WITH_CX_LOGGING", None))
            ext.include_dirs = [self.cx_logging]
            name = self._build_cx_logging()
            if sys.platform == "win32":
                ext.library_dirs = [os.path.dirname(name)]
                ext.libraries.append("cx_Logging")
            else:
                ext.extra_link_args = [name]
        distutils.command.build_ext.build_ext.build_extension(self, ext)

    def finalize_options(self):
        distutils.command.build_ext.build_ext.finalize_options(self)
        envName = "CX_LOGGING_SOURCE"
        if self.with_cx_logging is None:
            self.with_cx_logging = envName in os.environ
        if self.with_cx_logging:
            if self.cx_logging is None:
                self.cx_logging = os.environ.get(envName)
            if self.cx_logging is None:
                dirName = os.path.join("..", "cx_Logging")
                self.cx_logging = os.path.realpath(dirName)
            os.environ[envName] = self.cx_logging

    def initialize_options(self):
        distutils.command.build_ext.build_ext.initialize_options(self)
        self.with_cx_logging = None
        self.cx_logging = None


class bdist_rpm(distutils.command.bdist_rpm.bdist_rpm):

    def run(self):
        distutils.command.bdist_rpm.bdist_rpm.run(self)
        specFile = os.path.join(self.rpm_base, "SPECS",
                "%s.spec" % self.distribution.get_name())
        queryFormat = "%{name}-%{version}-%{release}.%{arch}.rpm"
        command = "rpm -q --qf '%s' --specfile %s" % (queryFormat, specFile)
        origFileName = os.popen(command).read()
        parts = origFileName.split("-")
        parts.insert(2, "py%s%s" % sys.version_info[:2])
        newFileName = "-".join(parts)
        self.move_file(os.path.join("dist", origFileName),
        os.path.join("dist", newFileName))


class test(distutils.core.Command):
    user_options = []

    def finalize_options(self):
        pass

    def initialize_options(self):
        pass

    def run(self):
        self.run_command("build")
        buildCommand = self.distribution.get_command_obj("build")
        sys.path.insert(0, os.path.abspath(os.path.join("test", self.subdir)))
        sys.path.insert(0, os.path.abspath(buildCommand.build_lib))
        if sys.version_info[0] < 3:
            execfile(os.path.join("test", self.subdir, "test.py"))
        else:
            fileName = os.path.join("test", self.subdir, "test3k.py")
            exec(open(fileName).read())

class test_pgsql(test):
    description = "run the test suite for PostgreSQL"
    subdir = "pgsql"


class test_mysql(test):
    description = "run the test suite for MySQL"
    subdir = "mysql"


class test_sqlserver(test):
    description = "run the test suite for SQL Server"
    subdir = "sqlserver"


# define the list of files to be included as documentation
dataFiles = None
docFiles = "HISTORY.txt LICENSE.txt README.txt html test"
if sys.platform in ("win32", "cygwin"):
    baseName = "ceODBC-doc"
    dataFiles = [ (baseName, [ "HISTORY.TXT", "LICENSE.TXT", "README.TXT" ]) ]
    for subDir in ("html", "test"):
        for path, dirNames, fileNames in os.walk(subDir):
            if ".svn" in dirNames:
                dirNames.remove(".svn")
            qualifiedFileNames = [os.path.join(path, n) for n in fileNames]
            qualifiedPath = os.path.join(baseName, path)
            dataFiles.append((qualifiedPath, qualifiedFileNames))

# setup link and compile args
defineMacros = [("BUILD_VERSION", BUILD_VERSION)]
if sys.platform == "win32":
    libs = ["odbc32"]
else:
    libs = ["odbc"]

# define command classes
commandClasses = dict(
        bdist_rpm = bdist_rpm,
        build_ext = build_ext,
        test_pgsql = test_pgsql,
        test_mysql = test_mysql,
        test_sqlserver = test_sqlserver)

# define the classifiers for the package index
classifiers = [
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Python Software Foundation License",
        "Natural Language :: English",
        "Operating System :: OS Independent",
        "Programming Language :: C",
        "Programming Language :: Python",
        "Topic :: Database"
]

# setup the extension
extension = Extension(
        name = "ceODBC",
        libraries = libs,
        define_macros = defineMacros,
        sources = ["ceODBC.c"],
        depends = ["ApiTypes.c", "BinaryVar.c", "BitVar.c", "Connection.c",
                "Cursor.c", "DateTimeVar.c", "Environment.c", "Error.c",
                "NumberVar.c", "StringUtils.c", "StringVar.c", "UnicodeVar.c",
                "Variable.c"])

# perform the setup
setup(
        name = "ceODBC",
        version = BUILD_VERSION,
        description = "Python interface to ODBC",
        license = "Python Software Foundation License",
        long_description = \
            "Python interface to ODBC conforming to the Python DB API 2.0 "
            "specification.\n"
            "See http://www.python.org/topics/database/DatabaseAPI-2.0.html.",
        author = "Anthony Tuininga",
        author_email = "anthony.tuininga@gmail.com",
        maintainer = "Anthony Tuininga",
        maintainer_email = "anthony.tuininga@gmail.com",
        url = "http://ceodbc.sourceforge.net",
        ext_modules = [extension],
        data_files = dataFiles,
        classifiers = classifiers,
        cmdclass = commandClasses,
        options = dict(bdist_rpm = dict(doc_files = docFiles)))

