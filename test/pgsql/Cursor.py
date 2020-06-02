"""Module for testing cursor objects."""

import ceODBC
import sys

import TestEnv

class TestCase(TestEnv.BaseTestCase):

    def testCallProcNoArgs(self):
        """test executing a stored procedure without any arguments"""
        results = self.cursor.callproc("sp_TestNoArgs")
        self.assertEqual(results, [])

    def testExecuteNoArgs(self):
        """test executing a statement without any arguments"""
        self.cursor.execute("select null")
        result, = self.cursor.fetchone()
        self.assertEqual(result, None)

    def testExecuteNoStatementWithArgs(self):
        """test executing a None statement with bind variables"""
        self.assertRaises(ceODBC.ProgrammingError, self.cursor.execute,
                None, 5)

    def testExecuteAndModifyArraySize(self):
        """test executing a statement and then changing the array size"""
        self.cursor.execute("select IntCol from TestNumbers")
        self.cursor.arraysize = 20
        self.assertEqual(len(self.cursor.fetchall()), 4)

    def testExecuteMany(self):
        """test executing a statement multiple times"""
        self.cursor.execute("delete from TestTempTable")
        rows = [ [n] for n in range(230) ]
        self.cursor.arraysize = 500
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.cursor.executemany(statement, rows)
        self.connection.commit()
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    def testExecuteManyWithPrepare(self):
        """test executing a statement multiple times (with prepare)"""
        self.cursor.execute("delete from TestTempTable")
        rows = [ [n] for n in range(225) ]
        self.cursor.arraysize = 100
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.cursor.prepare(statement)
        self.cursor.executemany(None, rows)
        self.connection.commit()
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    def testExecuteManyWithRebind(self):
        """test executing a statement multiple times (with rebind)"""
        self.cursor.execute("delete from TestTempTable")
        rows = [ [n] for n in range(235) ]
        self.cursor.arraysize = 100
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.cursor.executemany(statement, rows[:50])
        self.cursor.executemany(statement, rows[50:])
        self.connection.commit()
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    def testExecuteManyWithResize(self):
        """test executing a statement multiple times (with resize)"""
        self.cursor.execute("delete from TestTempTable")
        rows = [ ( 1, "First" ),
                 ( 2, "Second" ),
                 ( 3, "Third" ),
                 ( 4, "Fourth" ),
                 ( 5, "Fifth" ),
                 ( 6, "Sixth" ),
                 ( 7, "Seventh" ) ]
        self.cursor.bindarraysize = 5
        self.cursor.setinputsizes(int, 100)
        sql = "insert into TestTempTable (IntCol, StringCol) values (?, ?)"
        self.cursor.executemany(sql, rows)
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    def testExecuteManyWithExecption(self):
        """test executing a statement multiple times (with exception)"""
        cursor = self.connection.cursor()
        cursor.execute("delete from TestTempTable")
        self.connection.commit()
        rows = [(1,), (2,), (3,), (2,), (5,)]
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.assertRaises(ceODBC.DatabaseError, self.cursor.executemany,
                statement, rows)

    def testPrepare(self):
        """test preparing a statement and executing it multiple times"""
        self.assertEqual(self.cursor.statement, None)
        statement = "select ? + 5"
        self.cursor.prepare(statement)
        self.assertEqual(self.cursor.statement, statement)
        self.cursor.execute(None, 2)
        result, = self.cursor.fetchone()
        self.assertEqual(result, 7)
        self.cursor.execute(None, 7)
        result, = self.cursor.fetchone()
        self.assertEqual(result, 12)
        self.cursor.execute("select ? + 3;", 12)
        result, = self.cursor.fetchone()
        self.assertEqual(result, 15)

    def testExceptionOnClose(self):
        "confirm an exception is raised after closing a cursor"
        self.cursor.close()
        self.assertRaises(ceODBC.InterfaceError, self.cursor.execute,
                "select 1")

    def testIterators(self):
        """test iterators"""
        self.cursor.execute("""
                select IntCol
                from TestNumbers
                where IntCol between 1 and 3
                order by IntCol""")
        rows = []
        for row in self.cursor:
            rows.append(row[0])
        self.assertEqual(rows, [1, 2, 3])

    def testIteratorsInterrupted(self):
        """test iterators (with intermediate execute)"""
        self.cursor.execute("delete from TestTempTable")
        self.cursor.execute("""
                select IntCol
                from TestNumbers
                where IntCol between 1 and 3
                order by IntCol""")
        testIter = iter(self.cursor)
        if sys.version_info[0] >= 3:
            value, = next(testIter)
        else:
            value, = testIter.next()
        self.cursor.execute("insert into TestTempTable (IntCol) values (1)")
        if sys.version_info[0] >= 3:
            self.assertRaises(ceODBC.InterfaceError, next, testIter) 
        else:
            self.assertRaises(ceODBC.InterfaceError, testIter.next) 

    def testBadPrepare(self):
        """test that subsequent executes succeed after bad prepare"""
        self.assertRaises(ceODBC.DatabaseError,
                self.cursor.execute, "select nullx")
        self.cursor.execute("select null")

    def testBadExecute(self):
        """test that subsequent fetches fail after bad execute"""
        self.assertRaises(ceODBC.DatabaseError,
                self.cursor.execute, "select y")
        self.assertRaises(ceODBC.InterfaceError,
                self.cursor.fetchall)


if __name__ == "__main__":
    TestEnv.run_test_cases()
