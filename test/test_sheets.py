#!/usr/bin/env python

import dump
import unittest


class TestCase(unittest.TestCase):

    def assert_equals(self, text, table):
        self.__assert_equals(text, dump.dump_table(table), b=False)
        self.__assert_equals(text, dump.dump_table(table), b=True)

    def assert_error(self, text, message):
        self.__assert_equals(text, message, b=False)
        self.__assert_equals(text, message, b=True)

    def __assert_equals(self, text, expected, b):
        self.assertEquals(self.__dump_text(text, b), expected)

    def __dump_text(self, text, b):
        return dump.dump_text(text, b, self.d, self.e, self.q)


class CSVTest(TestCase):

    def test_lf(self):
        self.assert_equals('a,b\nc\n', [['a', 'b'], ['c']])

    def test_cr(self):
        self.assert_equals('a,b\rc\n', [['a', 'b'], ['c']])

    def test_crlf(self):
        self.assert_equals('a,b\r\nc\n', [['a', 'b'], ['c']])

    def test_missing_newline_at_eof(self):
        self.assert_equals('a,b\nc', [['a', 'b'], ['c']])

    def test_empty_source(self):
        self.assert_equals('', [])

    def test_empty_line(self):
        self.assert_equals('\n', [])

    def test_empty_record(self):
        self.assert_equals('a,b\n\nc', [['a', 'b'], [], ['c']])

    def test_quote(self):
        self.assert_error('a"b,c', 'sheets_reader_error: Unexpected character')

    def test_quoted(self):
        self.assert_equals('"a","b"\n"c"\n', [['a', 'b'], ['c']])

    def test_quoted_with_embedded_delimiter(self):
        self.assert_equals('"a,b"\nc\n', [['a,b'], ['c']])

    def test_quoted_with_embedded_lf(self):
        self.assert_equals('"a\nb"\nc\n', [['a\nb'], ['c']])

    def test_quoted_with_embedded_quote(self):
        self.assert_equals('"a""b"\nc\n', [['a"b'], ['c']])

    def test_quoted_with_preceding_whitespace(self):
        self.assert_equals('  "a",  "b"\n  "c"\n',
            [['a', 'b'], ['c']])

    def test_quoted_with_subsequent_whitespace(self):
        self.assert_equals('"a"  ,"b"  \n"c"  \n',
            [['a', 'b'], ['c']])

    def test_quoted_without_terminating_quote(self):
        self.assert_equals('a,b\n"c', [['a', 'b'], ['c']])

    def test_quoted_with_preceding_garbage(self):
        self.assert_error('a"b",c',
            'sheets_reader_error: Unexpected character')

    def test_quoted_with_subsequent_garbage(self):
        self.assert_error('"a"b,c',
            'sheets_reader_error: Unexpected character')

    def test_quoted_among_non_quoted(self):
        self.assert_equals('a,"b",c', [['a', 'b', 'c']])

    def test_tsv(self):
        self.assert_equals('a\tb\nc\n', [['a\tb'], ['c']])

    def test_maximum_fields(self):
        self.assert_equals(','.join('a' * 15), [['a'] * 15])

    def test_too_many_fields(self):
        self.assert_error(','.join('a' * 16),
            'sheets_reader_error: Too many fields')

    def setUp(self):
        self.d = ','
        self.q = '"'
        self.e = None


class TSVTest(TestCase):

    def test_lf(self):
        self.assert_equals('a\tb\nc\n', [['a', 'b'], ['c']])

    def test_cr(self):
        self.assert_equals('a\tb\rc\n', [['a', 'b'], ['c']])

    def test_crlf(self):
        self.assert_equals('a\tb\r\nc\n', [['a', 'b'], ['c']])

    def test_missing_newline_at_eof(self):
        self.assert_equals('a\tb\nc', [['a', 'b'], ['c']])

    def test_empty_source(self):
        self.assert_equals('', [])

    def test_empty_line(self):
        self.assert_equals('\n', [])

    def test_empty_record(self):
        self.assert_equals('a\tb\n\nc', [['a', 'b'], [], ['c']])

    def test_escaped_delimiter(self):
        self.assert_equals('a\\\tb\nc', [['a\tb'], ['c']])

    def test_escaped_lf(self):
        self.assert_equals('a\tb\\\nc', [['a', 'b\nc']])

    def test_terminating_escape(self):
        self.assert_equals('a\tb\nc\\', [['a', 'b'], ['c']])

    def test_csv(self):
        self.assert_equals('a,b\nc\n', [['a,b'], ['c']])

    def test_maximum_fields(self):
        self.assert_equals('\t'.join('a' * 15), [['a'] * 15])

    def test_too_many_fields(self):
        self.assert_error('\t'.join('a' * 16),
            'sheets_reader_error: Too many fields')

    def setUp(self):
        self.d = '\t'
        self.e = '\\'
        self.q = None


if __name__ == "__main__":
    unittest.main()
