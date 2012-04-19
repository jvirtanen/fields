#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sheets.h"

static void iterate_buffer_source(const char *);
static void iterate_file_source(const char *);
static void iterate_buffer(const char *, size_t);
static void iterate_file(FILE *);
static void iterate(struct sheets_reader *);
static FILE *fopen_or_die(const char *, const char *);
static void fclose_or_die(FILE *);
static void fseek_or_die(FILE *, long, int);
static void usage(void);
static void die(const char *, ...);

struct sheets_settings settings = {
    .delimiter = '\t',
    .escape = '\0',
    .quote = '\0',
    .file_buffer_size = 4096,
    .record_buffer_size = 1024,
    .record_max_fields = 15
};

int
main(int argc, char *argv[])
{
    int ch;

    bool buffer = false;

    opterr = 0;

    while ((ch = getopt(argc, argv, "bd:e:f:q:r:")) != -1) {
        switch (ch) {
        case 'b':
            buffer = true;
            break;
        case 'd':
            settings.delimiter = optarg[0];
            break;
        case 'e':
            settings.escape = optarg[0];
            break;
        case 'f':
            settings.record_max_fields = atoi(optarg);
            break;
        case 'q':
            settings.quote = optarg[0];
            break;
        case 'r':
            settings.record_buffer_size = atoi(optarg);
            break;
        case '?':
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1)
        usage();

    if (buffer)
        iterate_buffer_source(argv[0]);
    else
        iterate_file_source(argv[0]);

    return EXIT_SUCCESS;
}

static void
iterate_buffer_source(const char *filename)
{
    FILE *  file;
    long    position;
    char *  buffer;
    size_t  buffer_size;

    file = fopen_or_die(filename, "rb");

    fseek_or_die(file, 0, SEEK_END);

    position = ftell(file);
    if (position < 0)
        die("ftell");

    buffer_size = position;

    fseek_or_die(file, 0, SEEK_SET);

    buffer = malloc(buffer_size);
    if (buffer == NULL)
        die("malloc");

    if (fread(buffer, 1, buffer_size, file) != buffer_size)
        die("fread");

    iterate_buffer(buffer, buffer_size);

    fclose_or_die(file);
}

static void
iterate_file_source(const char *filename)
{
    FILE *file;

    file = fopen_or_die(filename, "rb");

    iterate_file(file);

    fclose_or_die(file);
}

static void
iterate_buffer(const char *buffer, size_t buffer_size)
{
    struct sheets_reader *reader;

    reader = sheets_read_buffer(buffer, buffer_size, &settings);
    if (reader == NULL)
        die("sheets_read_buffer");

    iterate(reader);

    sheets_reader_free(reader);
}

static void
iterate_file(FILE *file)
{
    struct sheets_reader *reader;

    reader = sheets_read_file(file, &settings);
    if (reader == NULL)
        die("sheets_read_file");

    iterate(reader);

    sheets_reader_free(reader);
}

static void
iterate(struct sheets_reader *reader)
{
    struct sheets_record *record;
    int error;

    record = sheets_record_alloc(&settings);
    if (record == NULL)
        die("sheets_record_alloc");

    unsigned int row = 0;
    while (sheets_reader_read(reader, record) == 0) {
        size_t columns = sheets_record_size(record);

        for (unsigned int column = 0; column < columns; column++) {
            struct sheets_field field;

            if (sheets_record_field(record, column, &field) != 0)
                die("sheets_record_field");

            printf("%u:%u:%zu %s\n", row, column, field.length, field.value);
        }
        row++;
    }

    error = sheets_reader_error(reader);
    if (error != 0)
        die("sheets_reader_error: %d", error);

    sheets_record_free(record);
}

static FILE *
fopen_or_die(const char *filename, const char *mode)
{
    FILE *file;

    file = fopen(filename, mode);
    if (file == NULL)
        die("fopen");

    return file;
}

static void
fclose_or_die(FILE *file)
{
    if (fclose(file) != 0)
        die("fclose");
}

static void
fseek_or_die(FILE *file, long position, int whence)
{
    if (fseek(file, position, whence) != 0)
        die("fseek");
}

static void
usage(void)
{
    fprintf(stderr, "Usage: dump [OPTIONS] FILE\n");
    exit(EXIT_FAILURE);
}

static void
die(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);

    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}
