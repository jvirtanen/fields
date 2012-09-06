#include <stdio.h>
#include <stdlib.h>

#include "fields.h"

/*
 * Read tabular text data in the Yahoo! Finance Historical Prices format from
 * the standard input, print adjusted close prices to the standard output.
 *
 * For testing, you can download the data for SPDR S&P 500 (SPY) at:
 *
 *     http://finance.yahoo.com/q/hp?s=SPY
 */

static void die(const char *);

int
main(void)
{
    struct fields_reader *reader;
    struct fields_record *record;

    reader = fields_read_file(stdin, &fields_csv);
    if (reader == NULL)
        die("fields_read_file");

    record = fields_record_alloc(&fields_csv);
    if (record == NULL)
        die("fields_record_alloc");

    /* Skip the header. */
    fields_reader_read(reader, record);

    while (fields_reader_read(reader, record) == 0) {
        struct fields_field date;
        struct fields_field price;

        /* Date, Open, High, Low, Close, Volume and Adj Close. */
        if (fields_record_size(record) != 7)
            die("fields_record_size");

        /* Date and Adj Close. */
        fields_record_field(record, 0, &date);
        fields_record_field(record, 6, &price);

        printf("%s\t%s\n", date.value, price.value);
    }

    if (fields_reader_error(reader) != 0)
        die("fields_reader_error");

    fields_record_free(record);
    fields_reader_free(reader);

    return 0;
}

static void
die(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}
