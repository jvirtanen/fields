Sheets
======

Sheets is a simple and fast C library for reading tabular text formats, such
as CSV and TSV.


Features
--------

  - Quoted fields

  - Escaped characters

  - Simple API

  - Zero dependencies

  - Liberal license


Usage
-----

    file = fopen("example.csv", "rb");

    reader = sheets_read_file(file, &sheets_csv);
    record = sheets_record_alloc(&sheets_csv);

    while (sheets_reader_read(reader, record) == 0) {
        for (unsigned i = 0; i < sheets_record_size(record); i++) {
            sheets_record_field(record, i, &field);
            puts(field.value);
        }
    }

Sheets reads data in ASCII-compatible encoding, such as UTF-8, from a buffer
or a file. The record separator may be either CR (`\r`), LF (`\n`) or CRLF,
and the field delimiter may be any ASCII character except CR or LF.

Sheets supports two ways of embedding field delimiters and record separators
into fields: quoting and escaping. When quoting, a field containing a field
delimiter, record separator or quote character must be enclosed within quote
characters. Additionally, a quote character within a field must be escaped
with another quote character. When escaping, any character preceded by the
escape character is interpreted literally. The quote character or escape
character may be any ASCII character except the field delimiter, CR or LF.

Given the field delimiter is set to `,` and the quote character to `"`, Sheets
reads data that is in compliance with [RFC 4180][]. Given the field delimiter
is set to HT (`\t`) and quoting and escaping are disabled, Sheets reads data
that is in compliance with the [text/tab-separated-values][TSV] MIME type.

See `src/sheets.h` for reference documentation.

  [RFC 4180]: http://tools.ietf.org/html/rfc4180
  [TSV]:      http://www.iana.org/assignments/media-types/text/tab-separated-values


Installation
------------

Include `src/sheets.c` and `src/sheets.h` into your application.


History
-------

  - 0.3.0 (2012-04-21)
    - Adds error constants

  - 0.2.0 (2012-04-18)
    - Adds escaped characters

  - 0.1.0 (2012-04-18)
    - Initial release


License
-------

Sheets is released under the MIT License. See `LICENSE` for details.
