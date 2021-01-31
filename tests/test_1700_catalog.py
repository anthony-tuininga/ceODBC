#------------------------------------------------------------------------------
# test_1700_catalog.py
#   Module for testing the various catalog routines.
#------------------------------------------------------------------------------

import base

import ceODBC
import threading

class TestCase(base.BaseTestCase):

    def test_1700_columns(self):
        "1700 - test connection.columns()"
        num_all_columns = len(list(self.connection.columns()))
        self.assertTrue(num_all_columns > 0)
        num_columns = len(list(self.connection.columns(table="testtemptable")))
        self.assertEqual(num_columns, 2)
        num_columns = len(list(self.connection.columns(column="bigintcol")))
        self.assertEqual(num_columns, 1)

    def test_1701_primary_keys(self):
        "1701 - test connection.primarykeys()"
        num = len(list(self.connection.primarykeys(table="testtemptable")))
        self.assertEqual(num, 1)

    def test_1702_procedures(self):
        "1702 - test connection.procedures()"
        num = len(list(self.connection.procedures(proc="sp_test")))
        self.assertEqual(num, 1)

    def test_1703_procedure_columns(self):
        "1703 - test connection.procedurecolumns()"
        num = len(list(self.connection.procedurecolumns(proc="sp_test")))
        self.assertEqual(num, 3)

    def test_1704_tables(self):
        "1704 - test connection.tables()"
        num = len(list(self.connection.tables(table="testtemptable")))
        self.assertEqual(num, 1)

    def test_1705_table_privileges(self):
        "1705 - test connection.tableprivileges()"
        num = len(list(self.connection.tableprivileges(table="testtemptable")))
        self.assertTrue(num > 0)

if __name__ == "__main__":
    base.run_test_cases()
