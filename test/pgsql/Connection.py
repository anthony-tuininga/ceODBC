"""Module for testing connections."""

import threading

class TestConnection(TestCase):

    def setUp(self):
        self.dsn = DSN

    def __ConnectAndDrop(self):
        """Connect to the database, perform a query and drop the connection."""
        connection = ceODBC.connect(self.dsn)
        cursor = connection.cursor()
        cursor.execute("select count(*) from TestNumbers")
        count, = cursor.fetchone()
        self.failUnlessEqual(count, 4)

    def testExceptionOnClose(self):
        "confirm an exception is raised after closing a connection"
        connection = ceODBC.connect(self.dsn)
        connection.close()
        self.failUnlessRaises(ceODBC.InterfaceError, connection.rollback)

    def testRollbackOnClose(self):
        "connection rolls back before close"
        connection = ceODBC.connect(self.dsn)
        cursor = connection.cursor()
        cursor.execute("delete from TestExecuteMany")
        connection.commit()
        cursor.execute("select count(*) from TestExecuteMany")
        otherConnection = ceODBC.connect(self.dsn)
        otherCursor = otherConnection.cursor()
        otherCursor.execute("insert into TestExecuteMany (IntCol) values (1)")
        otherCursor.execute("insert into TestExecuteMany (IntCol) values (2)")
        otherConnection.close()
        cursor.execute("select count(*) from TestExecuteMany")
        count, = cursor.fetchone()
        self.failUnlessEqual(count, 0)

    def testRollbackOnDel(self):
        "connection rolls back before destruction"
        connection = ceODBC.connect(self.dsn)
        cursor = connection.cursor()
        cursor.execute("delete from TestExecuteMany")
        connection.commit()
        otherConnection = ceODBC.connect(self.dsn)
        otherCursor = otherConnection.cursor()
        otherCursor.execute("insert into TestExecuteMany (IntCol) values (1)")
        otherCursor.execute("insert into TestExecuteMany (IntCol) values (2)")
        otherCursor.execute("insert into TestExecuteMany (IntCol) values (3)")
        del otherCursor
        del otherConnection
        cursor.execute("select count(*) from TestExecuteMany")
        count, = cursor.fetchone()
        self.failUnlessEqual(count, 0)

    def testThreading(self):
        "connection to database with multiple threads"
        threads = []
        for i in range(20):
            thread = threading.Thread(None, self.__ConnectAndDrop)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

