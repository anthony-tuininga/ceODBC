"""Define test environment."""

import ceODBC
import os
import unittest

def get_value(name, label):
    value = os.environ.get("CEODBC_" + name)
    if value is None:
        value = raw_input(label + ": ")
    return value

def run_test_cases():
    unittest.main(testRunner=unittest.TextTestRunner(verbosity=2))

DSN = get_value("PGSQL_DSN", "DSN")
ARRAY_SIZE = int(get_value("ARRAY_SIZE", "array size"))

class BaseTestCase(unittest.TestCase):

    def getConnection(self):
        return ceODBC.Connection(DSN)

    def setUp(self):
        self.connection = self.getConnection()
        self.cursor = self.connection.cursor()
        self.cursor.arraysize = ARRAY_SIZE

    def tearDown(self):
        self.connection.close()
        del self.cursor
        del self.connection
