History
=======

0.6.0 (2013-04-09)
  - Improve documentation.
  - Add `fields_reader_position`.
  - Fix handling of CR and LF.
  - Allow `NULL` for default settings.
  - Extract `fields_format`.
  - Increase default source buffer size.
  - Fix `fields_fd_alloc`.

0.5.0 (2012-09-22)
  - Improve documentation.
  - Rename `file_buffer_size` to `source_buffer_size`.
  - Fix handling of CR and LF.
  - Remove escape characters.
  - Add example.
  - Add `make install` target.
  - Improve Python API.
  - Rename Sheets to Fields.

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
