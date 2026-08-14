#include <p101_record/record.h>
#include <stdint.h>
#include <string.h>

enum
{
    NUMBER_BASE  = 10,
    ASCII_DELETE = 0x7F
};

static const char *const NULL_POINTER_SPELLINGS[] = {"-", "0", "0x0", "(nil)", "NULL"};

char *p101_record_split(char **cursor)
{
    char *p101_single_result_;
    char *start;
    char *tab;

    if(cursor == NULL || *cursor == NULL)
    {
        p101_single_result_ = NULL;
        goto p101_single_exit_;
    }

    start = *cursor;
    tab   = start;
    while(*tab != '\0' && *tab != '\t')
    {
        tab++;
    }

    if(*tab == '\0')
    {
        *cursor = NULL;
    }
    else
    {
        *tab    = '\0';
        *cursor = tab + 1;
    }
    p101_single_result_ = start;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

const char *p101_record_escape_byte(unsigned char value)
{
    const char *escaped;

    if(value == '\t')
    {
        escaped = "\\t";
    }
    else if(value == '\n')
    {
        escaped = "\\n";
    }
    else if(value == '\r')
    {
        escaped = "\\r";
    }
    else if(value == '\\')
    {
        escaped = "\\\\";
    }
    else if(value < ' ' || value == ASCII_DELETE)
    {
        escaped = "?";
    }
    else
    {
        escaped = NULL;
    }

    return escaped;
}

void p101_record_unescape_field(char *field)
{
    char *read_cursor;
    char *write_cursor;

    if(field == NULL)
    {
        goto p101_single_exit_;
    }

    read_cursor  = field;
    write_cursor = field;
    while(*read_cursor != '\0')
    {
        if(read_cursor[0] == '\\' && read_cursor[1] != '\0')
        {
            read_cursor++;
            if(*read_cursor == 't')
            {
                *write_cursor = '\t';
            }
            else if(*read_cursor == 'n')
            {
                *write_cursor = '\n';
            }
            else if(*read_cursor == 'r')
            {
                *write_cursor = '\r';
            }
            else
            {
                *write_cursor = *read_cursor;
            }
            read_cursor++;
            write_cursor++;
        }
        else
        {
            *write_cursor++ = *read_cursor++;
        }
    }
    *write_cursor = '\0';

p101_single_exit_:
    return;
}

bool p101_record_pointer_is_null(const char *text)
{
    bool p101_single_result_;

    p101_single_result_ = (text == NULL || text[0] == '\0') != 0;
    if(!p101_single_result_)
    {
        for(size_t index = 0U; index < sizeof(NULL_POINTER_SPELLINGS) / sizeof(NULL_POINTER_SPELLINGS[0]); index++)
        {
            int comparison;

            comparison = strcmp(text, NULL_POINTER_SPELLINGS[index]);
            if(comparison == 0)
            {
                p101_single_result_ = true;
                break;
            }
        }
    }
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_record_parse_size(const char *text, size_t *out)
{
    int         p101_single_result_;
    const char *cursor;
    size_t      value;

    if(text == NULL || out == NULL || *text == '\0')
    {
        p101_single_result_ = 0;
        goto p101_single_exit_;
    }

    cursor = text;
    value  = 0U;
    while(*cursor != '\0')
    {
        size_t digit;

        if(*cursor < '0' || *cursor > '9')
        {
            p101_single_result_ = 0;
            goto p101_single_exit_;
        }
        digit = (size_t)(*cursor - '0');
        if(value > (SIZE_MAX - digit) / (size_t)NUMBER_BASE)
        {
            p101_single_result_ = 0;
            goto p101_single_exit_;
        }
        value = (value * (size_t)NUMBER_BASE) + digit;
        cursor++;
    }
    *out                = value;
    p101_single_result_ = 1;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}
