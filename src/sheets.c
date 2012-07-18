/*
 * Copyright (c) 2012 Jussi Virtanen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "sheets.h"

#define SHEETS_FAILURE (-1)

typedef int sheets_parse_fn(struct sheets_reader *, struct sheets_record *);

struct sheets_reader
{
    void *                  source;
    sheets_source_read_fn * source_read;
    sheets_source_free_fn * source_free;
    char                    delimiter;
    char                    escape;
    char                    quote;
    sheets_parse_fn *       parse;
    const char *            buffer;
    size_t                  buffer_size;
    const char *            cursor;
    char                    skip;
    int                     error;
};

static const char *sheets_reader_end(const struct sheets_reader *);
static int sheets_reader_fill(struct sheets_reader *);
static void sheets_reader_skip(struct sheets_reader *);

struct sheets_record
{
    char *  buffer;
    size_t  buffer_size;
    char ** fields;
    size_t  num_fields;
    size_t  max_fields;
    bool    expand;
};

static const char *sheets_record_end(const struct sheets_record *);
static void sheets_record_init(struct sheets_record *);
static char *sheets_record_expand(struct sheets_record *, char *);
static int sheets_record_push(struct sheets_record *, char *);
static char *sheets_record_pop(struct sheets_record *);
static void sheets_record_finish(struct sheets_record *, char *);
static void sheets_record_normalize(struct sheets_record *);

static sheets_parse_fn *sheets_settings_parser(const struct sheets_settings *);

static int sheets_parse_unquoted(struct sheets_reader *,
    struct sheets_record *);
static int sheets_parse_quoted(struct sheets_reader *, struct sheets_record *);
static int sheets_parse_crlf(struct sheets_reader *, struct sheets_record *,
    const char *, char *);
static int sheets_parse_fail(struct sheets_reader *, struct sheets_record *,
    enum sheets_reader_error);
static int sheets_parse_start(struct sheets_reader *, struct sheets_record *);
static int sheets_parse_finish(struct sheets_reader *, struct sheets_record *,
    const char *, char *);

struct sheets_buffer {
    const char *buffer;
    size_t      buffer_size;
};

static struct sheets_buffer *sheets_buffer_alloc(const char *, size_t);
static int sheets_buffer_read(void *, const char **, size_t *);
static void sheets_buffer_free(void *);

struct sheets_file {
    FILE *  file;
    char *  buffer;
    size_t  buffer_size;
};

static struct sheets_file *sheets_file_alloc(FILE *, size_t);
static int sheets_file_read(void *, const char **, size_t *);
static void sheets_file_free(void *);

enum sheets_state {
    SHEETS_STATE_MAYBE_INSIDE_FIELD,
    SHEETS_STATE_INSIDE_FIELD,
    SHEETS_STATE_INSIDE_QUOTED_FIELD,
    SHEETS_STATE_MAYBE_BEYOND_QUOTED_FIELD,
    SHEETS_STATE_BEYOND_QUOTED_FIELD
};

static inline bool sheets_crlf(char);
static inline bool sheets_whitespace(char);

/*
 * Readers
 * =======
 */

struct sheets_reader *
sheets_read_buffer(const char *buffer, size_t buffer_size,
    const struct sheets_settings *settings)
{
    struct sheets_reader *reader;
    struct sheets_buffer *source;

    source = sheets_buffer_alloc(buffer, buffer_size);
    if (source == NULL)
        return NULL;

    reader = sheets_reader_alloc(source, &sheets_buffer_read,
        &sheets_buffer_free, settings);
    if (reader == NULL) {
        sheets_buffer_free(source);
        return NULL;
    }

    return reader;
}

struct sheets_reader *
sheets_read_file(FILE *file, const struct sheets_settings *settings)
{
    struct sheets_reader *reader;
    struct sheets_file *source;

    source = sheets_file_alloc(file, settings->file_buffer_size);
    if (source == NULL)
        return NULL;

    reader = sheets_reader_alloc(source, &sheets_file_read, &sheets_file_free,
        settings);
    if (reader == NULL) {
        sheets_file_free(source);
        return NULL;
    }

    return reader;
}

