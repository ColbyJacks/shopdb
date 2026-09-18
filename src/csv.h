/*  Copyright 2015--2021 Samuel Alexander

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TITLE OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    VENDORED AND AMALGAMATED FROM 
    https://github.com/semitrivial/csv_parser
*/

#ifndef CSV_ONCE
#define CSV_ONCE

#include <stdlib.h>
#include <string.h>

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1

#define READ_BLOCK_SIZE 65536
#define QUICK_GETC( ch, fp ) do {\
    if (read_ptr == read_end) {\
        fread_len = fread(read_buf, sizeof(char), READ_BLOCK_SIZE, fp);\
        if (fread_len < READ_BLOCK_SIZE) {\
            read_buf[fread_len] = '\0';\
        }\
        read_ptr = read_buf;\
    }\
    ch = *read_ptr++;\
} while(0)

char **parse_csv(const char *line);
void free_csv_line(char **parsed);
char **split_on_unescaped_newlines(const char *txt);
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err);

#ifdef CSV_IMPLEMENTATION

void free_csv_line(char **parsed) {
    char **ptr;
    for (ptr = parsed; *ptr; ptr++) {
        free(*ptr);
    }
    free(parsed);
}

static int count_fields(const char *line) {
    const char *ptr;
    int cnt, fQuote;
    for (cnt = 1, fQuote = 0, ptr = line; *ptr; ptr++) {
        if (fQuote) {
            if (*ptr == '\"') {
                fQuote = 0;
            }
            continue;
        }
        switch(*ptr) {
            case '\"':
                fQuote = 1;
                continue;
            case ',':
                cnt++;
                continue;
            default:
                continue;
        }
    }
    if (fQuote) {
        return -1;
    }
    return cnt;
}

/*
 *  Given a string containing no linebreaks, or containing line breaks
 *  which are escaped by "double quotes", extract a NULL-terminated
 *  array of strings, one for every cell in the row.
 */
char **parse_csv(const char *line) {
    char **buf, **bptr, *tmp, *tptr;
    const char *ptr;
    int fieldcnt, fQuote, fEnd;
    fieldcnt = count_fields(line);
    if (fieldcnt == -1) {
        return NULL;
    }
    buf = malloc(sizeof(char*) * (fieldcnt+1));
    if (!buf) {
        return NULL;
    }
    tmp = malloc(strlen(line) + 1);
    if (!tmp) {
        free(buf);
        return NULL;
    }
    bptr = buf;
    for (ptr = line, fQuote = 0, *tmp = '\0', tptr = tmp, fEnd = 0; ; ptr++) {
        if (fQuote) {
            if (!*ptr) {
                break;
            }
            if ( *ptr == '\"' ) {
                if ( ptr[1] == '\"' ) {
                    *tptr++ = '\"';
                    ptr++;
                    continue;
                }
                fQuote = 0;
            }
            else {
                *tptr++ = *ptr;
            }
            continue;
        }
        switch( *ptr ) {
            case '\"':
                fQuote = 1;
                continue;
            case '\0':
                fEnd = 1;
            case ',':
                *tptr = '\0';
                *bptr = strdup( tmp );
                if (!*bptr) {
                    for (bptr--; bptr >= buf; bptr--) {
                        free(*bptr);
                    }
                    free(buf);
                    free(tmp);
                    return NULL;
                }
                bptr++;
                tptr = tmp;
                if (fEnd) {
                    break;
                } else {
                    continue;
                }
            default:
                *tptr++ = *ptr;
                continue;
        }
        if (fEnd) {
            break;
        }
    }
    *bptr = NULL;
    free(tmp);
    return buf;
}

/*
 * Given a string which might contain unescaped newlines, split it up into
 * lines which do not contain unescaped newlines, returned as a
 * NULL-terminated array of malloc'd strings.
 */
char **split_on_unescaped_newlines(const char *txt) {
    const char *ptr, *lineStart;
    char **buf, **bptr;
    int fQuote, nLines;
    /* First pass: count how many lines we will need */
    for (nLines = 1, ptr = txt, fQuote = 0; *ptr; ptr++) {
        if (fQuote) {
            if (*ptr == '\"') {
                fQuote = 0;
            }
        } else if (*ptr == '\"') {
            fQuote = 1;
        } else if (*ptr == '\n') {
            nLines++;
        }
    }
    buf = malloc(sizeof(char*) * (nLines+1));
    if (!buf) {
        return NULL;
    }
    /* Second pass: populate results */
    lineStart = txt;
    for (bptr = buf, ptr = txt, fQuote = 0 ;; ptr++) {
        if (fQuote) {
            if (*ptr == '\"') {
                fQuote = 0;
                continue;
            } else if (*ptr) {
                continue;
            }
        }
        if (*ptr == '\"') {
            fQuote = 1;
        } else if (*ptr == '\n' || !*ptr) {
            size_t len = ptr - lineStart;
            if (len == 0) {
                *bptr = NULL;
                return buf;
            }
            *bptr = malloc( len + 1 );
            if (!*bptr) {
                for (bptr--; bptr >= buf; bptr--) {
                    free(*bptr);
                }
                free(buf);
                return NULL;
            }
            memcpy(*bptr, lineStart, len);
            (*bptr)[len] = '\0';
            if (*ptr) {
                lineStart = ptr + 1;
                bptr++;
            } else {
                bptr[1] = NULL;
                return buf;
            }
        }
    }
}

