#include <p101_record/record.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check(bool condition, int line)
{
    int status;

    status = condition ? EXIT_SUCCESS : EXIT_FAILURE;
    if(status != EXIT_SUCCESS)
    {
        fprintf(stderr, "%s:%d: error: record contract failed\n", __FILE__, line);
    }
    return status;
}

#define CHECK(expression)                                                                                                                                                                                                                                          \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        status = check((expression), __LINE__);                                                                                                                                                                                                                    \
        if(status != EXIT_SUCCESS)                                                                                                                                                                                                                                 \
        {                                                                                                                                                                                                                                                          \
            goto done;                                                                                                                                                                                                                                             \
        }                                                                                                                                                                                                                                                          \
    } while(0)

int main(void)
{
    char   text[] = "one\ttwo";
    char  *cursor;
    size_t value;
    int    status;

    status = EXIT_SUCCESS;
    cursor = text;
    CHECK(strcmp(p101_record_split(&cursor), "one") == 0);
    CHECK(strcmp(p101_record_split(&cursor), "two") == 0);
    CHECK(cursor == NULL);
    CHECK(p101_record_parse_size("42", &value) == 1);
    CHECK(value == 42U);
    CHECK(p101_record_parse_size("42x", &value) == 0);
done:
    return status;
}
