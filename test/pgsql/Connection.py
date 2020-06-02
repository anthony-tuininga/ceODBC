"""Module for testing connections."""

import ceODBC
import threading

import TestEnv

class TestCase(TestEnv.BaseTestCase):

    def __ConnectAndDrop(self):
        """Connect to the database, perform a query and drop the connection."""
        connection = self.getConnection()
        cursor = connection.cursor()
        cursor.execute("select count(*) from TestNumbers")
        count, = cursor.fetchone()
        self.assertEqual(count, 4)

    def testExceptionOnClose(self):
        "confirm an exception is raised after closing a connection"
        connection = self.getConnection()
        connection.close()
        self.assertRaises(ceODBC.InterfaceError, connection.rollback)

    def testRollbackOnClose(self):
        "connection rolls back before close"
        connection = self.getConnection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        cursor.execute("select count(*) from TestTempTable")
        otherConnection = self.getConnection()
        otherCursor = otherConnection.cursor()
        otherCursor.execute("insert into TestTempTable (IntCol) values (1)")
        otherCursor.execute("insert into TestTempTable (IntCol) values (2)")
        otherConnection.close()
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def testRollbackOnDel(self):
        "connection rolls back before destruction"
        connection = self.getConnection()
        cursor = connection.cursor()
        cursor.execute("delete from TestTempTable")
        connection.commit()
        otherConnection = self.getConnection()
        otherCursor = otherConnection.cursor()
        otherCursor.execute("insert into TestTempTable (IntCol) values (1)")
        otherCursor.execute("insert into TestTempTable (IntCol) values (2)")
        otherCursor.execute("insert into TestTempTable (IntCol) values (3)")
        del otherCursor
        del otherConnection
        cursor.execute("select count(*) from TestTempTable")
        count, = cursor.fetchone()
        self.assertEqual(count, 0)

    def testThreading(self):
        "connection to database with multiple threads"
        threads = []
        for i in range(20):
            thread = threading.Thread(None, self.__ConnectAndDrop)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()


if __name__ == "__main__":
    TestEnv.run_test_cases()
