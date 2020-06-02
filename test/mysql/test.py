"""Runs all defined unit tests."""

import ceODBC
import imp
import os
import sys
import unittest

inSetup = (os.path.basename(sys.argv[0]).lower() == "setup.py")

print("Running tests for ceODBC version", ceODBC.version)

import TestEnv

if len(sys.argv) > 1 and not inSetup:
    moduleNames = [os.path.splitext(v)[0] for v in sys.argv[1:]]
else:
    moduleNames = [
            "Connection",
            "Cursor",
            "NumberVar",
            "StringVar"
    ]

class BaseTestCase(unittest.TestCase):

    def setUp(self):
        import ceODBC
        import TestEnv
        self.connection = ceODBC.Connection(TestEnv.DSN)
        self.cursor = self.connection.cursor()
        self.cursor.arraysize = TestEnv.ARRAY_SIZE

    def tearDown(self):
        del self.cursor
        del self.connection


loader = unittest.TestLoader()
runner = unittest.TextTestRunner(verbosity = 2)
failures = []
for name in moduleNames:
    fileName = name + ".py"
    print()
    print("Running tests in", fileName)
    if inSetup:
        fileName = os.path.join("test", "mysql", fileName)
    module = imp.new_module(name)
    import ceODBC
    setattr(module, "DSN", TestEnv.DSN)
    setattr(module, "ARRAY_SIZE", TestEnv.ARRAY_SIZE)
    setattr(module, "TestCase", unittest.TestCase)
    setattr(module, "BaseTestCase", BaseTestCase)
    setattr(module, "ceODBC", ceODBC)
    exec(open(fileName).read(), module.__dict__)
    tests = loader.loadTestsFromModule(module)
    result = runner.run(tests)
    if not result.wasSuccessful():
        failures.append(name)
if failures:
    print("***** Some tests in the following modules failed. *****")
    for name in failures:
        print("      %s" % name)
    sys.exit(1)

