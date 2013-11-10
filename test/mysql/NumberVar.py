"""Module for testing number variables."""

import ceODBC
import decimal
import sys

class TestNumberVar(BaseTestCase):

    def setUp(self):
        BaseTestCase.setUp(self)
        self.rawData = [
                (1, 25, 5.2, 7.3, decimal.Decimal("125.25")),
                (2, 1234567890123456, 25.1, 17.8, decimal.Decimal("245.37")),
                (3, 9876543210, 37.8, 235.19, decimal.Decimal("25.99")),
                (4, 98765432101234, 77.27, 922.78, decimal.Decimal("445.79"))
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
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[1]])

    def testBindFloat(self):
        "test binding in a float"
        self.cursor.execute("""
                select * from TestNumbers
                where abs(BigIntCol - ? - DoubleCol) < .001""", 17.7)
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[1]])

    def testBindInteger(self):
        "test binding in an integer"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 2)
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[2]])

    def testBindLargeLong(self):
        "test binding in a large long integer"
        valueVar = self.cursor.var(ceODBC.BigIntegerVar)
        valueVar.setvalue(0, 1234567890123456)
        self.cursor.execute("select ?", valueVar)
        value = valueVar.getvalue()
        self.failUnlessEqual(value, 1234567890123456)

    def testBindIntegerAfterString(self):
        "test binding in an number after setting input sizes to a string"
        self.cursor.setinputsizes(15)
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", 3)
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[3]])

    def testBindNull(self):
        "test binding in a null"
        self.cursor.execute("""
                select * from TestNumbers
                where IntCol = ?""", None)
        self.failUnlessEqual(self.cursor.fetchall(), [])

    def testCursorDescription(self):
        "test cursor description is accurate"
        self.cursor.execute("select * from TestNumbers")
        self.failUnlessEqual(self.cursor.description,
                [ ('IntCol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                  ('BigIntCol', ceODBC.NUMBER, 20, 19, 19, 0, True),
                  ('FloatCol', ceODBC.NUMBER, 8, 7, 7, 0, True),
                  ('DoubleCol', ceODBC.NUMBER, 16, 15, 15, 0, True),
                  ('DecimalCol', ceODBC.NUMBER, 8, 6, 6, 2, True) ])

    def testFetchAll(self):
        "test that fetching all of the data returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.failUnlessEqual(self.cursor.fetchall(), self.rawData)
        self.failUnlessEqual(self.cursor.fetchall(), [])

    def testFetchMany(self):
        "test that fetching data in chunks returns the correct results"
        self.cursor.execute("select * From TestNumbers order by IntCol")
        self.failUnlessEqual(self.cursor.fetchmany(2), self.rawData[0:2])
        self.failUnlessEqual(self.cursor.fetchmany(1), self.rawData[2:3])
        self.failUnlessEqual(self.cursor.fetchmany(4), self.rawData[3:])
        self.failUnlessEqual(self.cursor.fetchmany(3), [])

    def testFetchOne(self):
        "test that fetching a single row returns the correct results"
        self.cursor.execute("""
                select *
                from TestNumbers
                where IntCol in (2, 3)
                order by IntCol""")
        self.failUnlessEqual(self.cursor.fetchone(), self.dataByKey[2])
        self.failUnlessEqual(self.cursor.fetchone(), self.dataByKey[3])
        self.failUnlessEqual(self.cursor.fetchone(), None)

    def testReturnAsLong(self):
        "test that fetching a long integer returns such in Python"
        self.cursor.execute("""
                select BigIntCol
                from TestNumbers
                where IntCol = 2""")
        col, = self.cursor.fetchone()
        if sys.version_info[0] < 3:
            intType = long
        else:
            intType = int
        self.failUnless(isinstance(col, intType), "integer not returned")

    def testReturnAsDecimal(self):
        "test that fetching a decimal returns such in Python"
        self.cursor.execute("select 1.25")
        result, = self.cursor.fetchone()
        self.failUnlessEqual(result, decimal.Decimal("1.25"))

