"""Runs all defined unit tests."""

import ceODBC
import sys
import unittest

import TestEnv

print("Running tests for ceODBC version", ceODBC.version)
print("File:", ceODBC.__file__)

moduleNames = [
    "Connection",
    "Cursor",
    "NumberVar",
    "StringVar"
]

failures = []
loader = unittest.TestLoader()
runner = unittest.TextTestRunner(verbosity=2)
for name in moduleNames:
    fileName = name + ".py"
    print()
    print("Running tests in", name)
    tests = loader.loadTestsFromName(name + ".TestCase")
    result = runner.run(tests)
    if not result.wasSuccessful():
        failures.append(name)

if failures:
    print("***** Some tests in the following modules failed. *****")
    for name in failures:
        print("      %s" % name)
    sys.exit(1)
