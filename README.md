# minitrace

`minitrace` is a didactic (and simple) strace-like syscall tracer written in C for Linux x86-64 systems.

<p align="center">
  <img src="assets/demo.gif" alt="minitrace tracing cat" width="720">
  <br/>
  <sub><code>minitrace</code> tracing <code>cat</code></sub>
</p>

Work in progress: see below what this tracer can and cannot do.

## Build and run

Requirements: Linux x86-64, gcc, make.

```bash
make
# usage: mt BINARY [ARGS...]
./mt /bin/ls ~
# clean
make clean
```

## Example

Example of `minitrace` tracing `ls`:

```
$ ./mt /bin/ls
Child created with PID = 93679
newfstatat(-100, 0x00007f29f167d47e = "/etc/ld.so.cache", 0x7ffd56011ff0, 0x0) = 0
openat(-100, 0x00007f29f167d47e = "/etc/ld.so.cache", 0x80000, 0000) = 3
fstat(3, 0x7ffd56011f60) = 0
...
brk(0x561017cb7000) = 94627118673920
openat(-100, 0x00007f29f13c2da0 = "/usr/lib/locale/locale-archive", 0x80000, 0000) = 3
fstat(3, 0x7f29f141a7c0) = 0
mmap(0x0, 3064432, 0x1, 0x2, 3, 0) = 139818111598592
close(3) = 0
ioctl(1, 0x802c542a, 0x7ffd56013490) = 0
ioctl(1, 0x5413, 0x7ffd56013530) = 0
openat(-100, 0x0000561017c9c710 = ".", 0x90800, 0000) = 3
fstat(3, 0x7ffd56013090) = 0
<syscall-217>(0x3, 0x561017c9c760, 0x8000, 0x8030, 0x561017c9c734, 0x8040) = 304
<syscall-217>(0x3, 0x561017c9c760, 0x8000, 0xa, 0x561017c9c734, 0x20) = 0
close(3) = 0
fstat(1, 0x7ffd56010f00) = 0
write(1, 0x0000561017c9c980 = "assets	LICENSE  Makefile  mt  obj  README.md  src
", 50)assets	LICENSE  Makefile  mt  obj  README.md  src
 = 50
close(1) = 0
close(2) = 0
exit_group(0)Child process exited with status 0

```
## What minitrace can do

- Trace the syscall flow and identify the most common syscalls (86 currently).
- Correctly distinguish between signal-delivery stops and syscall stops.
- Print the syscall arguments in different formats (hex, integer, unsigned integer, string, ...) knowing the syscall's name.
- Print the first characters of a string used as a parameter. This is achieved by correctly reading `/proc/<childpid>/mem`.

## Known problems to be fixed

- The buffer argument of `write` is typed as SATYPE_STRING even if it could also contain bytes. Could be fixed implementing the SATYPE_BUFFER type.

## What minitrace cannot do...

... and is planned to be implemented:

- Return type for each syscall.
- Print errno in case of error.
