"""Define test environment."""

import ceODBC
import os

def GetValue(name, label):
    value = os.environ.get("CEODBC_" + name)
    if value is None:
        value = raw_input(label + ": ")
    return value

DSN = GetValue("PGSQL_DSN", "DSN")
ARRAY_SIZE = int(GetValue("ARRAY_SIZE", "array size"))

