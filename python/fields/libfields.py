import ctypes
import os


try:
    _so = ctypes.CDLL('libfields.so')
except OSError:
    _so = ctypes.CDLL('libfields.dylib')


class Field(ctypes.Structure):
    _fields_ = [
        ('value', ctypes.POINTER(ctypes.c_char)),
        ('length', ctypes.c_size_t)
    ]

Field_p = ctypes.POINTER(Field)


class Position(ctypes.Structure):
    _fields_ = [
        ('row', ctypes.c_long),
        ('column', ctypes.c_long)
    ]

Position_p = ctypes.POINTER(Position)


class Reader(object):

    def __init__(self, source, fmt, settings):
        try:
            self.source = source
            fd = self.source.fileno()
            if gzip(fd):
                self.ptr = _so.fields_read_gzip(fd, fmt, settings)
            else:
                self.ptr = _so.fields_read_fd(fd, fmt, settings)
        except AttributeError:
            self.source = str(source)
            self.ptr = _so.fields_read_buffer(self.source, len(self.source),
                fmt, settings)
        if not self.ptr:
            message = format_strerror(fmt)
            if message:
                raise ValueError(message)
            message = settings_strerror(settings)
            if message:
                raise ValueError(message)
            raise MemoryError

    def __del__(self):
        if self.ptr:
            _so.fields_reader_free(self.ptr)

    def read(self, record):
        return _so.fields_reader_read(self.ptr, record.ptr)

    def error(self):
        message = self.strerror()
        return '%s: %s' % (self.position(), message) if message else None

    def position(self):
        position = Position()
        _so.fields_reader_position(self.ptr, position)
        return '%d:%d' % (position.row, position.column)

    def strerror(self):
        result = _so.fields_reader_error(self.ptr)
        return _so.fields_reader_strerror(result) if result else None


Reader_p = ctypes.c_void_p


class Record(object):

    def __init__(self, settings):
        self.ptr = _so.fields_record_alloc(settings)
        if not self.ptr:
            raise MemoryError

    def __del__(self):
        if self.ptr:
            _so.fields_record_free(self.ptr)

    def field(self, index):
        field = Field()
        result = _so.fields_record_field(self.ptr, index, ctypes.byref(field))
        if result != 0:
            raise IndexError
        return ctypes.string_at(field.value, field.length)

    def size(self):
        return _so.fields_record_size(self.ptr)


Record_p = ctypes.c_void_p


class Format(ctypes.Structure):
    _fields_ = [
        ('delimiter', ctypes.c_char),
        ('quote', ctypes.c_char)
    ]

Format_p = ctypes.POINTER(Format)


class Settings(ctypes.Structure):
    _fields_ = [
        ('expand', ctypes.c_int),
        ('source_buffer_size', ctypes.c_size_t),
        ('record_buffer_size', ctypes.c_size_t),
        ('record_max_fields', ctypes.c_size_t)
    ]

Settings_p = ctypes.POINTER(Settings)


def gzip(fd):
    pos = os.lseek(fd, 0, os.SEEK_CUR)
    os.lseek(fd, 0, os.SEEK_SET)
    result = os.read(fd, 2) == '\x1f\x8b' # RFC 1952
    os.lseek(fd, pos, os.SEEK_SET)
    return result


def format_strerror(fmt):
    result = _so.fields_format_error(fmt)
    return _so.fields_format_strerror(result) if result else None

def settings_strerror(settings):
    result = _so.settings_strerror(settings)
    return _so.fields_settings_strerror(result) if result else None


_so.fields_read_buffer.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_size_t,
    Format_p,
    Settings_p
]
_so.fields_read_buffer.restype = Reader_p

_so.fields_read_fd.argtypes = [ ctypes.c_int, Format_p, Settings_p ]
_so.fields_read_fd.restype = Reader_p

_so.fields_read_gzip.argtypes = [ ctypes.c_int, Format_p, Settings_p ]
_so.fields_read_gzip.restype = Reader_p

_so.fields_reader_free.argtypes = [ Reader_p ]
_so.fields_reader_free.restype = None

_so.fields_reader_read.argtypes = [ Reader_p, Record_p ]
_so.fields_reader_read.restype = ctypes.c_int

_so.fields_reader_position.argtypes = [ Reader_p, Position_p ]
_so.fields_reader_position.restype = None

_so.fields_reader_error.argtypes = [ Reader_p ]
_so.fields_reader_error.restype = ctypes.c_int

_so.fields_reader_strerror.argtypes = [ ctypes.c_int ]
_so.fields_reader_strerror.restype = ctypes.c_char_p

_so.fields_record_alloc.argtypes = [ Settings_p ]
_so.fields_record_alloc.restype = Record_p

_so.fields_record_free.argtypes = [ Record_p ]
_so.fields_record_free.restype = None

_so.fields_record_field.argtypes = [ Record_p, ctypes.c_uint, Field_p ]
_so.fields_record_field.restype = ctypes.c_int

_so.fields_record_size.argtypes = [ Record_p ]
_so.fields_record_size.restype = ctypes.c_size_t

_so.fields_format_error.argtypes = [ Format_p ]
_so.fields_format_error.restype = ctypes.c_int

_so.fields_format_strerror.argtypes = [ ctypes.c_int ]
_so.fields_format_strerror.restype = ctypes.c_char_p

_so.fields_settings_error.argtypes = [ Settings_p ]
_so.fields_settings_error.restype = ctypes.c_int

_so.fields_settings_strerror.argtypes = [ ctypes.c_int ]
_so.fields_settings_strerror.restype = ctypes.c_char_p
