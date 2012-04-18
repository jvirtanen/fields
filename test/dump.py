import subprocess

from tempfile import NamedTemporaryFile


def dump_table(table):
    def handle_record(record, fields):
        def handle_field(column, field):
            return '%d:%d:%d %s' % (record, column, len(field), field)
        return '\n'.join(handle_field(column, field) for (column, field) in
            enumerate(fields))
    return '\n'.join(handle_record(record, fields) for (record, fields) in
        enumerate(table))

def dump_text(text, b=False, d=None, q=None):
    with NamedTemporaryFile() as outfile:
        outfile.write(text)
        outfile.flush()
        return _dump(outfile.name, b, d, q).strip('\n')

def _dump(filename, b=False, d=None, q=None):
    args = ['./dump']
    if b:
        args.append('-b')
    if d:
        args.append('-d%s' % d)
    if q:
        args.append('-q%s' % q)
    args.append(filename)
    return _popen(args)

def _popen(args):
    pipe = subprocess.PIPE
    process = subprocess.Popen(args, stdout=pipe, stderr=pipe)
    stdout, stderr = process.communicate()
    return stderr if len(stderr) > 0 else stdout
