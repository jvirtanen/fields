/*
 * Copyright (c) 2013 Jussi Virtanen
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

#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "fields.h"
#include "fields_gzip.h"

#define FIELDS_FAILURE (-1)

struct fields_gzip {
    gzFile  file;
    char *  buffer;
    size_t  buffer_size;
};

static struct fields_gzip *
fields_gzip_alloc(int fd, size_t buffer_size)
{
    struct fields_gzip *self;
    gzFile file;
    char *buffer;
    int dup_fd;

    /*
     * As `gzclose_r` closes the file descriptor given to `gzdopen`, duplicate
     * `fd` and use the duplicate so that the original remains open even after
     * `fields_source_free_fn`.
     */
    dup_fd = dup(fd);
    if (dup_fd == -1)
        return NULL;

    file = gzdopen(dup_fd, "rb");
    if (file == NULL) {
        close(dup_fd);
        return NULL;
    }

    /*
     * zlib maintains two buffers: one of the specified size and one twice the
     * specified size. Specify the size so that the size of the larger buffer
     * will be `buffer_size`.
     */
    if (gzbuffer(file, buffer_size / 2) != 0) {
        gzclose_r(file);
        return NULL;
    }

    buffer = malloc(buffer_size);
    if (buffer == NULL) {
        gzclose_r(file);
        return NULL;
    }

    self = malloc(sizeof(*self));
    if (self == NULL) {
        gzclose_r(file);
        free(buffer);
        return NULL;
    }

    self->file = file;
    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self;
}

static int
fields_gzip_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct fields_gzip *self = source;
    int size;
    int errnum;

    size = gzread(self->file, self->buffer, self->buffer_size);
    if (size == -1)
        return FIELDS_FAILURE;

    gzerror(self->file, &errnum);
    if (errnum != Z_OK)
        return FIELDS_FAILURE;

    *buffer = self->buffer;
    *buffer_size = size;

    return 0;
}

static void
fields_gzip_free(void *source)
{
    struct fields_gzip *self = source;

    gzclose_r(self->file);
    free(self->buffer);
    free(self);
}

struct fields_reader *
fields_read_gzip(int fd, const struct fields_format *format,
    const struct fields_settings *settings)
{
    struct fields_reader *reader;
    struct fields_gzip *source;

    source = fields_gzip_alloc(fd, settings->source_buffer_size);
    if (source == NULL)
        return NULL;

    reader = fields_reader_alloc(source, &fields_gzip_read, &fields_gzip_free,
        format, settings);
    if (reader == NULL) {
        fields_gzip_free(source);
        return NULL;
    }

    return reader;
}
