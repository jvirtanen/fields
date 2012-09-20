import ctypes


try:
    so = ctypes.CDLL('libfields.so')
except OSError:
    so = ctypes.CDLL('libfields.dylib')


class Field(ctypes.Structure):
    _fields_ = [
        ('value',   ctypes.POINTER(ctypes.c_char)),
        ('length',  ctypes.c_size_t)
    ]

Field_p = ctypes.POINTER(Field)


class Reader(object):

    def __init__(self, source, settings):
        try:
            self.source = source
            self.c = so.fields_read_fd(self.source.fileno(), settings)
        except AttributeError:
            self.source = str(source)
            self.c = so.fields_read_buffer(self.source, len(self.source),
                settings)
        if not self.c:
            result = so.fields_settings_error(settings)
            if result != 0:
                raise ValueError(so.fields_settings_strerror(result))
            raise MemoryError

    def __del__(self):
        if self.c:
            so.fields_reader_free(self.c)

    def read(self, record):
        return so.fields_reader_read(self.c, record.c)

    def error(self):
        result = so.fields_reader_error(self.c)
        return so.fields_reader_strerror(result) if result != 0 else None


Reader_p = ctypes.c_void_p


class Record(object):

    def __init__(self, settings):
        self.c = so.fields_record_alloc(settings)
        if not self.c:
            raise MemoryError

    def __del__(self):
        if self.c:
            so.fields_record_free(self.c)

    def field(self, index):
        field = Field()
        result = so.fields_record_field(self.c, index, ctypes.byref(field))
        if result != 0:
            raise IndexError
        return ctypes.string_at(field.value, field.length)

    def size(self):
        return so.fields_record_size(self.c)


Record_p = ctypes.c_void_p


class Settings(ctypes.Structure):
    _fields_ = [
        ('delimiter',           ctypes.c_char),
        ('quote',               ctypes.c_char),
        ('expand',              ctypes.c_int),
        ('source_buffer_size',  ctypes.c_size_t),
        ('record_buffer_size',  ctypes.c_size_t),
        ('record_max_fields',   ctypes.c_size_t)
    ]

Settings_p = ctypes.POINTER(Settings)


so.fields_read_buffer.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_size_t,
    Settings_p
]
so.fields_read_buffer.restype = Reader_p

so.fields_read_fd.argtypes = [ ctypes.c_int, Settings_p ]
so.fields_read_fd.restype = Reader_p

so.fields_reader_free.argtypes = [ Reader_p ]
so.fields_reader_free.restype = None

so.fields_reader_read.argtypes = [ Reader_p, Record_p ]
so.fields_reader_read.restype = ctypes.c_int

so.fields_reader_error.argtypes = [ Reader_p ]
so.fields_reader_error.restype = ctypes.c_int

so.fields_reader_strerror.argtypes = [ ctypes.c_int ]
so.fields_reader_strerror.restype = ctypes.c_char_p

so.fields_record_alloc.argtypes = [ Settings_p ]
so.fields_record_alloc.restype = Record_p

so.fields_record_free.argtypes = [ Record_p ]
so.fields_record_free.restype = None

so.fields_record_field.argtypes = [ Record_p, ctypes.c_uint, Field_p ]
so.fields_record_field.restype = ctypes.c_int

so.fields_record_size.argtypes = [ Record_p ]
so.fields_record_size.restype = ctypes.c_size_t

so.fields_settings_error.argtypes = [ Settings_p ]
so.fields_settings_error.restype = ctypes.c_int

so.fields_settings_strerror.argtypes = [ ctypes.c_int ]
so.fields_settings_strerror.restype = ctypes.c_char_p
