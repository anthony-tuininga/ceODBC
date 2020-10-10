#------------------------------------------------------------------------------
# test_1000_types.py
#   Module for testing the database and API types.
#------------------------------------------------------------------------------

import base

import ceODBC
import pickle

class TestCase(base.BaseTestCase):
    establish_connection = False

    def __test_compare(self, db_type, api_type):
        self.assertEqual(db_type, db_type)
        self.assertEqual(db_type, api_type)
        self.assertEqual(api_type, db_type)
        self.assertNotEqual(db_type, 5)

    def __test_pickle(self, typ):
        self.assertIs(typ, pickle.loads(pickle.dumps(typ)))

    def test_1000_DB_TYPE_BIGINT(self):
        "1000 - test DB_TYPE_BIGINT comparisons"
        self.__test_compare(ceODBC.DB_TYPE_BIGINT, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_BIGINT)

    def test_1001_DB_TYPE_INT(self):
        "1001 - test DB_TYPE_INT comparisons"
        self.__test_compare(ceODBC.DB_TYPE_INT, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_INT)

    def test_1002_DB_TYPE_STRING(self):
        "1002 - test DB_TYPE_STRING comparisons"
        self.__test_compare(ceODBC.DB_TYPE_STRING, ceODBC.STRING)
        self.__test_pickle(ceODBC.DB_TYPE_STRING)

if __name__ == "__main__":
    base.run_test_cases()
