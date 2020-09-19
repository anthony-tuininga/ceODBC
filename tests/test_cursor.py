#------------------------------------------------------------------------------
# test_cursor.pyx
#   Module for testing the cursor object.
#------------------------------------------------------------------------------

import base

import ceODBC

class TestCase(base.BaseTestCase):

    def test_callproc_no_args(self):
        "test executing a stored procedure without any arguments"
        results = self.cursor.callproc("sp_TestNoArgs")
        self.assertEqual(results, [])

    def test_execute_no_args(self):
        "test executing a statement without any arguments"
        self.cursor.execute("select null")
        result, = self.cursor.fetchone()
        self.assertEqual(result, None)

    def test_execute_no_statement_with_args(self):
        "test executing a None statement with args"
        self.assertRaises(ceODBC.ProgrammingError, self.cursor.execute,
                          None, 5)

    def test_exception_on_close(self):
        "confirm an exception is raised after closing a cursor"
        self.cursor.close()
        self.assertRaises(ceODBC.InterfaceError, self.cursor.execute,
                "select 1")

    def test_iterators(self):
        "test iterators"
        self.cursor.execute("""
                select IntCol
                from TestNumbers
                where IntCol between 1 and 3
                order by IntCol""")
        rows = [v for v, in self.cursor]
        self.assertEqual(rows, [1, 2, 3])

    def test_iterators_interrupted(self):
        "test iterators (with intermediate execute)"
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

    def test_bad_execute(self):
        "test that subsequent fetches fail after bad execute"
        self.assertRaises(ceODBC.DatabaseError, self.cursor.execute,
                          "select y")
        self.assertRaises(ceODBC.InterfaceError, self.cursor.fetchall)

if __name__ == "__main__":
    base.run_test_cases()
