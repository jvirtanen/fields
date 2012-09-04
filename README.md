Fields
======

Fields is a fast C library for reading CSV and other tabular text formats.


Features
--------

  - Quoted fields

  - Escaped characters

  - Simple API

  - Zero dependencies

  - Liberal license


Usage
-----

```c
file = fopen("example.csv", "rb");

reader = fields_read_file(file, &fields_csv);
record = fields_record_alloc(&fields_csv);

struct fields_field field;

while (fields_reader_read(reader, record) == 0) {
    for (unsigned i = 0; i < fields_record_size(record); i++) {
        fields_record_field(record, i, &field);
        puts(field.value);
    }
}
```

Fields reads data in ASCII-compatible encoding, such as UTF-8, from a buffer
or file. The record separator may be either CR, LF or CRLF, and the field
delimiter may be any ASCII character except CR or LF.

Fields supports two methods of embedding field delimiters and record separators
into fields: quoting and escaping. When quoting, a field containing a field
delimiter, record separator or quote character must be enclosed within quote
characters. Additionally, a quote character within a field must be escaped
with another quote character. When escaping, any character preceded by the
escape character is interpreted literally. The quote character or escape
character may be any ASCII character except the field delimiter, CR or LF.

Given the field delimiter is set to `,` and the quote character to `"`
(built-in configuration `fields_csv`), Fields reads data that is in compliance
with [RFC 4180][]. Given the field delimiter is set to HT and quoting and
escaping are disabled (built-in configuration `fields_tsv`), Fields reads data
that is in compliance with the [text/tab-separated-values][TSV] MIME type.

See `include/fields.h` for reference documentation.

  [RFC 4180]: http://tools.ietf.org/html/rfc4180
  [TSV]:      http://www.iana.org/assignments/media-types/text/tab-separated-values


Installation
------------

If you want to use Fields in a C application, you can simply copy the header
and source files into its source tree. On the other hand, the Python binding,
for example, depends on an installation of Fields. 

Install to `/usr/local`:

    make install


History
-------

0.4.0 (2012-07-03)
  - Add record expansion.
  - Add Python API.
  - Add POSIX API.
  - Add `sheets_reader_strerror` and `sheets_settings_strerror`.
  - Add `sheets_settings_error`.
  - Rename `sheets_error` to `sheets_reader_error`.
  - Fix parsing of quoted field after non-quoted.

0.3.3 (2012-06-13)
  - Fix uninitialized value in `sheets_reader_alloc`.

0.3.2 (2012-05-07)
  - Enable `-Wswitch-default` and `-Wswitch-enum` warnings.

0.3.1 (2012-04-28)
  - Fix handling of empty records.

0.3.0 (2012-04-21)
  - Add error constants.

0.2.0 (2012-04-18)
  - Add escaped characters.

0.1.0 (2012-04-18)
  - Initial release.


License
-------

Fields is released under the MIT License. See `LICENSE` for details.