struct sheets_reader *
sheets_reader_alloc(void *source, sheets_source_read_fn *read_fn,
    sheets_source_free_fn *free_fn, const struct sheets_settings *settings)
{
    struct sheets_reader *self;

    if (sheets_settings_error(settings) != 0)
        return NULL;

    self = malloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    self->source = source;
    self->source_read = read_fn;
    self->source_free = free_fn;
    self->delimiter = settings->delimiter;
    self->escape = settings->escape;
    self->quote = settings->quote;
    self->parse = sheets_settings_parser(settings);
    self->buffer = NULL;
    self->buffer_size = 0;
    self->cursor = NULL;
    self->skip = '\0';
    self->error = 0;

    return self;
}

void
sheets_reader_free(struct sheets_reader *self)
{
    self->source_free(self->source);

    free(self);
}

static const char *
sheets_reader_end(const struct sheets_reader *self)
{
    return self->buffer + self->buffer_size;
}

static int
sheets_reader_fill(struct sheets_reader *self)
{
    int result;

    result = self->source_read(self->source, &self->buffer, &self->buffer_size);
    self->cursor = self->buffer;

    if (result != 0) {
        self->error = SHEETS_READER_ERROR_UNREADABLE_SOURCE;
        return SHEETS_FAILURE;
    }

    return 0;
}

static void
sheets_reader_skip(struct sheets_reader *self)
{
    if (self->skip == '\0')
        return;

    if (self->cursor == sheets_reader_end(self))
        return;

    if (self->skip != *self->cursor)
        return;

    self->cursor++;
    self->skip = '\0';
}

int
sheets_reader_read(struct sheets_reader *self, struct sheets_record *record)
{
    if (sheets_parse_start(self, record) != 0)
        return SHEETS_FAILURE;

    sheets_record_init(record);

    return self->parse(self, record);
}

int
sheets_reader_error(const struct sheets_reader *self)
{
    return self->error;
}

const char *
sheets_reader_strerror(int error)
{
    switch (error) {
    case SHEETS_READER_ERROR_TOO_BIG_RECORD:
        return "Too big record";
    case SHEETS_READER_ERROR_TOO_MANY_FIELDS:
        return "Too many fields";
    case SHEETS_READER_ERROR_UNEXPECTED_CHARACTER:
        return "Unexpected character";
    case SHEETS_READER_ERROR_UNREADABLE_SOURCE:
        return "Unreadable source";
    case 0:
        return "";
    default:
        break;
    }

    return "Unknown error";
}

/*
 * Records
 * =======
 */

struct sheets_record *
sheets_record_alloc(const struct sheets_settings *settings)
{
    struct sheets_record *self;
    char *buffer;
    size_t buffer_size;
    char **fields;
    size_t max_fields;

    buffer_size = settings->record_buffer_size;
    max_fields = settings->record_max_fields;

    buffer = malloc(buffer_size);
    if (buffer == NULL)
        return NULL;

    /*
     * `buffer` stores the fields separated by single `NUL` characters. Note
     * that also the fields themselves may contain `NUL` characters.
     *
     * `fields` stores a pointer to the beginning of each field. The pointer
     * to the beginning of field `n + 1` is used for calculating the length
     * of field `n`. The last field is no exception: the value at the index
     * `num_fields` stores a pointer pointing to where the next field would
     * start. Hence the size of `fields` is `max_fields + 1`.
     */
    fields = malloc((max_fields + 1) * sizeof(char *));
    if (fields == NULL) {
        free(buffer);
        return NULL;
    }

    self = malloc(sizeof(*self));
    if (self == NULL) {
        free(buffer);
        free(fields);
        return NULL;
    }

    self->buffer = buffer;
    self->buffer_size = buffer_size;
    self->fields = fields;
    self->num_fields = 0;
    self->max_fields = max_fields;
    self->expand = settings->expand;

    return self;
}

