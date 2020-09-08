#------------------------------------------------------------------------------
# test_cursor.pyx
#   Module for testing the cursor object.
#------------------------------------------------------------------------------

import base

import ceODBC

class TestCase(base.BaseTestCase):

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

if __name__ == "__main__":
    base.run_test_cases()
