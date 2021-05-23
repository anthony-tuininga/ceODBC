#------------------------------------------------------------------------------
# test_1500_module.py
#   Module for testing the module constants and constructors.
#------------------------------------------------------------------------------

import datetime
import time

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):
    establish_connection = False

    def test_1500_binary(self):
        "1500 - test ceODBC.Binary"
        expected_value = "unimportant".encode()
        actual_value = ceODBC.Binary(expected_value)
        self.assertEqual(actual_value, expected_value)

    def test_1501_date(self):
        "1501 - test ceODBC.Date"
        expected_value = datetime.date(2020, 10, 9)
        actual_value = ceODBC.Date(2020, 10, 9)
        self.assertEqual(actual_value, expected_value)

    def test_1502_date_from_ticks(self):
        "1502 - test ceODBC.DateFromTicks"
        expected_value = datetime.date(2019, 8, 25)
        ticks = time.mktime((2019, 8, 25, 0, 0, 0, 0, 0, 0))
        actual_value = ceODBC.DateFromTicks(ticks)
        self.assertEqual(actual_value, expected_value)

    def test_1503_time(self):
        "1503 - test ceODBC.Time"
        expected_value = datetime.time(16, 24, 59)
        actual_value = ceODBC.Time(16, 24, 59)
        self.assertEqual(actual_value, expected_value)

    def test_1504_time_from_ticks(self):
        "1504 - test ceODBC.TimeFromTicks"
        expected_value = datetime.time(7, 30, 18)
        ticks = time.mktime((2018, 12, 25, 7, 30, 18, 0, 0, 0))
        actual_value = ceODBC.TimeFromTicks(ticks)
        self.assertEqual(actual_value, expected_value)

    def test_1505_timestamp(self):
        "1505 - test ceODBC.Timestamp"
        expected_value = datetime.datetime(2021, 8, 7, 5, 25, 29)
        actual_value = ceODBC.Timestamp(2021, 8, 7, 5, 25, 29)
        self.assertEqual(actual_value, expected_value)

    def test_1506_timestamp_from_ticks(self):
        "1506 - test ceODBC.TimestampFromTicks"
        expected_value = datetime.datetime(2019, 11, 18, 21, 19, 18)
        ticks = time.mktime((2019, 11, 18, 21, 19, 18, 0, 0, 0))
        actual_value = ceODBC.TimestampFromTicks(ticks)
        self.assertEqual(actual_value, expected_value)

    def test_1507_api_level(self):
        "1507 - test ceODBC.apilevel"
        self.assertEqual(ceODBC.apilevel, '2.0')

    def test_1508_param_style(self):
        "1507 - test ceODBC.paramstyle"
        self.assertEqual(ceODBC.paramstyle, 'qmark')

    def test_1509_thread_safety(self):
        "1509 - test ceODBC.threadsafety"
        self.assertEqual(ceODBC.threadsafety, 2)

    def test_1510_version(self):
        "1510 - test ceODBC.__version__"
        self.assertEqual(ceODBC.__version__, ceODBC.version)

if __name__ == "__main__":
    test_env.run_test_cases()