void
sheets_record_free(struct sheets_record *self)
{
    free(self->buffer);
    free(self->fields);
    free(self);
}

int
sheets_record_field(const struct sheets_record *self, unsigned int index,
    struct sheets_field *field)
{
    if (index >= self->num_fields)
        return SHEETS_FAILURE;

    field->value = self->fields[index];
    field->length = self->fields[index + 1] - self->fields[index] - 1;
    return 0;
}

size_t
sheets_record_size(const struct sheets_record *self)
{
    return self->num_fields;
}

static const char *
sheets_record_end(const struct sheets_record *self)
{
    return self->buffer + self->buffer_size;
}

static void
sheets_record_init(struct sheets_record *self)
{
    self->num_fields = 0;
}

static char *
sheets_record_expand(struct sheets_record *self, char *cursor)
{
    char *      buffer;
    size_t      buffer_size;
    unsigned    i;
    size_t      offset;

    if (!self->expand)
        return NULL;

    offset = cursor - self->buffer;

    buffer_size = self->buffer_size * 2;

    buffer = realloc(self->buffer, buffer_size);
    if (buffer == NULL)
        return NULL;

    for (i = 0; i < self->num_fields; i++)
        self->fields[i] = self->fields[i] - self->buffer + buffer;

    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self->buffer + offset;
}

static int
sheets_record_push(struct sheets_record *self, char *cursor)
{
    if (self->num_fields == self->max_fields) {
        size_t  max_fields;
        char ** fields;

        if (!self->expand)
            return SHEETS_FAILURE;

        max_fields = self->max_fields * 2;

        fields = realloc(self->fields, (max_fields + 1) * sizeof(char *));
        if (fields == NULL)
            return SHEETS_FAILURE;

        self->fields = fields;
        self->max_fields = max_fields;
    }

    self->fields[self->num_fields++] = cursor;
    return 0;
}

static char *
sheets_record_pop(struct sheets_record *self)
{
    if (self->num_fields == 0)
        return self->buffer;

    self->num_fields--;

    return self->fields[self->num_fields];
}

static void
sheets_record_finish(struct sheets_record *self, char *cursor)
{
    self->fields[self->num_fields] = cursor;

    sheets_record_normalize(self);
}

static void
sheets_record_normalize(struct sheets_record *self)
{
    if (sheets_record_size(self) == 1) {
        struct sheets_field field;

        sheets_record_field(self, 0, &field);

        if (field.length == 0)
            sheets_record_pop(self);
    }
}

/*
 * Settings
 * ========
 */

const struct sheets_settings sheets_csv =
{
    .delimiter = ',',
    .escape = '\0',
    .quote = '"',
    .expand = true,
    .file_buffer_size = SHEETS_DEFAULT_FILE_BUFFER_SIZE,
    .record_buffer_size = SHEETS_DEFAULT_RECORD_BUFFER_SIZE,
    .record_max_fields = SHEETS_DEFAULT_RECORD_MAX_FIELDS
};

const struct sheets_settings sheets_tsv =
{
    .delimiter = '\t',
    .escape = '\0',
    .quote = '\0',
    .expand = true,
    .file_buffer_size = SHEETS_DEFAULT_FILE_BUFFER_SIZE,
    .record_buffer_size = SHEETS_DEFAULT_RECORD_BUFFER_SIZE,
    .record_max_fields = SHEETS_DEFAULT_RECORD_MAX_FIELDS
};

