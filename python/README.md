Fields for Python
=================

Fields for Python is a Python binding to Fields, the fast C library for
reading CSV and other tabular text formats.

Fields for Python is implemented using Python's `ctypes` module. As a
consequence, it depends on an installation of Fields.


Usage
-----

The API is similar to Python's `csv` module:

    >>> import fields
    >>> for record in fields.reader('a,b\nc'):
    ...     print record
    ...
    ...
    ['a', 'b']
    ['c']


License
-------

Fields for Python is released under the MIT License. See `LICENSE` for details.
