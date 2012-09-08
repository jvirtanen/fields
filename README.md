Fields
======

Fields is a fast C library for reading CSV and other tabular text formats.


Usage
-----

Fields reads records from a source, such as a file or a buffer. Each record
contains zero or more fields. Records are separated by record separators and
fields by field delimiters.

Data read from the source must be in ASCII-compatible encoding, such as UTF-8.
The record separator may be either a carriage return (CR, Mac OS), a line feed
(LF, Unix) or a CRLF (Windows). The field delimiter can be set to any character
except CR or LF.

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
