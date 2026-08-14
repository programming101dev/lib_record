#include <errno.h>
#include <p101_record/event.h>
#include <p101_record/record.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#if defined(__GNUC__) || defined(__clang__)
    #define P101_RECORD_PRINTF(format_index, first_arg) __attribute__((format(printf, format_index, first_arg)))
#else
    #define P101_RECORD_PRINTF(format_index, first_arg)
#endif

enum
{
    EVENT_FD_MAX = 1048576
};

struct line_builder
{
    char   data[P101_TOOL_EVENT_LINE_MAX_BYTES + 1U];
    size_t length;
    int    failed;
};

static void append_char(struct line_builder *builder, char value);
static void append_text(struct line_builder *builder, const char *text);
static void append_format(struct line_builder *builder, const char *format, ...) P101_RECORD_PRINTF(2, 3);
static void append_field(struct line_builder *builder, const char *text);
static void write_metadata(struct line_builder *builder, const struct p101_tool_event_output *record);
static void write_payload(struct line_builder *builder, const struct p101_tool_event_output *record);
static bool output_is_valid(const struct p101_tool_event_output *record);

#ifdef P101_RECORD_TESTING
static int force_zero_errno_on_write_error;
static int force_format_overflow;

void p101_record_test_force_zero_errno_on_write_error(void)
{
    force_zero_errno_on_write_error = 1;
}

void p101_record_test_force_format_overflow(void)
{
    force_format_overflow = 1;
}
#endif

