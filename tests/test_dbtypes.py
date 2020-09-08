#------------------------------------------------------------------------------
# test_connection.pyx
#   Module for testing the connection object.
#------------------------------------------------------------------------------

import base

import ceODBC

class TestCase(base.BaseTestCase):
    establish_connection = False

    def _test_compare(self, db_type, api_type):
        self.assertEqual(db_type, db_type)
        self.assertEqual(db_type, api_type)
        self.assertEqual(api_type, db_type)
        self.assertNotEqual(db_type, 5)

    def test_DB_TYPE_BIGINT(self):
        "test DB_TYPE_BIGINT comparisons"
        self._test_compare(ceODBC.DB_TYPE_BIGINT, ceODBC.NUMBER)

    def test_DB_TYPE_INT(self):
        "test DB_TYPE_INT comparisons"
        self._test_compare(ceODBC.DB_TYPE_INT, ceODBC.NUMBER)

    def test_DB_TYPE_STRING(self):
        "test DB_TYPE_STRING comparisons"
        self._test_compare(ceODBC.DB_TYPE_STRING, ceODBC.STRING)

if __name__ == "__main__":
    base.run_test_cases()
