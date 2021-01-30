#------------------------------------------------------------------------------
# test_1700_catalog.py
#   Module for testing the various catalog routines.
#------------------------------------------------------------------------------

import base

import ceODBC
import threading

class TestCase(base.BaseTestCase):

    def __connect_and_drop(self):
        "connect to the database, perform a query and drop the connection"
        connection = base.get_connection()
        cursor = connection.cursor()
        cursor.execute("select count(*) from TestNumbers")
        count, = cursor.fetchone()
        self.assertEqual(count, 4)

    def test_1700_columns(self):
        "1700 - test connection.columns()"
        num_all_columns = len(list(self.connection.columns()))
        self.assertTrue(num_all_columns > 0)
        num_columns = len(list(self.connection.columns(table="testtemptable")))
        self.assertEqual(num_columns, 2)
        num_columns = len(list(self.connection.columns(column="bigintcol")))
        self.assertEqual(num_columns, 1)

if __name__ == "__main__":
    base.run_test_cases()
