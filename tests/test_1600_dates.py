#------------------------------------------------------------------------------
# test_1600_dates.py
#   Module for testing binding and fetching dates.
#------------------------------------------------------------------------------

import datetime

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):

    def setUp(self):
        super().setUp()
        self.raw_data = [
            (1, datetime.date(2020, 2, 8),
                datetime.datetime(2019, 12, 20, 18, 35, 25),
                datetime.date(1969, 7, 29),
                datetime.datetime(1988, 1, 25, 8, 24, 13)),
            (2, datetime.date(1978, 2, 12),
                datetime.datetime(2009, 2, 20, 8, 23, 12),
                None, None),
            (3, datetime.date(2000, 6, 18),
                datetime.datetime(2007, 1, 28, 6, 22, 11),
                datetime.date(1988, 6, 30),
                datetime.datetime(1998, 4, 29, 11, 35, 24)),
            (4, datetime.date(1999, 10, 5),
                datetime.datetime(2009, 2, 19, 0, 1, 2),
                None, None)
        ]
        self.data_by_key = {}
        for data_tuple in self.raw_data:
            key = data_tuple[0]
            self.data_by_key[key] = data_tuple

    def test_1600_bind_date(self):
        "1600 - test binding in a datetime.date"
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""",
                datetime.date(2000, 6, 18))
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[3]])

    def test_1601_bind_datetime(self):
        "1600 - test binding in a datetime.datetime"
        self.cursor.execute("""
                select * from TestDates
                where TimestampCol = ?""",
                datetime.datetime(2009, 2, 20, 8, 23, 12))
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[2]])

    def test_1602_bind_date_after_string(self):
        "1602 - test binding in a date after setting input sizes to a string"
        self.cursor.setinputsizes(str)
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""",
                datetime.date(1999, 10, 5))
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[4]])

    def test_1603_bind_null(self):
        "1603 - test binding in a null"
        self.cursor.execute("""
                select * from TestDates
                where TimestampCol = ?""",
                None)
        self.assertEqual(self.cursor.fetchall(), [])

    def test_1604_CursorDescription(self):
        "1604 - test cursor description is accurate"
        self.cursor.execute("select * from TestDates")
        dsn_type = test_env.get_dsn_type()
        if dsn_type == "pgsql":
            expected_data = [
                ('intcol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('datecol', ceODBC.DATETIME, 10, 10, 0, 0, False),
                ('timestampcol', ceODBC.DATETIME, 26, 26, 0, 0, False),
                ('nullabledatecol', ceODBC.DATETIME, 10, 10, 0, 0, True),
                ('nullabletimestampcol', ceODBC.DATETIME, 26, 26, 0, 0, True)
            ]
        elif dsn_type == "mysql":
            expected_data = [
                ('IntCol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('DateCol', ceODBC.DATETIME, 10, 10, 0, 0, False),
                ('TimestampCol', ceODBC.DATETIME, 19, 19, 0, 0, True),
                ('NullableDateCol', ceODBC.DATETIME, 10, 10, 0, 0, True),
                ('NullableTimestampCol', ceODBC.DATETIME, 19, 19, 0, 0, True)
            ]
        self.assertEqual(self.cursor.description, expected_data)

    def test_1605_fetchall(self):
        "1605 - test that fetching all of the data returns the correct results"
        self.cursor.execute("""
                select * from TestDates
                order by IntCol""")
        self.assertEqual(self.cursor.fetchall(), self.raw_data)
        self.assertEqual(self.cursor.fetchall(), [])

if __name__ == "__main__":
    test_env.run_test_cases()
