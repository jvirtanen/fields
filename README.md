Fields
======

Fields is a fast C library for reading CSV and other tabular text formats.


Usage
-----

Fields reads records from a source, such as a file or a buffer. Each record
contains zero or more fields. Records are separated by record separators and
fields by field delimiters.

Data read from the source must be in ASCII or UTF-8. The record separator may
be either a carriage return (CR, Mac OS), a line feed (LF, Unix) or a CRLF
(Windows). The field delimiter can be set to any ASCII character except CR or
LF.

Field delimiters and record separators can be embedded into quoted fields. A
quoted field starts and ends with the quote character, and quote characters
within quoted fields must be doubled. The quote character can be set to any
character except the field delimiter, CR or LF. If the quote character is set
to the null character (NUL), quoting is disabled.

Given the field delimiter is set to `,` and the quote character to `"` (the
built-in configuration `fields_csv`), Fields reads data that is in compliance
with [RFC 4180][]. Given the field delimiter is set to the tab character (HT)
and quoting is disabled (the built-in configuration `fields_tsv`), Fields reads
data that is in compliance with the [text/tab-separated-values][TSV] MIME type.

  [RFC 4180]: http://tools.ietf.org/html/rfc4180
  [TSV]:      http://www.iana.org/assignments/media-types/text/tab-separated-values

See the header files in the `include` directory for reference documentation.


Installation
------------

    make install

The default installation location is `/usr/local`. Override it with `$PREFIX`:

    PREFIX=$HOME make install


History
-------

See `History.md`.


License
-------

Fields is released under the MIT License. See `LICENSE` for details.
