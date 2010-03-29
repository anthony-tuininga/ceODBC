"""Module for testing methods which accept Unicode parameters in addition
   to strings and is only relevant for Python 2.x."""

import threading

class TestConnection(BaseTestCase):

    def testBindUnicode(self):
        """test binding a Unicode value to a function"""
        result = self.cursor.callfunc("ufn_StringLength", int, (u"freddie",))
        self.failUnlessEqual(result, 7)

    def testConnect(self):
        "test connecting with a Unicode connect string"
        self.connection.close()
        dsn = unicode(self.connection.dsn)
        connection = ceODBC.Connection(dsn)

