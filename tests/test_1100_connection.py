#------------------------------------------------------------------------------
# test_1100_connection.py
#   Module for testing the connection object.
#------------------------------------------------------------------------------

import test_env

import ceODBC
import threading

class TestCase(test_env.BaseTestCase):
    establish_connection = False

    def __connect_and_drop(self):
        "connect to the database, perform a query and drop the connection"
        connection = test_env.get_connection()
        cursor = connection.cursor()
        cursor.execute("select count(*) from TestNumbers")
        count, = cursor.fetchone()
        self.assertEqual(count, 4)

    def test_1100_exception_on_close(self):
        "1100 - confirm an exception is raised after closing a connection"
        connection = test_env.get_connection()
        connection.close()
        self.assertRaises(ceODBC.InterfaceError, connection.rollback)

    def test_1101_rollback_on_close(self):
        "1101 - connection rolls back before close"
        connection = test_env.get_connection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        cursor.execute("select count(*) from TestTempTable")
        other_connection = test_env.get_connection()
        other_cursor = other_connection.cursor()
        other_cursor.execute("insert into TestTempTable (IntCol) values (1)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (2)")
        other_connection.close()
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def test_1102_rollback_on_del(self):
        "1102 - connection rolls back before destruction"
        connection = test_env.get_connection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        other_connection = test_env.get_connection()
        other_cursor = other_connection.cursor()
        other_cursor.execute("insert into TestTempTable (IntCol) values (1)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (2)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (3)")
        del other_cursor
        del other_connection
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def test_1103_threading(self):
        "1103 - connection to database with multiple threads"
        threads = []
        for i in range(20):
            thread = threading.Thread(None, self.__connect_and_drop)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

if __name__ == "__main__":
    test_env.run_test_cases()
