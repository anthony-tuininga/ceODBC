#------------------------------------------------------------------------------
# test_connection.pyx
#   Module for testing the connection object.
#------------------------------------------------------------------------------

import base

import ceODBC
import threading

class TestCase(base.BaseTestCase):
    establish_connection = False

    def _connect_and_drop(self):
        "connect to the database, perform a query and drop the connection"
        connection = base.get_connection()
        cursor = connection.cursor()
        cursor.execute("select count(*) from TestNumbers")
        count, = cursor.fetchone()
        self.assertEqual(count, 4)

    def test_exception_on_close(self):
        "confirm an exception is raised after closing a connection"
        connection = base.get_connection()
        connection.close()
        self.assertRaises(ceODBC.InterfaceError, connection.rollback)

    def test_rollback_on_close(self):
        "connection rolls back before close"
        connection = base.get_connection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        cursor.execute("select count(*) from TestTempTable")
        other_connection = base.get_connection()
        other_cursor = other_connection.cursor()
        other_cursor.execute("insert into TestTempTable (IntCol) values (1)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (2)")
        other_connection.close()
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def test_rollback_on_del(self):
        "connection rolls back before destruction"
        connection = base.get_connection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        other_connection = base.get_connection()
        other_cursor = other_connection.cursor()
        other_cursor.execute("insert into TestTempTable (IntCol) values (1)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (2)")
        other_cursor.execute("insert into TestTempTable (IntCol) values (3)")
        del other_cursor
        del other_connection
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def test_threading(self):
        "connection to database with multiple threads"
        threads = []
        for i in range(20):
            thread = threading.Thread(None, self._connect_and_drop)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

if __name__ == "__main__":
    base.run_test_cases()