int
sheets_settings_error(const struct sheets_settings *settings)
{
    if (settings->delimiter == '\n')
        return SHEETS_SETTINGS_ERROR_DELIMITER;

    if (settings->delimiter == '\r')
        return SHEETS_SETTINGS_ERROR_DELIMITER;

    if (settings->escape == '\n')
        return SHEETS_SETTINGS_ERROR_ESCAPE;

    if (settings->escape == '\r')
        return SHEETS_SETTINGS_ERROR_ESCAPE;

    if (settings->escape == settings->delimiter)
        return SHEETS_SETTINGS_ERROR_ESCAPE;

    if (settings->quote == '\n')
        return SHEETS_SETTINGS_ERROR_QUOTE;

    if (settings->quote == '\r')
        return SHEETS_SETTINGS_ERROR_QUOTE;

    if (settings->quote == settings->delimiter)
        return SHEETS_SETTINGS_ERROR_QUOTE;

    if ((settings->quote != '\0') && (settings->escape != '\0'))
        return SHEETS_SETTINGS_ERROR_QUOTE;

    if (settings->file_buffer_size < SHEETS_MINIMUM_FILE_BUFFER_SIZE)
        return SHEETS_SETTINGS_ERROR_FILE_BUFFER_SIZE;

    if (settings->record_buffer_size < SHEETS_MINIMUM_RECORD_BUFFER_SIZE)
        return SHEETS_SETTINGS_ERROR_RECORD_BUFFER_SIZE;

    if (settings->record_max_fields < SHEETS_MINIMUM_RECORD_MAX_FIELDS)
        return SHEETS_SETTINGS_ERROR_RECORD_MAX_FIELDS;

    return 0;
}

const char *
sheets_settings_strerror(int error)
{
    switch (error) {
    case SHEETS_SETTINGS_ERROR_DELIMITER:
        return "Bad field delimiter";
    case SHEETS_SETTINGS_ERROR_ESCAPE:
        return "Bad escape character";
    case SHEETS_SETTINGS_ERROR_QUOTE:
        return "Bad quote character";
    case SHEETS_SETTINGS_ERROR_FILE_BUFFER_SIZE:
        return "Too low file buffer size";
    case SHEETS_SETTINGS_ERROR_RECORD_BUFFER_SIZE:
        return "Too low record buffer size";
    case SHEETS_SETTINGS_ERROR_RECORD_MAX_FIELDS:
        return "Too low maximum for fields in record";
    case 0:
        return "";
    default:
        break;
    }

    return "Unknown error";
}

static sheets_parse_fn *
sheets_settings_parser(const struct sheets_settings *settings)
{
    if (settings->quote != '\0')
        return &sheets_parse_quoted;
    else
        return &sheets_parse_unquoted;
}

/*
 * Parsers
 * =======
 */

