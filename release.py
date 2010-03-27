"""
Script for creating all of the binaries that are released for the current
platform.
"""

import os
import sys

mode = None
if len(sys.argv) > 1:
    mode = sys.argv[1]

pythonVersions = os.environ["CEODBC_PYTHON_VERSIONS"].split(",")
pythonFormat = os.environ["CEODBC_PYTHON_FORMAT"]

for version in pythonVersions:
    majorVersion, minorVersion = [int(s) for s in version.split(".")]
    python = pythonFormat % (majorVersion, minorVersion)
    if mode is not None:
        subCommand = mode
        subCommandArgs = ""
    elif sys.platform == "win32":
        subCommandArgs = ""
        if majorVersion == 2 and minorVersion == 4:
            subCommand = "bdist_wininst"
        else:
            subCommand = "bdist_msi"
    else:
        subCommand = "bdist_rpm"
        subCommandArgs = "--no-autoreq --python %s" % python
    command = "%s setup.py %s %s" % \
            (python, subCommand, subCommandArgs)
    messageFragment = "%s for Python %s.%s" % \
            (subCommand, majorVersion, minorVersion)
    sys.stdout.write("Executing %s.\n" % messageFragment)
    sys.stdout.write("Running command %s\n" % command)
    if os.system(command) != 0:
        msg = "Stopping. execution of %s failed.\n" % messageFragment
        sys.exit(msg)

