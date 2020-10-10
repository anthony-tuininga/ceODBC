#------------------------------------------------------------------------------
# test_1200_cursor.py
#   Module for testing the cursor object.
#------------------------------------------------------------------------------

import base

import ceODBC

class TestCase(base.BaseTestCase):

    def test_1200_callproc_no_args(self):
        "1200 - test executing a stored procedure without any arguments"
        results = self.cursor.callproc("sp_TestNoArgs")
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

if __name__ == "__main__":
    base.run_test_cases()
