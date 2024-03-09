# -----------------------------------------------------------------------------
# test_1000_types.py
#   Module for testing the database and API types.
# -----------------------------------------------------------------------------

import test_env

import ceODBC
import pickle


class TestCase(test_env.BaseTestCase):
    establish_connection = False

    def __test_compare(self, db_type, api_type):
        self.assertEqual(db_type, db_type)
        self.assertEqual(db_type, api_type)
        self.assertEqual(api_type, db_type)
        self.assertNotEqual(db_type, 5)

    def __test_pickle(self, typ):
        self.assertIs(typ, pickle.loads(pickle.dumps(typ)))

    def test_1000(self):
        "1000 - test DB_TYPE_BIGINT comparisons"
        self.__test_compare(ceODBC.DB_TYPE_BIGINT, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_BIGINT)

    def test_1001(self):
        "1001 - test DB_TYPE_BINARY comparisons"
        self.__test_compare(ceODBC.DB_TYPE_BINARY, ceODBC.BINARY)
        self.__test_pickle(ceODBC.DB_TYPE_BINARY)

    def test_1002(self):
        "1002 - test DB_TYPE_BIT comparisons"
        self.__test_pickle(ceODBC.DB_TYPE_BIT)

    def test_1003(self):
        "1003 - test DB_TYPE_DATE comparisons"
        self.__test_compare(ceODBC.DB_TYPE_DATE, ceODBC.DATETIME)
        self.__test_pickle(ceODBC.DB_TYPE_DATE)

    def test_1004(self):
        "1004 - test DB_TYPE_DECIMAL comparisons"
        self.__test_compare(ceODBC.DB_TYPE_DECIMAL, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_DECIMAL)

    def test_1005(self):
        "1005 - test DB_TYPE_DOUBLE comparisons"
        self.__test_compare(ceODBC.DB_TYPE_DOUBLE, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_DOUBLE)

    def test_1006(self):
        "1006 - test DB_TYPE_INT comparisons"
        self.__test_compare(ceODBC.DB_TYPE_INT, ceODBC.NUMBER)
        self.__test_pickle(ceODBC.DB_TYPE_INT)

    def test_1007(self):
        "1007 - test DB_TYPE_LONG_BINARY comparisons"
        self.__test_compare(ceODBC.DB_TYPE_LONG_BINARY, ceODBC.BINARY)
        self.__test_pickle(ceODBC.DB_TYPE_LONG_BINARY)

    def test_1008(self):
        "1008 - test DB_TYPE_LONG_STRING comparisons"
        self.__test_compare(ceODBC.DB_TYPE_LONG_STRING, ceODBC.STRING)
        self.__test_pickle(ceODBC.DB_TYPE_LONG_STRING)

    def test_1009(self):
        "1009 - test DB_TYPE_STRING comparisons"
        self.__test_compare(ceODBC.DB_TYPE_STRING, ceODBC.STRING)
        self.__test_pickle(ceODBC.DB_TYPE_STRING)

    def test_1010(self):
        "1010 - test DB_TYPE_TIME comparisons"
        self.__test_pickle(ceODBC.DB_TYPE_TIME)

    def test_1011(self):
        "1011 - test DB_TYPE_TIMESTAMP comparisons"
        self.__test_compare(ceODBC.DB_TYPE_TIMESTAMP, ceODBC.DATETIME)
        self.__test_pickle(ceODBC.DB_TYPE_TIMESTAMP)


if __name__ == "__main__":
    test_env.run_test_cases()
