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

#include "fields.h"

#define FIELDS_FAILURE (-1)

typedef int fields_parse_fn(struct fields_reader *, struct fields_record *);

struct fields_reader
{
    void *                  source;
    fields_source_read_fn * source_read;
    fields_source_free_fn * source_free;
    char                    delimiter;
    char                    escape;
    char                    quote;
    fields_parse_fn *       parse;
    const char *            buffer;
    size_t                  buffer_size;
    const char *            cursor;
    char                    skip;
    int                     error;
};

static const char *fields_reader_end(const struct fields_reader *);
static int fields_reader_fill(struct fields_reader *);
static void fields_reader_skip(struct fields_reader *);

struct fields_record
{
    char *  buffer;
    size_t  buffer_size;
    char ** fields;
    size_t  num_fields;
    size_t  max_fields;
    bool    expand;
};

static const char *fields_record_end(const struct fields_record *);
static void fields_record_init(struct fields_record *);
static char *fields_record_expand(struct fields_record *, char *);
static int fields_record_push(struct fields_record *, char *);
static char *fields_record_pop(struct fields_record *);
static void fields_record_finish(struct fields_record *, char *);
static void fields_record_normalize(struct fields_record *);

static fields_parse_fn *fields_settings_parser(const struct fields_settings *);

static int fields_parse_unquoted(struct fields_reader *,
    struct fields_record *);
static int fields_parse_quoted(struct fields_reader *, struct fields_record *);
static int fields_parse_crlf(struct fields_reader *, struct fields_record *,
    const char *, char *);
static int fields_parse_fail(struct fields_reader *, struct fields_record *,
    enum fields_reader_error);
static int fields_parse_start(struct fields_reader *, struct fields_record *);
static int fields_parse_finish(struct fields_reader *, struct fields_record *,
    const char *, char *);

struct fields_buffer {
    const char *buffer;
    size_t      buffer_size;
};

static struct fields_buffer *fields_buffer_alloc(const char *, size_t);
static int fields_buffer_read(void *, const char **, size_t *);
static void fields_buffer_free(void *);

struct fields_file {
    FILE *  file;
    char *  buffer;
    size_t  buffer_size;
};

static struct fields_file *fields_file_alloc(FILE *, size_t);
static int fields_file_read(void *, const char **, size_t *);
static void fields_file_free(void *);

enum fields_state {
    FIELDS_STATE_MAYBE_INSIDE_FIELD,
    FIELDS_STATE_INSIDE_FIELD,
    FIELDS_STATE_INSIDE_QUOTED_FIELD,
    FIELDS_STATE_MAYBE_BEYOND_QUOTED_FIELD,
    FIELDS_STATE_BEYOND_QUOTED_FIELD
};

static inline bool fields_crlf(char);
static inline bool fields_whitespace(char);

/*
 * Readers
 * =======
 */

struct fields_reader *
fields_read_buffer(const char *buffer, size_t buffer_size,
    const struct fields_settings *settings)
{
    struct fields_reader *reader;
    struct fields_buffer *source;

    source = fields_buffer_alloc(buffer, buffer_size);
    if (source == NULL)
        return NULL;

    reader = fields_reader_alloc(source, &fields_buffer_read,
        &fields_buffer_free, settings);
    if (reader == NULL) {
        fields_buffer_free(source);
        return NULL;
    }

    return reader;
}

struct fields_reader *
fields_read_file(FILE *file, const struct fields_settings *settings)
{
    struct fields_reader *reader;
    struct fields_file *source;

    source = fields_file_alloc(file, settings->file_buffer_size);
    if (source == NULL)
        return NULL;

    reader = fields_reader_alloc(source, &fields_file_read, &fields_file_free,
        settings);
    if (reader == NULL) {
        fields_file_free(source);
        return NULL;
    }

    return reader;
}

struct fields_reader *
fields_reader_alloc(void *source, fields_source_read_fn *read_fn,
    fields_source_free_fn *free_fn, const struct fields_settings *settings)
{
    struct fields_reader *self;

    if (fields_settings_error(settings) != 0)
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
    self->parse = fields_settings_parser(settings);
    self->buffer = NULL;
    self->buffer_size = 0;
    self->cursor = NULL;
    self->skip = '\0';
    self->error = 0;

    return self;
}

