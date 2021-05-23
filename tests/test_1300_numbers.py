#------------------------------------------------------------------------------
# test_1300_numbers.py
#   Module for testing binding and fetching numbers.
#------------------------------------------------------------------------------

import decimal

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):

    def setUp(self):
        super().setUp()
        self.raw_data = [
                (1, 25, 5.2, 7.3, decimal.Decimal("125.25")),
                (2, 1234567890123456, 25.1, 17.8, decimal.Decimal("245.37")),
                (3, 9876543210, 37.8, 235.19, decimal.Decimal("25.99")),
                (4, 98765432101234, 77.27, 922.78, decimal.Decimal("445.79"))
        ]
        self.data_by_key = {}
        for data_tuple in self.raw_data:
            key = data_tuple[0]
            self.data_by_key[key] = data_tuple

    def test_1300_bind_decimal(self):
        "1300 - test binding in a decimal.Decimal"
        self.cursor.execute("""
                select * from TestNumbers
                where DecimalCol - ? - ? = BigIntCol""",
                decimal.Decimal("0.25"),
                decimal.Decimal("100"))
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[1]])

    def test_1301_bind_float(self):
        "1301 - test binding in a float"
        self.cursor.execute("""
                select * from TestNumbers
                where BigIntCol = ? - DecimalCol""", 150.25)
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[1]])

    def test_1302_bind_integer(self):
        "1302 - test binding in an integer"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 2)
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[2]])

    def test_1303_bind_large_int(self):
        "1303 - test binding in a large long integer"
        value_var = self.cursor.var(ceODBC.DB_TYPE_BIGINT)
        value_var.setvalue(0, 1234567890123456)
        self.cursor.execute("""
                select count(*)
                from TestNumbers
                where BigIntCol < ?
                limit 1""", value_var)
        value = value_var.getvalue()
        self.assertEqual(value, 1234567890123456)

    def test_1304_bind_integer_after_string(self):
        "1304 - test binding a number after setting input sizes to a string"
        self.cursor.setinputsizes(15)
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 3)
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[3]])

    def test_1305_bind_null(self):
        "1305 - test binding in a null"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", None)
        self.assertEqual(self.cursor.fetchall(), [])

    def test_1306_cursor_description(self):
        "1306 - test cursor description is accurate"
        self.cursor.execute("select * from TestNumbers")
        dsn_type = test_env.get_dsn_type()
        if dsn_type == "pgsql":
            expected_data = [
                ('intcol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('bigintcol', ceODBC.NUMBER, 20, 19, 19, 0, True),
                ('floatcol', ceODBC.NUMBER, 18, 17, 17, 0, True),
                ('doublecol', ceODBC.NUMBER, 18, 17, 17, 0, True),
                ('decimalcol', ceODBC.NUMBER, 8, 6, 6, 2, True)
            ]
        else:
            expected_data = [
                ('IntCol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('BigIntCol', ceODBC.NUMBER, 20, 19, 19, 0, True),
                ('FloatCol', ceODBC.NUMBER, 8, 7, 7, 0, True),
                ('DoubleCol', ceODBC.NUMBER, 16, 15, 15, 0, True),
                ('DecimalCol', ceODBC.NUMBER, 8, 6, 6, 2, True)
            ]
        self.assertEqual(self.cursor.description, expected_data)

    def test_1307_fetchall(self):
        "1307 - test that fetching all of the data returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.assertEqual(self.cursor.fetchall(), self.raw_data)
        self.assertEqual(self.cursor.fetchall(), [])

    def test_1308_fetchmany(self):
        "1308 - test that fetching data in chunks returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.assertEqual(self.cursor.fetchmany(2), self.raw_data[0:2])
        self.assertEqual(self.cursor.fetchmany(1), self.raw_data[2:3])
        self.assertEqual(self.cursor.fetchmany(4), self.raw_data[3:])
        self.assertEqual(self.cursor.fetchmany(3), [])

    def test_1309_fetchone(self):
        "1309 - test that fetching a single row returns the correct results"
        self.cursor.execute("""
                select *
                from TestNumbers
                where IntCol in (2, 3)
                order by IntCol""")
        self.assertEqual(self.cursor.fetchone(), self.data_by_key[2])
        self.assertEqual(self.cursor.fetchone(), self.data_by_key[3])
        self.assertEqual(self.cursor.fetchone(), None)

    def test_1310_return_as_long(self):
        "1310 - test that fetching a long integer returns such in Python"
        self.cursor.execute("""
                select BigIntCol
                from TestNumbers
                where IntCol = 2""")
        col, = self.cursor.fetchone()
        self.assertTrue(isinstance(col, int), "integer not returned")

    def test_1311_return_as_decimal(self):
        "1311 - test that fetching a decimal returns such in Python"
        self.cursor.execute("select 1.25")
        result, = self.cursor.fetchone()
        self.assertEqual(result, decimal.Decimal("1.25"))

if __name__ == "__main__":
    test_env.run_test_cases()