static int
sheets_parse_unquoted(struct sheets_reader *reader, struct sheets_record *record)
{
    char delimiter;
    char escape;

    bool escaped;

    const char *rp;
    const char *rq;

    char *wp;
    const char *wq;

    delimiter = reader->delimiter;
    escape = reader->escape;

    escaped = false;

    rp = reader->cursor;
    rq = sheets_reader_end(reader);

    wp = record->buffer;
    wq = sheets_record_end(record);

    sheets_record_push(record, wp);

    while (true) {
        while ((rp != rq) && (wp != wq)) {
            if (escaped) {
                escaped = false;
                *wp++ = *rp++;
            }
            else if ((*rp == escape) && (escape != '\0')) {
                rp++;
                escaped = true;
            }
            else if (sheets_crlf(*rp))
                return sheets_parse_crlf(reader, record, rp, wp);
            else if (*rp == delimiter) {
                *wp++ = '\0';
                rp++;
                if (sheets_record_push(record, wp) != 0)
                    return sheets_parse_fail(reader, record,
                        SHEETS_READER_ERROR_TOO_MANY_FIELDS);
            }
            else
                *wp++ = *rp++;
        }

        if (wp == wq) {
            wp = sheets_record_expand(record, wp);
            if (wp == NULL)
                return sheets_parse_fail(reader, record,
                    SHEETS_READER_ERROR_TOO_BIG_RECORD);

            wq = sheets_record_end(record);
        }

        if (rp == rq) {
            if (sheets_reader_fill(reader) != 0)
                return sheets_parse_fail(reader, record,
                    SHEETS_READER_ERROR_UNREADABLE_SOURCE);

            rp = reader->cursor;
            rq = sheets_reader_end(reader);
        }

        if (rp == rq) {
            *wp++ = '\0';
            return sheets_parse_finish(reader, record, rp, wp);
        }
    }

    return sheets_parse_fail(reader, record, SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
}

static int
sheets_parse_quoted(struct sheets_reader *reader, struct sheets_record *record)
{
    enum sheets_state state;

    char delimiter;
    char quote;

    const char *rp;
    const char *rq;

    char *wp;
    const char *wq;

    state = SHEETS_STATE_MAYBE_INSIDE_FIELD;

    delimiter = reader->delimiter;
    quote = reader->quote;

    rp = reader->cursor;
    rq = sheets_reader_end(reader);

    wp = record->buffer;
    wq = sheets_record_end(record);

    sheets_record_push(record, wp);

    while (true) {
        while ((rp != rq) && (wp != wq)) {
            switch (state) {
            case SHEETS_STATE_MAYBE_INSIDE_FIELD:
                if (*rp == quote) {
                    rp++;
                    wp = sheets_record_pop(record);
                    if (sheets_record_push(record, wp) != 0)
                        return sheets_parse_fail(reader, record,
                            SHEETS_READER_ERROR_TOO_MANY_FIELDS);
                    state = SHEETS_STATE_INSIDE_QUOTED_FIELD;
                }
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (sheets_record_push(record, wp) != 0)
                        return sheets_parse_fail(reader, record,
                            SHEETS_READER_ERROR_TOO_MANY_FIELDS);
                }
                else if (sheets_crlf(*rp))
                    return sheets_parse_crlf(reader, record, rp, wp);
                else if (sheets_whitespace(*rp))
                    *wp++ = *rp++;
                else {
                    *wp++ = *rp++;
                    state = SHEETS_STATE_INSIDE_FIELD;
                }
                break;
            case SHEETS_STATE_INSIDE_FIELD:
                if (*rp == quote)
                    return sheets_parse_fail(reader, record,
                        SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (sheets_record_push(record, wp) != 0)
                        return sheets_parse_fail(reader, record,
                            SHEETS_READER_ERROR_TOO_MANY_FIELDS);
                    state = SHEETS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (sheets_crlf(*rp))
                    return sheets_parse_crlf(reader, record, rp, wp);
                else
                    *wp++ = *rp++;
                break;
            case SHEETS_STATE_INSIDE_QUOTED_FIELD:
                if (*rp == quote) {
                    rp++;
                    state = SHEETS_STATE_MAYBE_BEYOND_QUOTED_FIELD;
                }
                else
                    *wp++ = *rp++;
                break;
            case SHEETS_STATE_MAYBE_BEYOND_QUOTED_FIELD:
                if (*rp == quote) {
                    *wp++ = *rp++;
                    state = SHEETS_STATE_INSIDE_QUOTED_FIELD;
                }
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (sheets_record_push(record, wp) != 0)
                        return sheets_parse_fail(reader, record,
                            SHEETS_READER_ERROR_TOO_MANY_FIELDS);
                    state = SHEETS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (sheets_crlf(*rp))
                    return sheets_parse_crlf(reader, record, rp, wp);
                else if (sheets_whitespace(*rp)) {
                    rp++;
                    state = SHEETS_STATE_BEYOND_QUOTED_FIELD;
                }
                else
                    return sheets_parse_fail(reader, record,
                        SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
                break;
            case SHEETS_STATE_BEYOND_QUOTED_FIELD:
                if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (sheets_record_push(record, wp) != 0)
                        return sheets_parse_fail(reader, record,
                            SHEETS_READER_ERROR_TOO_MANY_FIELDS);
                    state = SHEETS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (sheets_crlf(*rp))
                    return sheets_parse_crlf(reader, record, rp, wp);
                else if (sheets_whitespace(*rp))
                    rp++;
                else
                    return sheets_parse_fail(reader, record,
                        SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
                break;
            default:
                return sheets_parse_fail(reader, record,
                    SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
            }

        }

        if (wp == wq) {
            wp = sheets_record_expand(record, wp);
            if (wp == NULL)
                return sheets_parse_fail(reader, record,
                    SHEETS_READER_ERROR_TOO_BIG_RECORD);

            wq = sheets_record_end(record);
        }

        if (rp == rq) {
            if (sheets_reader_fill(reader) != 0)
                return sheets_parse_fail(reader, record,
                    SHEETS_READER_ERROR_UNREADABLE_SOURCE);

            rp = reader->cursor;
            rq = sheets_reader_end(reader);
        }

        if (rp == rq) {
            *wp++ = '\0';
            return sheets_parse_finish(reader, record, rp, wp);
        }
    }

    return sheets_parse_fail(reader, record, SHEETS_READER_ERROR_UNEXPECTED_CHARACTER);
}

static int
sheets_parse_crlf(struct sheets_reader *reader, struct sheets_record *record,
    const char *rp, char *wp)
{
    if (*rp == '\r')
        reader->skip = '\n';

    *wp++ = '\0';
    rp++;

    return sheets_parse_finish(reader, record, rp, wp);
}

static int
sheets_parse_fail(struct sheets_reader *reader, struct sheets_record *record,
    enum sheets_reader_error error)
{
    reader->error = error;

    sheets_record_init(record);

    return SHEETS_FAILURE;
}


static int
sheets_parse_start(struct sheets_reader *reader, struct sheets_record *record)
{
    if (reader->error != 0)
        return sheets_parse_fail(reader, record, reader->error);

    if (reader->cursor == sheets_reader_end(reader)) {
        if (sheets_reader_fill(reader) != 0)
            return sheets_parse_fail(reader, record,
                SHEETS_READER_ERROR_UNREADABLE_SOURCE);

        if (reader->buffer_size == 0)
            return SHEETS_FAILURE;
    }

    sheets_reader_skip(reader);

    return 0;
}

static int
sheets_parse_finish(struct sheets_reader *reader, struct sheets_record *record,
    const char *rp, char *wp)
{
    reader->cursor = rp;

    sheets_record_finish(record, wp);

    return 0;
}

/*
 * Buffer Sources
 * ==============
 */

static struct sheets_buffer *
sheets_buffer_alloc(const char *buffer, size_t buffer_size)
{
    struct sheets_buffer *self;

    self = malloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self;
}

static int
sheets_buffer_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct sheets_buffer *self = source;

    *buffer = self->buffer;
    *buffer_size = self->buffer_size;

    self->buffer = NULL;
    self->buffer_size = 0;

    return 0;
}

static void
sheets_buffer_free(void *source)
{
    struct sheets_buffer *self = source;

    free(self);
}

/*
 * File Sources
 * ============
 */

static struct sheets_file *
sheets_file_alloc(FILE *file, size_t buffer_size)
{
    struct sheets_file *self;
    char *buffer;

    buffer = malloc(buffer_size);
    if (buffer == NULL)
        return NULL;

    self = malloc(sizeof(*self));
    if (self == NULL) {
        free(buffer);
        return NULL;
    }

    self->file = file;
    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self;
}

static int
sheets_file_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct sheets_file *self = source;
    size_t size;

    size = fread(self->buffer, 1, self->buffer_size, self->file);
    if (size != self->buffer_size) {
        if (ferror(self->file))
            return SHEETS_FAILURE;
    }

    *buffer = self->buffer;
    *buffer_size = size;

    return 0;
}

static void
sheets_file_free(void *source)
{
    struct sheets_file *self = source;

    free(self->buffer);
    free(self);
}

/*
 * Utilities
 * =========
 */

static inline bool
sheets_crlf(char ch)
{
    return (ch == '\n') || (ch == '\r');
}

static inline bool
sheets_whitespace(char ch)
{
    return (ch == ' ') || (ch == '\t');
}
