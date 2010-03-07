"""Module for testing cursor objects."""

import sys

class TestCursor(BaseTestCase):

    def testExecuteNoArgs(self):
        """test executing a statement without any arguments"""
        self.cursor.execute("select null")
        result, = self.cursor.fetchone()
        self.failUnlessEqual(result, None)

    def testExecuteNoStatementWithArgs(self):
        """test executing a None statement with bind variables"""
        self.failUnlessRaises(ceODBC.ProgrammingError, self.cursor.execute,
                None, 5)

    def testExecuteAndModifyArraySize(self):
        """test executing a statement and then changing the array size"""
        self.cursor.execute("select IntCol from TestNumbers")
        self.cursor.arraysize = 20
        self.failUnlessEqual(len(self.cursor.fetchall()), 4)

    def testExecuteMany(self):
        """test executing a statement multiple times"""
        self.cursor.execute("delete from TestExecuteMany")
        rows = [ [n] for n in range(230) ]
        self.cursor.arraysize = 500
        statement = "insert into TestExecuteMany (IntCol) values (?)"
        self.cursor.executemany(statement, rows)
        self.connection.commit()
        self.cursor.execute("select count(*) from TestExecuteMany")
        count, = self.cursor.fetchone()
        self.failUnlessEqual(count, len(rows))

    def testExecuteManyWithPrepare(self):
        """test executing a statement multiple times (with prepare)"""
        self.cursor.execute("delete from TestExecuteMany")
        rows = [ [n] for n in range(225) ]
        self.cursor.arraysize = 100
        statement = "insert into TestExecuteMany (IntCol) values (?)"
        self.cursor.prepare(statement)
        self.cursor.executemany(None, rows)
        self.connection.commit()
        self.cursor.execute("select count(*) from TestExecuteMany")
        count, = self.cursor.fetchone()
        self.failUnlessEqual(count, len(rows))

    def testExecuteManyWithRebind(self):
        """test executing a statement multiple times (with rebind)"""
        self.cursor.execute("delete from TestExecuteMany")
        rows = [ [n] for n in range(235) ]
        self.cursor.arraysize = 100
        statement = "insert into TestExecuteMany (IntCol) values (?)"
        self.cursor.executemany(statement, rows[:50])
        self.cursor.executemany(statement, rows[50:])
        self.connection.commit()
        self.cursor.execute("select count(*) from TestExecuteMany")
        count, = self.cursor.fetchone()
        self.failUnlessEqual(count, len(rows))

    def testExecuteManyWithResize(self):
        """test executing a statement multiple times (with resize)"""
        self.cursor.execute("delete from TestExecuteMany")
        rows = [ ( 1, "First" ),
                 ( 2, "Second" ),
                 ( 3, "Third" ),
                 ( 4, "Fourth" ),
                 ( 5, "Fifth" ),
                 ( 6, "Sixth" ),
                 ( 7, "Seventh" ) ]
        self.cursor.bindarraysize = 5
        self.cursor.setinputsizes(int, 100)
        sql = "insert into TestExecuteMany (IntCol, StringCol) values (?, ?)"
        self.cursor.executemany(sql, rows)
        self.cursor.execute("select count(*) from TestExecuteMany")
        count, = self.cursor.fetchone()
        self.failUnlessEqual(count, len(rows))

    def testExecuteManyWithExecption(self):
        """test executing a statement multiple times (with exception)"""
        self.cursor.execute("delete from TestExecuteMany")
        rows = [(1,), (2,), (3,), (2,), (5,)]
        statement = "insert into TestExecuteMany (IntCol) values (?)"
        self.failUnlessRaises(ceODBC.DatabaseError, self.cursor.executemany,
                statement, rows)
        self.failUnlessEqual(self.cursor.rowcount, 3)

    def testPrepare(self):
        """test preparing a statement and executing it multiple times"""
        self.failUnlessEqual(self.cursor.statement, None)
        statement = "select ? + 5"
        self.cursor.prepare(statement)
        var = self.cursor.var(ceODBC.NUMBER)
        self.failUnlessEqual(self.cursor.statement, statement)
        var.setvalue(0, 2)
        self.cursor.execute(None, var)
        self.failUnlessEqual(var.getvalue(), 7)
        self.cursor.execute(None, var)
        self.failUnlessEqual(var.getvalue(), 12)
        self.cursor.execute("select ? + 3;", var)
        self.failUnlessEqual(var.getvalue(), 15)

    def testExceptionOnClose(self):
        "confirm an exception is raised after closing a cursor"
        self.cursor.close()
        self.failUnlessRaises(ceODBC.InterfaceError, self.cursor.execute,
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
        self.failUnlessEqual(rows, [1, 2, 3])

    def testIteratorsInterrupted(self):
        """test iterators (with intermediate execute)"""
        self.cursor.execute("delete from TestExecuteMany")
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
        self.cursor.execute("insert into TestExecuteMany (IntCol) values (1)")
        if sys.version_info[0] >= 3:
            self.failUnlessRaises(ceODBC.InterfaceError, next, testIter) 
        else:
            self.failUnlessRaises(ceODBC.InterfaceError, testIter.next) 

    def testBadPrepare(self):
        """test that subsequent executes succeed after bad prepare"""
        self.failUnlessRaises(ceODBC.DatabaseError,
                self.cursor.execute, "select nullx")
        self.cursor.execute("select null")

    def testBadExecute(self):
        """test that subsequent fetches fail after bad execute"""
        self.failUnlessRaises(ceODBC.DatabaseError,
                self.cursor.execute, "select y")
        self.failUnlessRaises(ceODBC.InterfaceError,
                self.cursor.fetchall)

    def testSetInputSizesByPosition(self):
        """test setting input sizes with positional args"""
        var = self.cursor.var(ceODBC.STRING, 100)
        self.cursor.setinputsizes(None, 5, None, 10, None, ceODBC.NUMBER)
        self.cursor.execute("select ? || ? || ? || ? || ?"
                [var, 'test_', 5, '_second_', 3, 7])
        self.failUnlessEqual(var.getvalue(), "test_5_second_37")

