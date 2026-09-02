#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>

// this struct defines how to stamp the arg (not the specific type).
typedef enum {
  SATYPE_NONE = 0,
  SATYPE_SIGNED,
  SATYPE_SIGNED64,
  SATYPE_UNSIGNED,
  SATYPE_OCT,
  SATYPE_HEX,
  SATYPE_STRING,
  SATYPE_FLAGS,
} arg_type;

typedef struct {
  const char *name;
  // terminated by SATYPE_NONE (0).
  arg_type args[6];
} syscall_info;

const syscall_info *syscall_lookup(long nr);

#endif