int p101_tool_event_write(FILE *stream, const struct p101_tool_event_output *record)
{
    int                 p101_single_result_;
    struct line_builder builder;
    const char         *magic;
    int                 actual_error;
    int                 saved_error;
    int                 result;
    bool                valid;
    int                 flush_status;
    int                 descriptor;
    ssize_t             bytes_written;

    valid = false;
    if(stream != NULL && record != NULL)
    {
        valid = output_is_valid(record);
    }
    if(!valid)
    {
        errno               = EINVAL;
        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    memset(&builder, 0, sizeof(builder));
    magic = p101_record_event_magic(record->record_kind);
    append_text(&builder, magic);
    append_char(&builder, '\t');
    write_metadata(&builder, record);
    write_payload(&builder, record);
    append_char(&builder, '\n');
    if(builder.failed != 0)
    {
        errno               = EMSGSIZE;
        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    saved_error  = errno;
    actual_error = 0;
    result       = 0;
    errno        = 0;
    flockfile(stream);
    flush_status  = fflush(stream);
    bytes_written = (ssize_t)builder.length;
    if(flush_status != EOF)
    {
        descriptor    = fileno(stream);
        bytes_written = write(descriptor, builder.data, builder.length);
    }
    if(flush_status == EOF || bytes_written != (ssize_t)builder.length)
    {
        result = -1;
#ifdef P101_RECORD_TESTING
        if(force_zero_errno_on_write_error != 0)
        {
            force_zero_errno_on_write_error = 0;
            errno                           = 0;
        }
#endif
        actual_error = errno == 0 ? EIO : errno;
    }
    funlockfile(stream);
    errno               = result == 0 ? saved_error : actual_error;
    p101_single_result_ = result;

p101_single_exit_:
    return p101_single_result_;
}

static void append_char(struct line_builder *builder, char value)
{
    if(builder->failed != 0)
    {
        goto p101_single_exit_;
    }
    if(builder->length >= P101_TOOL_EVENT_LINE_MAX_BYTES)
    {
        builder->failed = 1;
        goto p101_single_exit_;
    }
    builder->data[builder->length] = value;
    builder->length++;
    builder->data[builder->length] = '\0';

p101_single_exit_:
    return;
}

static void append_text(struct line_builder *builder, const char *text)
{
    while(*text != '\0')
    {
        append_char(builder, *text);
        text++;
    }
}

static void append_format(struct line_builder *builder, const char *format, ...)
{
    va_list arguments;
    int     written;
    size_t  available;

    if(builder->failed != 0)
    {
        goto p101_single_exit_;
    }
    available = sizeof(builder->data) - builder->length;
    va_start(arguments, format);
    written = vsnprintf(builder->data + builder->length, available, format, arguments);
    va_end(arguments);
#ifdef P101_RECORD_TESTING
    if(force_format_overflow != 0)
    {
        force_format_overflow = 0;
        written               = (int)available;
    }
#endif
    if(written < 0 || (size_t)written >= available)
    {
        builder->failed = 1;
        goto p101_single_exit_;
    }
    builder->length += (size_t)written;

p101_single_exit_:
    return;
}

static void write_metadata(struct line_builder *builder, const struct p101_tool_event_output *record)
{
    int version;

    version = record->version == 0 ? P101_TOOL_EVENT_LOG_VERSION : record->version;
    append_format(builder, "%d\t", version);
    append_field(builder, record->run_id);
    append_format(builder, "\t%ld\t%zu\t%zu\t", record->pid, record->context_id, record->sequence);
    if(record->monotonic_ns_available != 0)
    {
        append_format(builder, "%zu\t", record->monotonic_ns);
    }
    else
    {
        append_text(builder, "-\t");
    }
    if(record->wall_unix_ns_available != 0)
    {
        append_format(builder, "%zu\t", record->wall_unix_ns);
    }
    else
    {
        append_text(builder, "-\t");
    }
}

static void write_payload(struct line_builder *builder, const struct p101_tool_event_output *record)
{
    const char *kind_name;

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
    switch(record->record_kind)
    {
        case P101_TOOL_EVENT_RECORD_FD:
            kind_name = p101_record_event_fd_kind_name(record->fd_kind);
            append_format(builder, "%s\t%d\t%d\t", kind_name, record->fd, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            break;
        case P101_TOOL_EVENT_RECORD_ALLOC:
            kind_name = p101_record_event_alloc_kind_name(record->alloc_kind);
            append_format(builder, "%s\t", kind_name);
            append_field(builder, record->ptr);
            append_char(builder, '\t');
            append_field(builder, record->new_ptr);
            append_format(builder, "\t%zu\t%d\t", record->size, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            break;
        case P101_TOOL_EVENT_RECORD_FORK:
            append_format(builder, "%ld\t%d\t", record->child_pid, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            break;
        case P101_TOOL_EVENT_RECORD_SPAWN:
            append_format(builder, "%ld\t%d\t", record->child_pid, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            append_char(builder, '\t');
            append_field(builder, record->target);
            break;
        case P101_TOOL_EVENT_RECORD_EXEC:
            append_format(builder, "%d\t%d\t%d\t", record->fd, record->cloexec, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            append_char(builder, '\t');
            append_field(builder, record->target);
            break;
        case P101_TOOL_EVENT_RECORD_EXEC_FAIL:
            append_format(builder, "%d\t", record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            append_char(builder, '\t');
            append_field(builder, record->target);
            break;
        case P101_TOOL_EVENT_RECORD_CALL:
            kind_name = p101_record_event_call_kind_name(record->call_kind);
            append_format(builder, "%s\t%d\t", kind_name, record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->call_name);
            append_char(builder, '\t');
            append_field(builder, record->arguments);
            append_char(builder, '\t');
            append_field(builder, record->result);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            break;
        case P101_TOOL_EVENT_RECORD_RESOURCE:
            kind_name = p101_record_event_resource_kind_name(record->resource_kind);
            append_format(builder, "%s\t", kind_name);
            append_field(builder, record->resource_class);
            append_char(builder, '\t');
            append_field(builder, record->resource_id);
            append_char(builder, '\t');
            append_field(builder, record->related_id);
            append_format(builder, "\t%zu\t", record->size);
            append_field(builder, record->metadata);
            append_format(builder, "\t%d\t", record->line_number);
            append_field(builder, record->function_name);
            append_char(builder, '\t');
            append_field(builder, record->file_name);
            break;
        case P101_TOOL_EVENT_RECORD_COMPLETE:
            append_format(builder, "%zu\t%d\t%d", record->events_attempted, record->write_failed, record->write_errno);
            break;
        default:
            break;
    }
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
}

static void append_field(struct line_builder *builder, const char *text)
{
    if(text == NULL)
    {
        append_text(builder, "-");
    }
    else if(text[0] == '-' && text[1] == '\0')
    {
        append_text(builder, "\\-");
    }
    else
    {
        while(*text != '\0')
        {
            unsigned char ch;
            const char   *escaped;

            ch      = (unsigned char)*text;
            escaped = p101_record_escape_byte(ch);
            if(escaped != NULL)
            {
                append_text(builder, escaped);
            }
            else
            {
                append_char(builder, (char)ch);
            }
            text++;
        }
    }
}

static bool output_is_valid(const struct p101_tool_event_output *record)
{
    bool   p101_single_result_;
    int    version;
    size_t run_id_length;

    version       = record->version == 0 ? P101_TOOL_EVENT_LOG_VERSION : record->version;
    run_id_length = 0U;
    if(record->run_id != NULL)
    {
        run_id_length = strlen(record->run_id);
    }
    if(version != P101_TOOL_EVENT_LOG_VERSION || record->run_id == NULL || record->run_id[0] == '\0' || run_id_length > P101_TOOL_EVENT_RUN_ID_MAX_BYTES || record->pid < 0 || (record->monotonic_ns_available != 0 && record->monotonic_ns_available != 1) ||
       (record->wall_unix_ns_available != 0 && record->wall_unix_ns_available != 1))
    {
        p101_single_result_ = false;
        goto p101_single_exit_;
    }
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
    switch(record->record_kind)
    {
        case P101_TOOL_EVENT_RECORD_FD:
            p101_single_result_ = (record->fd >= 0 && record->fd <= EVENT_FD_MAX && record->line_number >= 0 && (record->fd_kind == P101_TOOL_EVENT_FD_OPEN || record->fd_kind == P101_TOOL_EVENT_FD_CLOSE)) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_ALLOC:
            p101_single_result_ = (record->line_number >= 0 && (record->alloc_kind == P101_TOOL_EVENT_ALLOC_ALLOC || record->alloc_kind == P101_TOOL_EVENT_ALLOC_FREE || record->alloc_kind == P101_TOOL_EVENT_ALLOC_REALLOC)) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_CALL:
            p101_single_result_ = (record->line_number >= 0 && (record->call_kind == P101_TOOL_EVENT_CALL_ENTER || record->call_kind == P101_TOOL_EVENT_CALL_EXIT)) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_RESOURCE:
            p101_single_result_ = (record->line_number >= 0 && (record->resource_kind == P101_TOOL_EVENT_RESOURCE_ACQUIRE || record->resource_kind == P101_TOOL_EVENT_RESOURCE_RELEASE || record->resource_kind == P101_TOOL_EVENT_RESOURCE_REPLACE ||
                                                                record->resource_kind == P101_TOOL_EVENT_RESOURCE_TRANSFER)) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_FORK:
        case P101_TOOL_EVENT_RECORD_SPAWN:
            p101_single_result_ = (record->child_pid >= 0 && record->line_number >= 0) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_EXEC:
            p101_single_result_ = (record->fd >= 0 && record->fd <= EVENT_FD_MAX && (record->cloexec == 0 || record->cloexec == 1) && record->line_number >= 0) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_EXEC_FAIL:
            p101_single_result_ = (record->line_number >= 0) != 0;
            break;
        case P101_TOOL_EVENT_RECORD_COMPLETE:
            p101_single_result_ = ((record->write_failed == 0 || record->write_failed == 1) && ((record->write_failed == 0 && record->write_errno == 0) || (record->write_failed == 1 && record->write_errno > 0))) != 0;
            break;
        default:
            p101_single_result_ = false;
            break;
    }
#ifdef __clang__
    #pragma clang diagnostic pop
#endif

p101_single_exit_:
    return p101_single_result_;
}

const char *p101_record_event_magic(p101_tool_event_record_kind kind)
{
    static const char *const names[] = {
        [P101_TOOL_EVENT_RECORD_FD]        = "P101FD",
        [P101_TOOL_EVENT_RECORD_ALLOC]     = "P101ALLOC",
        [P101_TOOL_EVENT_RECORD_FORK]      = "P101FORK",
        [P101_TOOL_EVENT_RECORD_SPAWN]     = "P101SPAWN",
        [P101_TOOL_EVENT_RECORD_EXEC]      = "P101EXEC",
        [P101_TOOL_EVENT_RECORD_EXEC_FAIL] = "P101EXECFAIL",
        [P101_TOOL_EVENT_RECORD_CALL]      = "P101CALL",
        [P101_TOOL_EVENT_RECORD_RESOURCE]  = "P101RESOURCE",
        [P101_TOOL_EVENT_RECORD_COMPLETE]  = "P101COMPLETE",
    };
    return names[kind];
}

const char *p101_record_event_fd_kind_name(p101_tool_event_fd_kind kind)
{
    static const char *const names[] = {[P101_TOOL_EVENT_FD_OPEN] = "OPEN", [P101_TOOL_EVENT_FD_CLOSE] = "CLOSE"};
    return names[kind];
}

const char *p101_record_event_alloc_kind_name(p101_tool_event_alloc_kind kind)
{
    static const char *const names[] = {[P101_TOOL_EVENT_ALLOC_ALLOC] = "ALLOC", [P101_TOOL_EVENT_ALLOC_FREE] = "FREE", [P101_TOOL_EVENT_ALLOC_REALLOC] = "REALLOC"};
    return names[kind];
}

const char *p101_record_event_call_kind_name(p101_tool_event_call_kind kind)
{
    static const char *const names[] = {[P101_TOOL_EVENT_CALL_ENTER] = "ENTER", [P101_TOOL_EVENT_CALL_EXIT] = "EXIT"};
    return names[kind];
}

const char *p101_record_event_resource_kind_name(p101_tool_event_resource_kind kind)
{
    static const char *const names[] = {[P101_TOOL_EVENT_RESOURCE_ACQUIRE] = "ACQUIRE", [P101_TOOL_EVENT_RESOURCE_RELEASE] = "RELEASE", [P101_TOOL_EVENT_RESOURCE_REPLACE] = "REPLACE", [P101_TOOL_EVENT_RESOURCE_TRANSFER] = "TRANSFER"};
    return names[kind];
}

#ifdef P101_RECORD_TESTING
void p101_record_test_write_unknown_payload(void)
{
    struct line_builder           builder;
    struct p101_tool_event_output record;

    memset(&builder, 0, sizeof(builder));
    memset(&record, 0, sizeof(record));
    record.record_kind = (p101_tool_event_record_kind)99;
    write_payload(&builder, &record);
}
#endif
