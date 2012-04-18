import unittest

from dump import dump_table, dump_text


class TestCase(unittest.TestCase):

    def assert_equals(self, text, table):
        self.__assert_equals(text, dump_table(table), b=False)
        self.__assert_equals(text, dump_table(table), b=True)

    def assert_error(self, text, message):
        self.__assert_equals(text, message, b=False)
        self.__assert_equals(text, message, b=True)

    def __assert_equals(self, text, expected, b):
        self.assertEquals(self.__dump_text(text, b), expected)

    def __dump_text(self, text, b):
        return dump_text(text, b, self.d, self.q)

    def setUp(self):
        self.d = None
        self.q = None


def main():
    unittest.main()
