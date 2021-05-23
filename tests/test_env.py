#------------------------------------------------------------------------------
# test_env.py
#   Base file for all unit tests.
#------------------------------------------------------------------------------

import ceODBC
import os
import unittest

# dictionary containing all parameters; these are acquired as needed by the
# methods below (which should be used instead of consulting this dictionary
# directly) and then stored in the dictionary for subsequent use
PARAMETERS = {}

def get_value(name, label):
    value = PARAMETERS.get(name)
    if value is not None:
        return value
    env_name = "CEODBC_TEST_" + name
    value = os.environ.get(env_name)
    if value is None:
        value = input(label + ": ")
    PARAMETERS[name] = value
    return value

def get_connection():
    return ceODBC.connect(get_dsn())

def get_dsn():
    dsn_type = get_dsn_type()
    return get_value(dsn_type.upper() + "_DSN", "DSN")

def get_dsn_type():
    return get_value("DSN_TYPE", "DSN Type")

def run_test_cases():
    unittest.main(testRunner=unittest.TextTestRunner(verbosity=2))

class BaseTestCase(unittest.TestCase):
    establish_connection = True

    def setUp(self):
        if self.establish_connection:
            self.connection = get_connection()
            self.cursor = self.connection.cursor()

    def tearDown(self):
        if self.establish_connection:
            self.connection.close()
            del self.cursor
            del self.connection
