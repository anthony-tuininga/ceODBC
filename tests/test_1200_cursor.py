#------------------------------------------------------------------------------
# test_1200_cursor.py
#   Module for testing the cursor object.
#------------------------------------------------------------------------------

import unittest

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):

    def test_1200_callproc_no_args(self):
        "1200 - test executing a stored procedure without any arguments"
        with self.connection.cursor() as cursor:
            results = cursor.callproc("sp_TestNoArgs")
        self.assertEqual(results, [])

    def test_1201_execute_no_args(self):
        "1201 - test executing a statement without any arguments"
        self.cursor.execute("select null")
        result, = self.cursor.fetchone()
        self.assertEqual(result, None)

    def test_1202_execute_no_statement_with_args(self):
        "1202 - test executing a None statement with args"
        self.assertRaises(ceODBC.ProgrammingError, self.cursor.execute,
                          None, 5)

    def test_1203_exception_on_close(self):
        "1203 - confirm an exception is raised after closing a cursor"
        self.cursor.close()
        self.assertRaises(ceODBC.InterfaceError, self.cursor.execute,
                "select 1")

    def test_1204_iterators(self):
        "1204 - test iterators"
        self.cursor.execute("""
                select IntCol
                from TestNumbers
                where IntCol between 1 and 3
                order by IntCol""")
        rows = [v for v, in self.cursor]
        self.assertEqual(rows, [1, 2, 3])

    def test_1205_iterators_interrupted(self):
        "1205 - test iterators (with intermediate execute)"
        self.cursor.execute("delete from TestTempTable")
        self.cursor.execute("""
                select IntCol
                from TestNumbers
                where IntCol between 1 and 3
                order by IntCol""")
        testIter = iter(self.cursor)
        value, = next(testIter)
        self.cursor.execute("insert into TestTempTable (IntCol) values (1)")
        self.assertRaises(ceODBC.InterfaceError, next, testIter)

    def test_1206_bad_execute(self):
        "1206 - test that subsequent fetches fail after bad execute"
        self.assertRaises(ceODBC.DatabaseError, self.cursor.execute,
                          "select y")
        self.assertRaises(ceODBC.InterfaceError, self.cursor.fetchall)

    def test_1207_executemany(self):
        "1207 - test executing a statement multiple times"
        self.cursor.execute("delete from TestTempTable")
        rows = [ [n] for n in range(230) ]
        self.cursor.arraysize = 500
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.cursor.executemany(statement, rows)
        self.connection.commit()
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    def test_1208_executemany_with_prepare(self):
        "1208 - test executing a statement multiple times (with prepare)"
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

    def test_1209_executemany_with_rebind(self):
        "1209 - test executing a statement multiple times (with rebind)"
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

    def test_1210_executemany_with_resize(self):
        "1210 - test executing a statement multiple times (with resize)"
        self.cursor.execute("delete from TestTempTable")
        rows = [ ( 1, "First" ),
                 ( 2, "Second" ),
                 ( 3, "Third" ),
                 ( 4, "Fourth" ),
                 ( 5, "Fifth" ),
                 ( 6, "Sixth" ),
                 ( 7, "Seventh" ) ]
        self.cursor.setinputsizes(int, 100)
        sql = "insert into TestTempTable (IntCol, StringCol) values (?, ?)"
        self.cursor.executemany(sql, rows)
        self.cursor.execute("select count(*) from TestTempTable")
        count, = self.cursor.fetchone()
        self.assertEqual(count, len(rows))

    @unittest.skipIf(test_env.get_dsn_type() == "mysql",
                     "MySQL doesn't generate an exception")
    def test_1211_executemany_with_execption(self):
        "1211 - test executing a statement multiple times (with exception)"
        with self.connection.cursor() as cursor:
            cursor.execute("delete from TestTempTable")
        self.connection.commit()
        rows = [(1,), (2,), (3,), (2,), (5,)]
        statement = "insert into TestTempTable (IntCol) values (?)"
        self.assertRaises(ceODBC.DatabaseError, self.cursor.executemany,
                          statement, rows)

    def test_1212_prepare(self):
        "1212 - test preparing a statement and executing it multiple times"
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

    def test_1213_bad_prepare(self):
        "1213 - test that subsequent executes succeed after bad prepare"
        self.assertRaises(ceODBC.DatabaseError,
                self.cursor.execute, "select nullx")
        self.cursor.execute("select null")

if __name__ == "__main__":
    test_env.run_test_cases()
