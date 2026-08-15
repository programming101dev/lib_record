# lib_record

`p101_record` is the libc-only foundation for bounded tab-delimited fields and
the minimal versioned event-wire writer used below `lib_env`. It owns encoding,
not event parsing, lifecycle analysis, JSON, diagnostics, or findings policy.

## Example

[`lib_record_examples`](https://github.com/programming101dev/lib_record_examples)
contains the executable examples for this library, including splitting and
parsing a bounded two-field record.
