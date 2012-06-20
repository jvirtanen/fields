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

#include <stdlib.h>
#include <unistd.h>

#include "sheets.h"
#include "sheets_posix.h"

#define SHEETS_FAILURE (-1)

struct sheets_fd {
    int     fd;
    char *  buffer;
    size_t  buffer_size;
};

static struct sheets_fd *sheets_fd_alloc(int, size_t);
static int sheets_fd_read(void *, const char **, size_t *);
static void sheets_fd_free(void *);

struct sheets_reader *
sheets_read_fd(int fd, const struct sheets_settings *settings)
{
    struct sheets_reader *reader;
    struct sheets_fd *source;

    source = sheets_fd_alloc(fd, settings->file_buffer_size);
    if (source == NULL)
        return NULL;

    reader = sheets_reader_alloc(source, &sheets_fd_read, &sheets_fd_free,
        settings);
    if (reader == NULL) {
        sheets_fd_free(source);
        return NULL;
    }

    return reader;
}

static struct sheets_fd *
sheets_fd_alloc(int fd, size_t buffer_size)
{
    struct sheets_fd *self;
    char *buffer;

    buffer = malloc(buffer_size);
    if (buffer == NULL)
        return NULL;

    self = malloc(sizeof(*self));
    if (self == NULL) {
        free(self);
        return NULL;
    }

    self->fd = fd;
    self->buffer = buffer;
    self->buffer_size = buffer_size;

    return self;
}

static int
sheets_fd_read(void *source, const char **buffer, size_t *buffer_size)
{
    struct sheets_fd *self = source;
    ssize_t size;

    size = read(self->fd, self->buffer, self->buffer_size);
    if (size == -1)
        return SHEETS_FAILURE;

    *buffer = self->buffer;
    *buffer_size = size;

    return 0;
}

static void
sheets_fd_free(void *source)
{
    struct sheets_fd *self = source;

    free(self->buffer);
    free(self);
}
