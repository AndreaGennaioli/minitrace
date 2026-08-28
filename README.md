# minitrace

`minitrace` is a didactic (and simple) strace-like syscall tracer written in C for Linux x86-64 systems.

Currently I'm just experimenting with `ptrace` and I don't have a clear roadmap.

## Build and run

```bash
make
# usage: mt BINARY [ARGS...]
./mt /bin/ls ~
# clean
make clean
```
