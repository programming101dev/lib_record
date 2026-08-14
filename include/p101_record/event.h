#ifndef P101_RECORD_EVENT_H
#define P101_RECORD_EVENT_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Minimal event-wire contract used by lib_env producers. Parsing,
     * lifecycle analysis, and reporting remain in p101_tool_event.
     */
    enum
    {
        P101_TOOL_EVENT_LINE_MAX_BYTES   = 4096,
        P101_TOOL_EVENT_RUN_ID_MAX_BYTES = 96,
        P101_TOOL_EVENT_LOG_VERSION      = 5
    };

#define P101_TOOL_EVENT_SCHEMA_NAME "p101-tool-event-format-v5"

    typedef enum
    {
        P101_TOOL_EVENT_RECORD_FD = 0,
        P101_TOOL_EVENT_RECORD_ALLOC,
        P101_TOOL_EVENT_RECORD_FORK,
        P101_TOOL_EVENT_RECORD_SPAWN,
        P101_TOOL_EVENT_RECORD_EXEC,
        P101_TOOL_EVENT_RECORD_EXEC_FAIL,
        P101_TOOL_EVENT_RECORD_CALL,
        P101_TOOL_EVENT_RECORD_RESOURCE,
        P101_TOOL_EVENT_RECORD_COMPLETE
    } p101_tool_event_record_kind;

    typedef enum
    {
        P101_TOOL_EVENT_FD_OPEN = 0,
        P101_TOOL_EVENT_FD_CLOSE
    } p101_tool_event_fd_kind;

    typedef enum
    {
        P101_TOOL_EVENT_ALLOC_ALLOC = 0,
        P101_TOOL_EVENT_ALLOC_FREE,
        P101_TOOL_EVENT_ALLOC_REALLOC
    } p101_tool_event_alloc_kind;

    typedef enum
    {
        P101_TOOL_EVENT_CALL_ENTER = 0,
        P101_TOOL_EVENT_CALL_EXIT
    } p101_tool_event_call_kind;

    typedef enum
    {
        P101_TOOL_EVENT_RESOURCE_ACQUIRE = 0,
        P101_TOOL_EVENT_RESOURCE_RELEASE,
        P101_TOOL_EVENT_RESOURCE_REPLACE,
        P101_TOOL_EVENT_RESOURCE_TRANSFER
    } p101_tool_event_resource_kind;

    struct p101_tool_event_output
    {
        int                           version;
        p101_tool_event_record_kind   record_kind;
        const char                   *run_id;
        long                          pid;
        long                          child_pid;
        size_t                        context_id;
        size_t                        sequence;
        size_t                        monotonic_ns;
        size_t                        wall_unix_ns;
        int                           monotonic_ns_available;
        int                           wall_unix_ns_available;
        int                           fd;
        int                           cloexec;
        p101_tool_event_fd_kind       fd_kind;
        p101_tool_event_alloc_kind    alloc_kind;
        p101_tool_event_call_kind     call_kind;
        p101_tool_event_resource_kind resource_kind;
        const char                   *ptr;
        const char                   *new_ptr;
        const char                   *target;
        const char                   *resource_class;
        const char                   *resource_id;
        const char                   *related_id;
        const char                   *metadata;
        size_t                        size;
        int                           line_number;
        const char                   *function_name;
        const char                   *call_name;
        const char                   *arguments;
        const char                   *result;
        const char                   *file_name;
        size_t                        events_attempted;
        int                           write_failed;
        int                           write_errno;
    };

    int         p101_tool_event_write(FILE *stream, const struct p101_tool_event_output *record);
    const char *p101_record_event_magic(p101_tool_event_record_kind kind);
    const char *p101_record_event_fd_kind_name(p101_tool_event_fd_kind kind);
    const char *p101_record_event_alloc_kind_name(p101_tool_event_alloc_kind kind);
    const char *p101_record_event_call_kind_name(p101_tool_event_call_kind kind);
    const char *p101_record_event_resource_kind_name(p101_tool_event_resource_kind kind);

#ifdef __cplusplus
}
#endif

#endif
