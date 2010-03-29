"""Module for testing date/time variables."""

import datetime
import time

class TestDateTimeVar(BaseTestCase):

    def setUp(self):
        BaseTestCase.setUp(self)
        self.rawData = [
                (1, datetime.datetime(2010, 3, 1),
                    datetime.datetime(2010, 3, 2, 8)),
                (2, datetime.datetime(2010, 3, 2, 8),
                    datetime.datetime(2010, 3, 3, 16)),
                (3, datetime.datetime(2010, 3, 3, 16),
                    datetime.datetime(2010, 3, 4, 7, 30)),
                (4, datetime.datetime(2010, 3, 4, 7, 30),
                    datetime.datetime(2010, 3, 5, 9, 28, 16))
        ]
        self.dataByKey = {}
        for dataTuple in self.rawData:
            key = dataTuple[0]
            self.dataByKey[key] = dataTuple

    def testBindDate(self):
        "test binding in a date"
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""",
                ceODBC.Timestamp(2010, 3, 2, 8, 0, 0))
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[2]])

    def testBindDateTime(self):
        "test binding in a native date time"
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""",
                datetime.datetime(2010, 3, 3, 16, 0, 0))
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[3]])

    def testBindDateAfterString(self):
        "test binding in a date after setting input sizes to a string"
        self.cursor.setinputsizes(15)
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""",
                ceODBC.Timestamp(2010, 3, 4, 7, 30, 0))
        self.failUnlessEqual(self.cursor.fetchall(), [self.dataByKey[4]])

    def testBindNull(self):
        "test binding in a null"
        self.cursor.setinputsizes(ceODBC.DATETIME)
        self.cursor.execute("""
                select * from TestDates
                where DateCol = ?""", None)
        self.failUnlessEqual(self.cursor.fetchall(), [])

    def testCursorDescription(self):
        "test cursor description is accurate"
        self.cursor.execute("select * from TestDates")
        self.failUnlessEqual(self.cursor.description,
                [ ('IntCol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                  ('DateCol', ceODBC.DATETIME, 23, 23, 0, 0, False),
                  ('NullableCol', ceODBC.DATETIME, 23, 23, 0, 0, True) ])

    def testFetchAll(self):
        "test that fetching all of the data returns the correct results"
        self.cursor.execute("select * From TestDates order by IntCol")
        self.failUnlessEqual(self.cursor.fetchall(), self.rawData)
        self.failUnlessEqual(self.cursor.fetchall(), [])

    def testFetchMany(self):
        "test that fetching data in chunks returns the correct results"
        self.cursor.execute("select * From TestDates order by IntCol")
        self.failUnlessEqual(self.cursor.fetchmany(2), self.rawData[0:2])
        self.failUnlessEqual(self.cursor.fetchmany(1), self.rawData[2:3])
        self.failUnlessEqual(self.cursor.fetchmany(4), self.rawData[3:])
        self.failUnlessEqual(self.cursor.fetchmany(3), [])

    def testFetchOne(self):
        "test that fetching a single row returns the correct results"
        self.cursor.execute("""
                select *
                from TestDates
                where IntCol in (3, 4)
                order by IntCol""")
        self.failUnlessEqual(self.cursor.fetchone(), self.dataByKey[3])
        self.failUnlessEqual(self.cursor.fetchone(), self.dataByKey[4])
        self.failUnlessEqual(self.cursor.fetchone(), None)

