# minitrace

`minitrace` is a didactic (and simple) strace-like syscall tracer written in C for Linux x86-64 systems.

Work in progress: see below what this tracer can and cannot do.

## Requirements

Linux x86-64, gcc, make.

## Build and run

```bash
make
# usage: mt BINARY [ARGS...]
./mt /bin/ls ~
# clean
make clean
```

## What minitrace can do

- Trace the syscall flow and identify the most common syscalls (84 currently).
- Print the syscall arguments in different formats (hex, integer, unsigned integer, string, ...) knowing the syscall's name.
- Print the first characters of a string used as a parameter. This is achieved by correctly reading `/proc/<childpid>/mem`.

## Known problems to be fixed

- Currently the main loop works only if no signals are received since it strictly expects stops to alternate between syscall entry and exit.
- The tracer should distinguish syscall stops from signal-delivery stops and, in the latter case, reinject the signal into the child.
- The buffer argument of `write` is typed as SATYPE_STRING even if it could also contain bytes. Could be fixed implementing the SATYPE_BUFFER type.

## What minitrace cannot do...

... and is planned to be implemented:

- Return type for each syscall.
- Print errno in case of error.
