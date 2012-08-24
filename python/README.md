Fields.py
=========

Fields.py is a Python binding to Fields, the fast C library for reading CSV
and other tabular text formats.


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


Installation
------------

    pip install .


License
-------

Fields.py is released under the MIT License. See `LICENSE` for details.
