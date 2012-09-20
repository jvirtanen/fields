from . import libfields


class Error(Exception):
    """
    This exception is raised whenever an error is detected.
    """
    pass


def reader(source, **kwargs):
    """
    Return a reader object that reads from `source`. `source` can be either a
    string or a file object. If `source` is a file object, it should be opened
    with the `b` flag.

    Optional keyword arguments can be given to override default formatting
    parameters:

      - `delimiter`: a one-character string used to separate fields. It
        defaults to `,`.

      - `quotechar`: a one-character string used to quote fields containing
        special characters, such as the `delimiter` or the `quotechar`. It
        defaults to `"`.

    The returned object is an iterator. Each iteration returns a record, a
    sequence of fields. Records are implemented as lists of strings.
    """
    return Reader(source, **kwargs)


class Reader(object):

    def __init__(self, source, **kwargs):
        settings = _settings(kwargs)
        try:
            self.__reader = libfields.Reader(source, settings)
            self.__record = libfields.Record(settings)
        except ValueError as e:
            raise Error(str(e))

    def __iter__(self):
        return self

    def next(self):
        result = self.__reader.read(self.__record)
        if result != 0:
            result = self.__reader.error()
            if result:
                raise Error(result)
            else:
                raise StopIteration
        return [self.__record.field(i) for i in xrange(self.__record.size())]


def _settings(kwargs):
        def as_char(value):
            return value or '\0'
        def as_int(value):
            return int(bool(value))
        delimiter = kwargs.get('delimiter', ',')
        quote = kwargs.get('quotechar', '"')
        expand = kwargs.get('_expand', True)
        source_buffer_size = kwargs.get('_source_buffer_size', 4 * 1024)
        record_buffer_size = kwargs.get('_record_buffer_size', 1024 * 1024)
        record_max_fields = kwargs.get('_record_max_fields', 1023)
        return libfields.Settings(
            as_char(delimiter),
            as_char(quote),
            as_int(expand),
            source_buffer_size,
            record_buffer_size,
            record_max_fields
        )