/*
 * Given a file pointer, read a CSV line from that file.
 * File may include newlines escaped with "double quotes".
 *
 * Warning: This function is optimized for the use case where
 *   you repeatedly call it until the file is exhausted.  It is
 *   very suboptimal for the use case of just grabbing one single
 *   line of CSV and stopping.  Also, this function advances the
 *   file position (in the fseek/ftell sense) unpredictably.  You
 *   should not change the file position between calls to
 *   fread_csv_line (e.g., don't use "getc" on the file in between
 *   calls to fread_csv_line).
 *
 * Other arguments:
 * size_t max_line_size: Maximum line size, in bytes.
 * int *done: Pointer to an int that will be set to 1 when file is exhausted.
 * int *err: Pointer to an int where error code will be written.
 *
 * Warning: Calling this function on an exhausted file (as indicated by the
 *   'done' flag) is undefined behavior.
 *
 * See csv.h for definitions of error codes.
 */
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err) {
    static FILE *bookmark;
    static char read_buf[READ_BLOCK_SIZE], *read_ptr, *read_end;
    static int fread_len, prev_max_line_size = -1;
    static char *buf;
    char *bptr, *limit;
    char ch;
    int fQuote;

    if (max_line_size > prev_max_line_size) {
        if (prev_max_line_size != -1) {
            free(buf);
        }
        buf = malloc(max_line_size + 1);
        if (!buf) {
            *err = CSV_ERR_NO_MEMORY;
            prev_max_line_size = -1;
            return NULL;
        }
        prev_max_line_size = max_line_size;
    }
    bptr = buf;
    limit = buf + max_line_size;
    if (bookmark != fp) {
        read_ptr = read_end = read_buf + READ_BLOCK_SIZE;
        bookmark = fp;
    }
    for (fQuote = 0;;) {
        QUICK_GETC(ch, fp);
        if (!ch || (ch == '\n' && !fQuote)) {
            break;
        }
        if (bptr >= limit) {
            free( buf );
            *err = CSV_ERR_LONGLINE;
            return NULL;
        }
        *bptr++ = ch;
        if (fQuote) {
            if (ch == '\"') {
                QUICK_GETC(ch, fp);
                if (ch != '\"') {
                    if (!ch || ch == '\n') {
                        break;
                    }
                    fQuote = 0;
                }
                *bptr++ = ch;
            }
        } else if (ch == '\"') {
            fQuote = 1;
        }
    }
    *done = !ch;
    *bptr = '\0';
    return strdup(buf);
}

SQL_Result load_csv(char* path) {
    SQL_Result result = {0};
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        result.error = sql_copy_string("could not open csv");
        return result;
    }

    int done = 0;
    int err = 0;
    char* line = fread_csv_line(fp, 1024 * 1024, &done, &err);
    if (!line) {
        result.error = sql_copy_string(
            err == CSV_ERR_LONGLINE
                ? "csv line too long"
                : "could not read csv"
        );
        fclose(fp);
        return result;
    }

    char** header = parse_csv(line);
    free(line);
    if (!header) {
        result.error = sql_copy_string("could not parse csv header");
        fclose(fp);
        return result;
    }

    // count columns
    while (header[result.width]) {
        result.width++;
    }
    if (result.width == 0) {
        free_csv_line(header);
        result.error = sql_copy_string("csv has no columns");
        fclose(fp);
        return result;
    }

    // Steal the strings from parse_csv instead of copying them.
    result.columns = malloc(
        sizeof(char*) * (size_t)result.width
    );
    if (!result.columns) {
        free_csv_line(header);
        result.error = sql_copy_string("buy more RAM, lol!");
        fclose(fp);
        return result;
    }
    for (int x = 0; x < result.width; x++) {
        result.columns[x] = header[x];
    }
    // Don't free_csv_line(header), because we stole its strings.
    free(header);
    int row_cap = 0;
    while (!done) {
        line = fread_csv_line(fp, 1024 * 1024, &done, &err);
        if (!line) {
            result.error = sql_copy_string(
                err == CSV_ERR_LONGLINE
                    ? "csv line too long"
                    : "could not read csv"
            );
            goto fail;
        }
        if (done && line[0] == '\0') {
            free(line);
            break;
        }
        char** row = parse_csv(line);
        free(line);
        if (!row) {
            result.error = sql_copy_string("could not parse csv row");
            goto fail;
        }
        // Make sure this row has exactly result.width fields 
        int row_width = 0;
        while (row[row_width]) {
            row_width++;
        }
        if (row_width != result.width) {
            free_csv_line(row);
            result.error = sql_copy_string("csv row has wrong number of columns");
            goto fail;
        }

        if (result.height >= row_cap) {
            int new_cap = row_cap ? row_cap * 2 : 64;
            char** new_cells = realloc(
                result.cells,
                sizeof(char*) *
                (size_t)new_cap *
                (size_t)result.width
            );
            if (!new_cells) {
                free_csv_line(row);
                result.error = sql_copy_string("buy more RAM, lol!");
                goto fail;
            }
            result.cells = new_cells;
            row_cap = new_cap;
        }

        // Again: steal strings instead of strdup'ing everything.
        for (int x = 0; x < result.width; x++) {
            CELL(&result, x, result.height) = row[x];
        }

        free(row);
        result.height++;
    }
    fclose(fp);
    return result;

fail:
    fclose(fp);
    // Can't sql_result_free() directly because that would free error too.
    char* error = result.error;
    result.error = NULL;
    sql_result_free(&result);
    result.error = error;
    return result;
}

#endif
#endif
