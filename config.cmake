set(PROJECT_NAME "p101_record")
set(PROJECT_VERSION "0.0.1")
set(PROJECT_DESCRIPTION "Bounded record fields and the minimal p101 event wire writer")
set(PROJECT_LANGUAGE "C")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(STANDARD_FLAGS -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Werror)
set(DARWIN_STANDARD_FLAGS -D_DARWIN_C_SOURCE)
set(LINUX_STANDARD_FLAGS -D_GNU_SOURCE)
set(BSD_STANDARD_FLAGS -D_BSD_SOURCE -D__BSD_VISIBLE)

set(LIBRARY_TARGETS p101_record)
set(p101_record_SOURCES src/event_write.c src/record.c)
set(p101_record_HEADERS include/p101_record/event.h include/p101_record/record.h)
set(p101_record_LINK_LIBRARIES "")
