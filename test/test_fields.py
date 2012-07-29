#!/usr/bin/env python

import os
import sys
sys.path.insert(0, os.path.abspath('../python'))

import fields
import tempfile
import unittest


class TestCase(unittest.TestCase):

    def assertParseEqual(self, text, output):
        self.assertEqual(self.__parse_buffer(text), output)
        self.assertEqual(self.__parse_file(text), output)

    def __parse_buffer(self, text):
        return self.__parse(text)

    def __parse_file(self, text):
        with tempfile.TemporaryFile() as outfile:
            outfile.write(text)
            outfile.seek(0)
            return self.__parse(outfile)

    def __parse(self, source):
        reader = fields.reader(source, **self.settings)
        try:
            return [record for record in reader]
        except fields.Error as e:
            return str(e)


class SettingsTest(TestCase):

    def test_cr_as_delimiter(self):
        self.assertFail('Bad field delimiter', delimiter='\r')

    def test_lf_as_delimiter(self):
        self.assertFail('Bad field delimiter', delimiter='\n')

    def test_cr_as_escape(self):
        self.assertFail('Bad escape character', escapechar='\r')

    def test_lf_as_escape(self):
        self.assertFail('Bad escape character', escapechar='\n')

    def test_equal_delimiter_and_escape(self):
        self.assertFail('Bad escape character', delimiter=',', escapechar=',')

    def test_cr_as_quote(self):
        self.assertFail('Bad quote character', quotechar='\r')

    def test_lf_as_quote(self):
        self.assertFail('Bad quote character', quotechar='\n')

    def test_equal_delimiter_and_quote(self):
        self.assertFail('Bad quote character', delimiter=',', quotechar=',')

    def test_simultaneous_escape_and_quote(self):
        self.assertFail('Bad quote character', escapechar='x', quotechar='y')

    def assertFail(self, message, **settings):
        try:
            fields.reader('', **settings)
            self.fail()
        except fields.Error as e:
            self.assertEqual(str(e), message)


class CSVTest(TestCase):

    def test_nul(self):
        self.assertParseEqual('a\x00b\nc', [['a\x00b'], ['c']])

    def test_lf(self):
        self.assertParseEqual('a,b\nc\n', [['a', 'b'], ['c']])

    def test_cr(self):
        self.assertParseEqual('a,b\rc\n', [['a', 'b'], ['c']])

    def test_crlf(self):
        self.assertParseEqual('a,b\r\nc\n', [['a', 'b'], ['c']])

    def test_missing_newline_at_eof(self):
        self.assertParseEqual('a,b\nc', [['a', 'b'], ['c']])

    def test_empty_source(self):
        self.assertParseEqual('', [])

    def test_empty_line(self):
        self.assertParseEqual('\n', [[]])

    def test_empty_record(self):
        self.assertParseEqual('a,b\n\nc', [['a', 'b'], [], ['c']])

    def test_quote(self):
        self.assertParseEqual('a"b,c', 'Unexpected character')

    def test_quoted(self):
        self.assertParseEqual('"a","b"\n"c"\n', [['a', 'b'], ['c']])

    def test_quoted_with_embedded_delimiter(self):
        self.assertParseEqual('"a,b"\nc\n', [['a,b'], ['c']])

    def test_quoted_with_embedded_lf(self):
        self.assertParseEqual('"a\nb"\nc\n', [['a\nb'], ['c']])

    def test_quoted_with_embedded_quote(self):
       self.assertParseEqual('"a""b"\nc\n', [['a"b'], ['c']])

    def test_quoted_with_preceding_whitespace(self):
        self.assertParseEqual('  "a",  "b"\n  "c"\n',
            [['a', 'b'], ['c']])

    def test_quoted_with_subsequent_whitespace(self):
        self.assertParseEqual('"a"  ,"b"  \n"c"  \n',
            [['a', 'b'], ['c']])

    def test_quoted_without_terminating_quote(self):
        self.assertParseEqual('a,b\n"c', [['a', 'b'], ['c']])

    def test_quoted_with_preceding_garbage(self):
        self.assertParseEqual('a"b",c', 'Unexpected character')

    def test_quoted_with_subsequent_garbage(self):
        self.assertParseEqual('"a"b,c', 'Unexpected character')

    def test_quoted_among_non_quoted(self):
        self.assertParseEqual('a,"b",c', [['a', 'b', 'c']])

    def test_tsv(self):
        self.assertParseEqual('a\tb\nc\n', [['a\tb'], ['c']])

    def setUp(self):
        self.settings = {
            'delimiter' : ',',
            'quotechar' : '"',
            'escapechar': None
        }


