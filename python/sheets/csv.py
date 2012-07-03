from . import libsheets


class Error(Exception):
    pass


class Reader(object):

    def __init__(self, csvfile, **kwargs):
        settings = _settings(kwargs)
        try:
            self.__reader = libsheets.Reader(csvfile, settings)
            self.__record = libsheets.Record(settings)
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
            return value if value else '\0'
        def as_int(value):
            return 1 if value else 0
        delimiter = kwargs.get('delimiter', ',')
        escape = kwargs.get('escapechar')
        quote = kwargs.get('quotechar', '"')
        expand = kwargs.get('_expand', True)
        file_buffer_size = kwargs.get('_file_buffer_size', 4 * 1024)
        record_buffer_size = kwargs.get('_record_buffer_size', 1024 * 1024)
        record_max_fields = kwargs.get('_record_max_fields', 1023)
        return libsheets.Settings(
            as_char(delimiter),
            as_char(escape),
            as_char(quote),
            as_int(expand),
            file_buffer_size,
            record_buffer_size,
            record_max_fields
        )
