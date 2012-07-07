Sheets
======

Sheets is a fast C library for reading CSV and other tabular text formats.


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

reader = sheets_read_file(file, &sheets_csv);
record = sheets_record_alloc(&sheets_csv);

struct sheets_field field;

while (sheets_reader_read(reader, record) == 0) {
    for (unsigned i = 0; i < sheets_record_size(record); i++) {
        sheets_record_field(record, i, &field);
        puts(field.value);
    }
}
```

Sheets reads data in ASCII-compatible encoding, such as UTF-8, from a buffer
or file. The record separator may be either CR, LF or CRLF, and the field
delimiter may be any ASCII character except CR or LF.

Sheets supports two methods of embedding field delimiters and record separators
into fields: quoting and escaping. When quoting, a field containing a field
delimiter, record separator or quote character must be enclosed within quote
characters. Additionally, a quote character within a field must be escaped
with another quote character. When escaping, any character preceded by the
escape character is interpreted literally. The quote character or escape
character may be any ASCII character except the field delimiter, CR or LF.

Given the field delimiter is set to `,` and the quote character to `"`
(built-in configuration `sheets_csv`), Sheets reads data that is in compliance
with [RFC 4180][]. Given the field delimiter is set to HT and quoting and
escaping are disabled (built-in configuration `sheets_tsv`), Sheets reads data
that is in compliance with the [text/tab-separated-values][TSV] MIME type.

See `include/sheets.h` for reference documentation.

  [RFC 4180]: http://tools.ietf.org/html/rfc4180
  [TSV]:      http://www.iana.org/assignments/media-types/text/tab-separated-values


Installation
------------

Include `include/sheets.h` and `src/sheets.c` into your application.


History
-------

  - 0.4.0 (2012-07-03)
    - Adds record expansion
    - Adds Python API
    - Adds POSIX API 
    - Adds `sheets_reader_strerror` and `sheets_settings_strerror`
    - Adds `sheets_settings_error`
    - Renames `sheets_error` to `sheets_reader_error`
    - Fixes parsing of quoted field after non-quoted

  - 0.3.3 (2012-06-13)
    - Fixes uninitialized value in `sheets_reader_alloc`

  - 0.3.2 (2012-05-07)
    - Enables `-Wswitch-default` and `-Wswitch-enum` warnings

  - 0.3.1 (2012-04-28)
    - Fixes handling of empty records

  - 0.3.0 (2012-04-21)
    - Adds error constants

  - 0.2.0 (2012-04-18)
    - Adds escaped characters

  - 0.1.0 (2012-04-18)
    - Initial release


License
-------

Sheets is released under the MIT License. See `LICENSE` for details.