class TSVTest(TestCase):

    def test_nul(self):
        self.assertParseEqual('a\x00b\nc', [['a\x00b'], ['c']])

    def test_lf(self):
        self.assertParseEqual('a\tb\nc\n', [['a', 'b'], ['c']])

    def test_cr(self):
        self.assertParseEqual('a\tb\rc\n', [['a', 'b'], ['c']])

    def test_crlf(self):
        self.assertParseEqual('a\tb\r\nc\n', [['a', 'b'], ['c']])

    def test_missing_newline_at_eof(self):
        self.assertParseEqual('a\tb\nc', [['a', 'b'], ['c']])

    def test_empty_source(self):
        self.assertParseEqual('', [])

    def test_empty_line(self):
        self.assertParseEqual('\n', [[]])

    def test_empty_record(self):
        self.assertParseEqual('a\tb\n\nc', [['a', 'b'], [], ['c']])

    def test_escaped_delimiter(self):
        self.assertParseEqual('a\\\tb\nc', [['a\tb'], ['c']])

    def test_escaped_lf(self):
        self.assertParseEqual('a\tb\\\nc', [['a', 'b\nc']])

    def test_terminating_escape(self):
        self.assertParseEqual('a\tb\nc\\', [['a', 'b'], ['c']])

    def test_csv(self):
        self.assertParseEqual('a,b\nc\n', [['a,b'], ['c']])

    def setUp(self):
        self.settings = {
            'delimiter' : '\t',
            'quotechar' : None,
            'escapechar': '\\'
        }


class LimitsWithoutExpansionTest(TestCase):

    def test_full_buffer(self):
        self.assertParseEqual('a' * 1023, [['a' * 1023]])

    def test_maximum_number_of_fields(self):
        self.assertParseEqual(','.join('a' * 16), [['a'] * 16])

    def test_too_big_record(self):
        self.assertParseEqual('a' * 1024, 'Too big record')

    def test_too_many_fields(self):
        self.assertParseEqual(','.join('a' * 17), 'Too many fields')

    def setUp(self):
        self.settings = {
            '_expand'               : False,
            '_file_buffer_size'     : 1024,
            '_record_max_fields'    : 16,
            '_record_buffer_size'   : 1024
        }


class LimitsWithExpansionTest(TestCase):

    def test_full_buffer(self):
        self.assertParseEqual('a' * 1023, [['a' * 1023]])

    def test_maximum_number_of_fields(self):
        self.assertParseEqual(','.join('a' * 16), [['a'] * 16])

    def test_buffer_expansion(self):
        self.assertParseEqual('a' * 1024, [['a' * 1024]])

    def test_field_expansion(self):
        self.assertParseEqual(','.join('a' * 17), [['a'] * 17])

    def test_buffer_expansion_at_file_buffer_boundary(self):
        self.assertParseEqual('%s,%s' % ('a' * 1024, 'a'), [['a' * 1024, 'a']])

    def setUp(self):
        self.settings = {
            '_expand'               : True,
            '_file_buffer_size'     : 1024,
            '_record_max_fields'    : 16,
            '_record_buffer_size'   : 1024
        }


if __name__ == "__main__":
    unittest.main()
