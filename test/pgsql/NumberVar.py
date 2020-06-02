"""Module for testing number variables."""

import ceODBC
import decimal

import TestEnv

class TestCase(TestEnv.BaseTestCase):

    def setUp(self):
        TestEnv.BaseTestCase.setUp(self)
        self.rawData = [
                (1, 25, decimal.Decimal("125.25")),
                (2, 1234567890123456, decimal.Decimal("245.37")),
                (3, 9876543210, decimal.Decimal("25.99")),
                (4, 98765432101234, decimal.Decimal("445.79"))
        ]
        self.dataByKey = {}
        for dataTuple in self.rawData:
            key = dataTuple[0]
            self.dataByKey[key] = dataTuple

    def testBindDecimal(self):
        "test binding in a decimal.Decimal"
        self.cursor.execute("""
                select * from TestNumbers
                where DecimalCol - ? - ? = BigIntCol""",
                decimal.Decimal("0.25"),
                decimal.Decimal("100"))
        self.assertEqual(self.cursor.fetchall(), [self.dataByKey[1]])

    def testBindFloat(self):
        "test binding in a float"
        self.cursor.execute("""
                select * from TestNumbers
                where BigIntCol = ? - DecimalCol""", 150.25)
        self.assertEqual(self.cursor.fetchall(), [self.dataByKey[1]])

    def testBindInteger(self):
        "test binding in an integer"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 2)
        self.assertEqual(self.cursor.fetchall(), [self.dataByKey[2]])

    def testBindLargeLong(self):
        "test binding in a large long integer"
        valueVar = self.cursor.var(ceODBC.BigIntegerVar)
        valueVar.setvalue(0, 1234567890123456)
        self.cursor.execute("""
                select count(*)
                from TestNumbers
                where BigIntCol < ?
                limit 1""", valueVar)
        value = valueVar.getvalue()
        self.assertEqual(value, 1234567890123456)

    def testBindIntegerAfterString(self):
        "test binding in an number after setting input sizes to a string"
        self.cursor.setinputsizes(15)
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 3)
        self.assertEqual(self.cursor.fetchall(), [self.dataByKey[3]])

    def testBindNull(self):
        "test binding in a null"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", None)
        self.assertEqual(self.cursor.fetchall(), [])

    def testCursorDescription(self):
        "test cursor description is accurate"
        self.cursor.execute("select * from TestNumbers")
        self.assertEqual(self.cursor.description,
                [ ('intcol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                  ('bigintcol', ceODBC.NUMBER, 20, 19, 19, 0, True),
                  ('decimalcol', ceODBC.NUMBER, 8, 6, 6, 2, True) ])

    def testFetchAll(self):
        "test that fetching all of the data returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.assertEqual(self.cursor.fetchall(), self.rawData)
        self.assertEqual(self.cursor.fetchall(), [])

    def testFetchMany(self):
        "test that fetching data in chunks returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.assertEqual(self.cursor.fetchmany(2), self.rawData[0:2])
        self.assertEqual(self.cursor.fetchmany(1), self.rawData[2:3])
        self.assertEqual(self.cursor.fetchmany(4), self.rawData[3:])
        self.assertEqual(self.cursor.fetchmany(3), [])

    def testFetchOne(self):
        "test that fetching a single row returns the correct results"
        self.cursor.execute("""
                select *
                from TestNumbers
                where IntCol in (2, 3)
                order by IntCol""")
        self.assertEqual(self.cursor.fetchone(), self.dataByKey[2])
        self.assertEqual(self.cursor.fetchone(), self.dataByKey[3])
        self.assertEqual(self.cursor.fetchone(), None)

    def testReturnAsLong(self):
        "test that fetching a long integer returns such in Python"
        self.cursor.execute("""
                select BigIntCol
                from TestNumbers
                where IntCol = 2""")
        col, = self.cursor.fetchone()
        self.assertTrue(isinstance(col, int), "integer not returned")

    def testReturnAsDecimal(self):
        "test that fetching a decimal returns such in Python"
        self.cursor.execute("select 1.25")
        result, = self.cursor.fetchone()
        self.assertEqual(result, decimal.Decimal("1.25"))


if __name__ == "__main__":
    TestEnv.run_test_cases()
