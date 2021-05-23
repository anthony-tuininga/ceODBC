#------------------------------------------------------------------------------
# test_1400_strings.py
#   Module for testing binding and fetching strings.
#------------------------------------------------------------------------------

import decimal

import ceODBC
import test_env

class TestCase(test_env.BaseTestCase):

    def setUp(self):
        super().setUp()
        self.raw_data = [
                (1, 'String 1', None),
                (2, 'String 2B', 'Nullable One'),
                (3, 'String 3XX', None),
                (4, 'String 4YYY', 'Nullable Two')
        ]
        self.data_by_key = {}
        for data_tuple in self.raw_data:
            key = data_tuple[0]
            self.data_by_key[key] = data_tuple

    def test_1400_bind_string(self):
        "1400 - test binding in a string"
        self.cursor.execute("""
                select * from TestStrings
                where StringCol = ?""",
                "String 3XX")
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[3]])

    def test_1401_bind_string_after_number(self):
        "1401 - test binding in a string after setting input sizes to a number"
        self.cursor.setinputsizes(ceODBC.NUMBER)
        self.cursor.execute("""
                select * from TestStrings
                where StringCol = ?""",
                "String 2B")
        self.assertEqual(self.cursor.fetchall(), [self.data_by_key[2]])

    def test_1402_bind_null(self):
        "1402 - test binding in a null"
        self.cursor.execute("""
                select * from TestStrings
                where StringCol = ?""",
                None)
        self.assertEqual(self.cursor.fetchall(), [])

    def test_1403_bind_long_string_after_setting_size(self):
        "1403 - test that setinputsizes() returns a long variable"
        var = self.cursor.setinputsizes(90000)[0]
        self.assertEqual(var.type, ceODBC.STRING)
        in_string = "1234567890" * 9000
        var.setvalue(0, in_string)
        out_string = var.getvalue()
        self.assertEqual(in_string, out_string,
                "output does not match: in was %d, out was %d" % \
                (len(in_string), len(out_string)))

    def test_1404_CursorDescription(self):
        "1404 - test cursor description is accurate"
        self.cursor.execute("select * from TestStrings")
        dsn_type = test_env.get_dsn_type()
        if dsn_type == "pgsql":
            expected_data = [
                ('intcol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('stringcol', ceODBC.STRING, 20, 20, 0, 0, False),
                ('nullablecol', ceODBC.STRING, 50, 50, 0, 0, True)
            ]
        else:
            expected_data = [
                ('IntCol', ceODBC.NUMBER, 11, 10, 10, 0, False),
                ('StringCol', ceODBC.STRING, 20, 20, 0, 0, False),
                ('NullableCol', ceODBC.STRING, 50, 50, 0, 0, True)
            ]
        self.assertEqual(self.cursor.description, expected_data)

    def test_1405_fetchall(self):
        "1405 - test that fetching all of the data returns the correct results"
        self.cursor.execute("select * From TestStrings order by IntCol")
        self.assertEqual(self.cursor.fetchall(), self.raw_data)
        self.assertEqual(self.cursor.fetchall(), [])

    def test_1406_fetchmany(self):
        "1406 - test that fetching data in chunks returns the correct results"
        self.cursor.execute("select * From TestStrings order by IntCol")
        self.assertEqual(self.cursor.fetchmany(2), self.raw_data[0:2])
        self.assertEqual(self.cursor.fetchmany(1), self.raw_data[2:3])
        self.assertEqual(self.cursor.fetchmany(4), self.raw_data[3:])
        self.assertEqual(self.cursor.fetchmany(3), [])

    def test_1407_fetchone(self):
        "1407 - test that fetching a single row returns the correct results"
        self.cursor.execute("""
                select *
                from TestStrings
                where IntCol in (3, 4)
                order by IntCol""")
        self.assertEqual(self.cursor.fetchone(), self.data_by_key[3])
        self.assertEqual(self.cursor.fetchone(), self.data_by_key[4])
        self.assertEqual(self.cursor.fetchone(), None)

    def test_1408_supplemental_characters(self):
        "1408 - test binding and fetching supplemental characters"
        supplemental_chars = "𠜎 𠜱 𠝹 𠱓 𠱸 𠲖 𠳏 𠳕 𠴕 𠵼 𠵿 𠸎 𠸏 𠹷 𠺝 " \
                "𠺢 𠻗 𠻹 𠻺 𠼭 𠼮 𠽌 𠾴 𠾼 𠿪 𡁜 𡁯 𡁵 𡁶 𡁻 𡃁 𡃉 𡇙 𢃇 " \
                "𢞵 𢫕 𢭃 𢯊 𢱑 𢱕 𢳂 𢴈 𢵌 𢵧 𢺳 𣲷 𤓓 𤶸 𤷪 𥄫 𦉘 𦟌 𦧲 " \
                "𦧺 𧨾 𨅝 𨈇 𨋢 𨳊 𨳍 𨳒 𩶘"
        self.cursor.execute("truncate table TestTempTable")
        self.cursor.execute("""
                insert into TestTempTable (IntCol, StringCol)
                values (?, ?)""",
                (1, supplemental_chars))
        self.connection.commit()
        self.cursor.execute("select StringCol from TestTempTable")
        value, = self.cursor.fetchone()
        self.assertEqual(value, supplemental_chars)

if __name__ == "__main__":
    test_env.run_test_cases()
