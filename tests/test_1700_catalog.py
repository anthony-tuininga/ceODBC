#------------------------------------------------------------------------------
# test_1700_catalog.py
#   Module for testing the various catalog routines.
#------------------------------------------------------------------------------

import threading
import unittest

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):

    def __get_catalog_name(self, name):
        if test_env.get_dsn_type() == "pgsql":
            return name.lower()
        return name

    def test_1700_columns(self):
        "1700 - test connection.columns()"
        num_all_columns = len(list(self.connection.columns()))
        self.assertTrue(num_all_columns > 0)
        table_name = self.__get_catalog_name("TestTempTable")
        num_columns = len(list(self.connection.columns(table=table_name)))
        self.assertEqual(num_columns, 2)
        num_columns = len(list(self.connection.columns(column="bigintcol")))
        self.assertEqual(num_columns, 1)

    def test_1701_primary_keys(self):
        "1701 - test connection.primarykeys()"
        table_name = self.__get_catalog_name("TestTempTable")
        num = len(list(self.connection.primarykeys(table=table_name)))
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
        table_name = self.__get_catalog_name("TestTempTable")
        num = len(list(self.connection.tables(table=table_name)))
        self.assertEqual(num, 1)

    @unittest.skipIf(test_env.get_dsn_type() == "mysql",
                     "MySQL doesn't have table privileges by default")
    def test_1705_table_privileges(self):
        "1705 - test connection.tableprivileges()"
        table_name = self.__get_catalog_name("TestTempTable")
        num = len(list(self.connection.tableprivileges(table=table_name)))
        self.assertTrue(num > 0)

if __name__ == "__main__":
    test_env.run_test_cases()
