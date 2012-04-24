#!/usr/bin/env python

from support import TestCase, main


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
        self.assert_equals('\n', [['']])

    def test_quote(self):
        self.assert_error('a"b,c', 'sheets_reader_error: %d' %
            self.SHEETS_ERROR_UNEXPECTED_CHARACTER)

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
        self.assert_error('a"b",c', 'sheets_reader_error: %d' %
            self.SHEETS_ERROR_UNEXPECTED_CHARACTER)

    def test_quoted_with_subsequent_garbage(self):
        self.assert_error('"a"b,c', 'sheets_reader_error: %d' %
            self.SHEETS_ERROR_UNEXPECTED_CHARACTER)

    def test_tsv(self):
        self.assert_equals('a\tb\nc\n', [['a\tb'], ['c']])

    def test_maximum_fields(self):
        self.assert_equals(','.join('a' * 15), [['a'] * 15])

    def test_too_many_fields(self):
        self.assert_error(','.join('a' * 16), 'sheets_reader_error: %d' %
            self.SHEETS_ERROR_TOO_MANY_FIELDS)

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
        self.assert_equals('\n', [['']])

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
        self.assert_error('\t'.join('a' * 16), 'sheets_reader_error: %d' %
            self.SHEETS_ERROR_TOO_MANY_FIELDS)

    def setUp(self):
        self.d = '\t'
        self.e = '\\'
        self.q = None


if __name__ == "__main__":
    main()
