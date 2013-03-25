Fields
======

Fields is a fast C library for reading CSV and other tabular text formats.


Usage
-----

Fields reads input consisting of zero or more records, each record consisting
of zero or more fields. Records are separated by record separators and fields
by field delimiters.

Fields handles input internally as UTF-8. The record separator may be either
a carriage return (CR, Mac OS), a line feed (LF, Unix) or a CRLF (Windows).
The field delimiter may be any ASCII character except CR or LF.

A field may begin and end with a quote character. A quoted field may contain
embedded record separators, field delimiters and quote characters. Each quote
character within a quoted field must be escaped with another quote character.
The quote character may be any ASCII character except the field delimiter, CR
or LF. If the quote character is set to the null character (NUL), quoting is
disabled.


Installation
------------

Installing Fields requires a C99 compiler and GNU Make.

Install Fields to `/usr/local`, the default installation location:

    make install

Install Fields to `$HOME`, an alternative installation location:

    make install PREFIX=$HOME


History
-------

See `History.md`.


License
-------

Fields is released under the MIT License. See `LICENSE` for details.
