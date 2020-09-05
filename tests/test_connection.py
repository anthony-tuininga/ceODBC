#------------------------------------------------------------------------------
# test_connection.pyx
#   Module for testing PostgreSQL connections.
#------------------------------------------------------------------------------

import base

class TestCase(base.BaseTestCase):

    def test_exception_on_close(self):
        "confirm an exception is raised after closing a connection"
        connection = self.getConnection()
        connection.close()
        self.assertRaises(ceODBC.InterfaceError, connection.rollback)


if __name__ == "__main__":
    base.run_test_cases()
