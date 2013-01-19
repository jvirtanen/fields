#!/usr/bin/env python

import fields
import tempfile
import unittest


class TestCase(unittest.TestCase):

    def assertParseEqual(self, text, output):
        encoded = _encode(text)
        self.assertEqual(self.__parse_buffer(encoded), output)
        self.assertEqual(self.__parse_file(encoded), output)

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
            return [_decode(record) for record in reader]
        except fields.Error as e:
            return str(e)


def _decode(record):
    return [field.decode('utf-8') for field in record]

def _encode(text):
    return text.encode('utf-8')


class SettingsTest(TestCase):

    def test_cr_as_delimiter(self):
        self.assertFail('Bad field delimiter', delimiter='\r')

    def test_lf_as_delimiter(self):
        self.assertFail('Bad field delimiter', delimiter='\n')

    def test_cr_as_quote(self):
        self.assertFail('Bad quote character', quotechar='\r')

    def test_lf_as_quote(self):
        self.assertFail('Bad quote character', quotechar='\n')

    def test_equal_delimiter_and_quote(self):
        self.assertFail('Bad quote character', delimiter=',', quotechar=',')

    def assertFail(self, message, **kwargs):
        try:
            fields.reader('', **kwargs)
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
        self.assertParseEqual('a"b,c', '1:2: Unexpected character')

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
        self.assertParseEqual('a"b",c', '1:2: Unexpected character')

    def test_quoted_with_subsequent_garbage(self):
        self.assertParseEqual('"a"b,c', '1:4: Unexpected character')

    def test_quoted_among_non_quoted(self):
        self.assertParseEqual('a,"b",c', [['a', 'b', 'c']])

    def test_tsv(self):
        self.assertParseEqual('a\tb\nc\n', [['a\tb'], ['c']])

    def setUp(self):
        self.settings = {
            'delimiter' : ',',
            'quotechar' : '"'
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

    def test_csv(self):
        self.assertParseEqual('a,b\nc\n', [['a,b'], ['c']])

    def setUp(self):
        self.settings = {
            'delimiter' : '\t',
            'quotechar' : None
        }


class UTF8Test(TestCase):

    def test_one_byte(self):
        self.assertParseEqual(u'a,b', [[u'a', u'b']])

    def test_two_byte(self):
        self.assertParseEqual(u'\u0627,b', [[u'\u0627', u'b']])

    def test_three_byte(self):
        self.assertParseEqual(u'\u0F69,b', [[u'\u0F69', u'b']])

    def test_four_byte(self):
        self.assertParseEqual(u'\U000103A0,b', [[u'\U000103A0', u'b']])

    def test_one_byte_traversal(self):
        self.assertParseEqual(u'a"', '1:2: Unexpected character')

    def test_two_byte_traversal(self):
        self.assertParseEqual(u'\u0627"', '1:2: Unexpected character')

    def test_three_byte_traversal(self):
        self.assertParseEqual(u'\u0F69"', '1:2: Unexpected character')

    def test_four_byte_traversal(self):
        self.assertParseEqual(u'\U000103A0"', '1:2: Unexpected character')

    def setUp(self):
        self.settings = {
            'delimiter': ',',
            'quotechar': '"'
        }


class LimitsWithoutExpansionTest(TestCase):

    def test_full_buffer(self):
        self.assertParseEqual('a' * 1023, [['a' * 1023]])

    def test_maximum_number_of_fields(self):
        self.assertParseEqual(','.join('a' * 16), [['a'] * 16])

    def test_too_big_record(self):
        self.assertParseEqual('a' * 1024, '1:1024: Too big record')

    def test_too_many_fields(self):
        self.assertParseEqual(','.join('a' * 17), '1:32: Too many fields')

    def setUp(self):
        self.settings = {
            '_expand'            : False,
            '_source_buffer_size': 1024,
            '_record_max_fields' : 16,
            '_record_buffer_size': 1024
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
            '_expand'            : True,
            '_source_buffer_size': 1024,
            '_record_max_fields' : 16,
            '_record_buffer_size': 1024
        }


if __name__ == "__main__":
    unittest.main()
