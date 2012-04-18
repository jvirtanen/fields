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

See `src/sheets.h` for reference documentation.


Installation
------------

Include `src/sheets.c` and `src/sheets.h` into your application.


History
-------

  - 0.2.0 (2012-04-18)
    - Adds escaped characters

  - 0.1.0 (2012-04-18)
    - Initial release


License
-------

Sheets is released under the MIT License. See `LICENSE` for details.