void
fields_reader_free(struct fields_reader *self)
{
    self->source_free(self->source);

    free(self);
}

static const char *
fields_reader_end(const struct fields_reader *self)
{
    return self->buffer + self->buffer_size;
}

static int
fields_reader_fill(struct fields_reader *self)
{
    int result;

    result = self->source_read(self->source, &self->buffer, &self->buffer_size);
    self->cursor = self->buffer;

    if (result != 0) {
        self->error = FIELDS_READER_ERROR_UNREADABLE_SOURCE;
        return FIELDS_FAILURE;
    }

    return 0;
}

static void
fields_reader_skip(struct fields_reader *self)
{
    if (self->skip == '\0')
        return;

    if (self->cursor == fields_reader_end(self))
        return;

    if (self->skip != *self->cursor)
        return;

    self->cursor++;
    self->skip = '\0';
}

int
fields_reader_read(struct fields_reader *self, struct fields_record *record)
{
    if (fields_parse_start(self, record) != 0)
        return FIELDS_FAILURE;

    fields_record_init(record);

    return self->parse(self, record);
}

int
fields_reader_error(const struct fields_reader *self)
{
    return self->error;
}

const char *
fields_reader_strerror(int error)
{
    switch (error) {
    case FIELDS_READER_ERROR_TOO_BIG_RECORD:
        return "Too big record";
    case FIELDS_READER_ERROR_TOO_MANY_FIELDS:
        return "Too many fields";
    case FIELDS_READER_ERROR_UNEXPECTED_CHARACTER:
        return "Unexpected character";
    case FIELDS_READER_ERROR_UNREADABLE_SOURCE:
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

struct fields_record *
fields_record_alloc(const struct fields_settings *settings)
{
    struct fields_record *self;
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
fields_record_free(struct fields_record *self)
{
    free(self->buffer);
    free(self->fields);
    free(self);
}

int
fields_record_field(const struct fields_record *self, unsigned int index,
    struct fields_field *field)
{
    if (index >= self->num_fields)
        return FIELDS_FAILURE;

    field->value = self->fields[index];
    field->length = self->fields[index + 1] - self->fields[index] - 1;
    return 0;
}

size_t
fields_record_size(const struct fields_record *self)
{
    return self->num_fields;
}

static const char *
fields_record_end(const struct fields_record *self)
{
    return self->buffer + self->buffer_size;
}

static void
fields_record_init(struct fields_record *self)
{
    self->num_fields = 0;
}

static char *
fields_record_expand(struct fields_record *self, char *cursor)
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
fields_record_push(struct fields_record *self, char *cursor)
{
    if (self->num_fields == self->max_fields) {
        size_t  max_fields;
        char ** fields;

        if (!self->expand)
            return FIELDS_FAILURE;

        max_fields = self->max_fields * 2;

        fields = realloc(self->fields, (max_fields + 1) * sizeof(char *));
        if (fields == NULL)
            return FIELDS_FAILURE;

        self->fields = fields;
        self->max_fields = max_fields;
    }

    self->fields[self->num_fields++] = cursor;
    return 0;
}

static char *
fields_record_pop(struct fields_record *self)
{
    if (self->num_fields == 0)
        return self->buffer;

    self->num_fields--;

    return self->fields[self->num_fields];
}

static void
fields_record_finish(struct fields_record *self, char *cursor)
{
    self->fields[self->num_fields] = cursor;

    fields_record_normalize(self);
}

static void
fields_record_normalize(struct fields_record *self)
{
    if (fields_record_size(self) == 1) {
        struct fields_field field;

        fields_record_field(self, 0, &field);

        if (field.length == 0)
            fields_record_pop(self);
    }
}

/*
 * Settings
 * ========
 */

const struct fields_settings fields_csv =
{
    .delimiter          = ',',
    .escape             = '\0',
    .quote              = '"',
    .expand             = true,
    .file_buffer_size   = FIELDS_DEFAULT_FILE_BUFFER_SIZE,
    .record_buffer_size = FIELDS_DEFAULT_RECORD_BUFFER_SIZE,
    .record_max_fields  = FIELDS_DEFAULT_RECORD_MAX_FIELDS
};

const struct fields_settings fields_tsv =
{
    .delimiter          = '\t',
    .escape             = '\0',
    .quote              = '\0',
    .expand             = true,
    .file_buffer_size   = FIELDS_DEFAULT_FILE_BUFFER_SIZE,
    .record_buffer_size = FIELDS_DEFAULT_RECORD_BUFFER_SIZE,
    .record_max_fields  = FIELDS_DEFAULT_RECORD_MAX_FIELDS
};

int
fields_settings_error(const struct fields_settings *settings)
{
    if (settings->delimiter == '\n')
        return FIELDS_SETTINGS_ERROR_DELIMITER;

    if (settings->delimiter == '\r')
        return FIELDS_SETTINGS_ERROR_DELIMITER;

    if (settings->escape == '\n')
        return FIELDS_SETTINGS_ERROR_ESCAPE;

    if (settings->escape == '\r')
        return FIELDS_SETTINGS_ERROR_ESCAPE;

    if (settings->escape == settings->delimiter)
        return FIELDS_SETTINGS_ERROR_ESCAPE;

    if (settings->quote == '\n')
        return FIELDS_SETTINGS_ERROR_QUOTE;

    if (settings->quote == '\r')
        return FIELDS_SETTINGS_ERROR_QUOTE;

    if (settings->quote == settings->delimiter)
        return FIELDS_SETTINGS_ERROR_QUOTE;

    if ((settings->quote != '\0') && (settings->escape != '\0'))
        return FIELDS_SETTINGS_ERROR_QUOTE;

    if (settings->file_buffer_size < FIELDS_MINIMUM_FILE_BUFFER_SIZE)
        return FIELDS_SETTINGS_ERROR_FILE_BUFFER_SIZE;

    if (settings->record_buffer_size < FIELDS_MINIMUM_RECORD_BUFFER_SIZE)
        return FIELDS_SETTINGS_ERROR_RECORD_BUFFER_SIZE;

    if (settings->record_max_fields < FIELDS_MINIMUM_RECORD_MAX_FIELDS)
        return FIELDS_SETTINGS_ERROR_RECORD_MAX_FIELDS;

    return 0;
}

const char *
fields_settings_strerror(int error)
{
    switch (error) {
    case FIELDS_SETTINGS_ERROR_DELIMITER:
        return "Bad field delimiter";
    case FIELDS_SETTINGS_ERROR_ESCAPE:
        return "Bad escape character";
    case FIELDS_SETTINGS_ERROR_QUOTE:
        return "Bad quote character";
    case FIELDS_SETTINGS_ERROR_FILE_BUFFER_SIZE:
        return "Too low file buffer size";
    case FIELDS_SETTINGS_ERROR_RECORD_BUFFER_SIZE:
        return "Too low record buffer size";
    case FIELDS_SETTINGS_ERROR_RECORD_MAX_FIELDS:
        return "Too low maximum for fields in record";
    case 0:
        return "";
    default:
        break;
    }

    return "Unknown error";
}

static fields_parse_fn *
fields_settings_parser(const struct fields_settings *settings)
{
    if (settings->quote != '\0')
        return &fields_parse_quoted;
    else
        return &fields_parse_unquoted;
}

/*
 * Parsers
 * =======
 */

static int
fields_parse_unquoted(struct fields_reader *reader, struct fields_record *record)
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
    rq = fields_reader_end(reader);

    wp = record->buffer;
    wq = fields_record_end(record);

    fields_record_push(record, wp);

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
            else if (fields_crlf(*rp))
                return fields_parse_crlf(reader, record, rp, wp);
            else if (*rp == delimiter) {
                *wp++ = '\0';
                rp++;
                if (fields_record_push(record, wp) != 0)
                    return fields_parse_fail(reader, record,
                        FIELDS_READER_ERROR_TOO_MANY_FIELDS);
            }
            else
                *wp++ = *rp++;
        }

        if (wp == wq) {
            wp = fields_record_expand(record, wp);
            if (wp == NULL)
                return fields_parse_fail(reader, record,
                    FIELDS_READER_ERROR_TOO_BIG_RECORD);

            wq = fields_record_end(record);
        }

        if (rp == rq) {
            if (fields_reader_fill(reader) != 0)
                return fields_parse_fail(reader, record,
                    FIELDS_READER_ERROR_UNREADABLE_SOURCE);

            rp = reader->cursor;
            rq = fields_reader_end(reader);
        }

        if (rp == rq) {
            *wp++ = '\0';
            return fields_parse_finish(reader, record, rp, wp);
        }
    }

    return fields_parse_fail(reader, record,
        FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
}

static int
fields_parse_quoted(struct fields_reader *reader, struct fields_record *record)
{
    enum fields_state state;

    char delimiter;
    char quote;

    const char *rp;
    const char *rq;

    char *wp;
    const char *wq;

    state = FIELDS_STATE_MAYBE_INSIDE_FIELD;

    delimiter = reader->delimiter;
    quote = reader->quote;

    rp = reader->cursor;
    rq = fields_reader_end(reader);

    wp = record->buffer;
    wq = fields_record_end(record);

    fields_record_push(record, wp);

    while (true) {
        while ((rp != rq) && (wp != wq)) {
            switch (state) {
            case FIELDS_STATE_MAYBE_INSIDE_FIELD:
                if (*rp == quote) {
                    rp++;
                    wp = fields_record_pop(record);
                    if (fields_record_push(record, wp) != 0)
                        return fields_parse_fail(reader, record,
                            FIELDS_READER_ERROR_TOO_MANY_FIELDS);
                    state = FIELDS_STATE_INSIDE_QUOTED_FIELD;
                }
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (fields_record_push(record, wp) != 0)
                        return fields_parse_fail(reader, record,
                            FIELDS_READER_ERROR_TOO_MANY_FIELDS);
                }
                else if (fields_crlf(*rp))
                    return fields_parse_crlf(reader, record, rp, wp);
                else if (fields_whitespace(*rp))
                    *wp++ = *rp++;
                else {
                    *wp++ = *rp++;
                    state = FIELDS_STATE_INSIDE_FIELD;
                }
                break;
            case FIELDS_STATE_INSIDE_FIELD:
                if (*rp == quote)
                    return fields_parse_fail(reader, record,
                        FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (fields_record_push(record, wp) != 0)
                        return fields_parse_fail(reader, record,
                            FIELDS_READER_ERROR_TOO_MANY_FIELDS);
                    state = FIELDS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (fields_crlf(*rp))
                    return fields_parse_crlf(reader, record, rp, wp);
                else
                    *wp++ = *rp++;
                break;
            case FIELDS_STATE_INSIDE_QUOTED_FIELD:
                if (*rp == quote) {
                    rp++;
                    state = FIELDS_STATE_MAYBE_BEYOND_QUOTED_FIELD;
                }
                else
                    *wp++ = *rp++;
                break;
            case FIELDS_STATE_MAYBE_BEYOND_QUOTED_FIELD:
                if (*rp == quote) {
                    *wp++ = *rp++;
                    state = FIELDS_STATE_INSIDE_QUOTED_FIELD;
                }
                else if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (fields_record_push(record, wp) != 0)
                        return fields_parse_fail(reader, record,
                            FIELDS_READER_ERROR_TOO_MANY_FIELDS);
                    state = FIELDS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (fields_crlf(*rp))
                    return fields_parse_crlf(reader, record, rp, wp);
                else if (fields_whitespace(*rp)) {
                    rp++;
                    state = FIELDS_STATE_BEYOND_QUOTED_FIELD;
                }
                else
                    return fields_parse_fail(reader, record,
                        FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
                break;
            case FIELDS_STATE_BEYOND_QUOTED_FIELD:
                if (*rp == delimiter) {
                    *wp++ = '\0';
                    rp++;
                    if (fields_record_push(record, wp) != 0)
                        return fields_parse_fail(reader, record,
                            FIELDS_READER_ERROR_TOO_MANY_FIELDS);
                    state = FIELDS_STATE_MAYBE_INSIDE_FIELD;
                }
                else if (fields_crlf(*rp))
                    return fields_parse_crlf(reader, record, rp, wp);
                else if (fields_whitespace(*rp))
                    rp++;
                else
                    return fields_parse_fail(reader, record,
                        FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
                break;
            default:
                return fields_parse_fail(reader, record,
                    FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
            }

        }

        if (wp == wq) {
            wp = fields_record_expand(record, wp);
            if (wp == NULL)
                return fields_parse_fail(reader, record,
                    FIELDS_READER_ERROR_TOO_BIG_RECORD);

            wq = fields_record_end(record);
        }

        if (rp == rq) {
            if (fields_reader_fill(reader) != 0)
                return fields_parse_fail(reader, record,
                    FIELDS_READER_ERROR_UNREADABLE_SOURCE);

            rp = reader->cursor;
            rq = fields_reader_end(reader);
        }

        if (rp == rq) {
            *wp++ = '\0';
            return fields_parse_finish(reader, record, rp, wp);
        }
    }

    return fields_parse_fail(reader, record,
        FIELDS_READER_ERROR_UNEXPECTED_CHARACTER);
}

static int
fields_parse_crlf(struct fields_reader *reader, struct fields_record *record,
    const char *rp, char *wp)
{
    if (*rp == '\r')
        reader->skip = '\n';

    *wp++ = '\0';
    rp++;

    return fields_parse_finish(reader, record, rp, wp);
}

static int
fields_parse_fail(struct fields_reader *reader, struct fields_record *record,
    enum fields_reader_error error)
{
    reader->error = error;

    fields_record_init(record);

    return FIELDS_FAILURE;
}

static int
fields_parse_start(struct fields_reader *reader, struct fields_record *record)
{
    if (reader->error != 0)
        return fields_parse_fail(reader, record, reader->error);

    if (reader->cursor == fields_reader_end(reader)) {
        if (fields_reader_fill(reader) != 0)
            return fields_parse_fail(reader, record,
                FIELDS_READER_ERROR_UNREADABLE_SOURCE);

        if (reader->buffer_size == 0)
            return FIELDS_FAILURE;
    }

    fields_reader_skip(reader);

    return 0;
}

static int
fields_parse_finish(struct fields_reader *reader, struct fields_record *record,
    const char *rp, char *wp)
{
    reader->cursor = rp;

    fields_record_finish(record, wp);

    return 0;
}

/*
 * Buffer Sources
 * ==============
 */

static struct fields_buffer *
fields_buffer_alloc(const char *buffer, size_t buffer_size)
{
    struct fields_buffer *self;

    self = malloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self;
}

static int
fields_buffer_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct fields_buffer *self = source;

    *buffer = self->buffer;
    *buffer_size = self->buffer_size;

    self->buffer = NULL;
    self->buffer_size = 0;

    return 0;
}

static void
fields_buffer_free(void *source)
{
    struct fields_buffer *self = source;

    free(self);
}

/*
 * File Sources
 * ============
 */

static struct fields_file *
fields_file_alloc(FILE *file, size_t buffer_size)
{
    struct fields_file *self;
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
fields_file_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct fields_file *self = source;
    size_t size;

    size = fread(self->buffer, 1, self->buffer_size, self->file);
    if (size != self->buffer_size) {
        if (ferror(self->file))
            return FIELDS_FAILURE;
    }

    *buffer = self->buffer;
    *buffer_size = size;

    return 0;
}

static void
fields_file_free(void *source)
{
    struct fields_file *self = source;

    free(self->buffer);
    free(self);
}

/*
 * Utilities
 * =========
 */

static inline bool
fields_crlf(char ch)
{
    return (ch == '\n') || (ch == '\r');
}

static inline bool
fields_whitespace(char ch)
{
    return (ch == ' ') || (ch == '\t');
}