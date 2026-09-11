#include <stdio.h>
#include <string.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include "syscalls.h"

void print_syscall(struct user_regs_struct *regs, int mem_fd);
// safely append the formatted string to buf using *off as offset inside buf.
// *off is incremented by the written bytes and truncated to cap-1 to prevent overflows.
void appends(char *buf, size_t cap, size_t *off, const char *fmt, ...);

enum { RS_FAULT, RS_OK, RS_TRUNC };
int read_cstring(int mem_fd, unsigned long long addr, int max, char *out, int *outlen);

int main(int argc, char *argv[], char *envp[]) {
  pid_t child = -1;
  int status = -1, len = 0, mem_file_fd = -1;
  char *target = NULL;
  char **child_argv = NULL;
  char buff[1024], mem_file_path[32];
  struct user_regs_struct regs;

  if(argc < 2){
    fprintf(stderr, "Usage: %s BINARY [ARGS...]\n", *argv);
    return 1;
  }

  target = argv[1];
  child_argv = &argv[1];

  child = fork();

  if(child == -1) {
    perror("fork()");
    return 1;
  } else if(child == 0) {
    // long ptrace(op, pid_t pid, addr, data)
    if(ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
      perror("ptrace()");
      return 1;
    }
    execve(target, child_argv, envp);

    perror("execve()");
    return 1;
  }

  // kernel sends SIGTRAP to child at the end of execve, so there is no
  // timing issue
  waitpid(child, &status, 0);

  fprintf(stdout, "Child created with PID = %d\n", child);
  snprintf(mem_file_path, sizeof(mem_file_path), "/proc/%d/mem", child);
  mem_file_fd = open(mem_file_path, O_RDONLY);

  if(mem_file_fd < 0) {
    perror("open()");
    return 1;
  }

  while(1) {
    // entering the syscall
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status)) break;

    ptrace(PTRACE_GETREGS, child, 0, &regs);
    print_syscall(&regs, mem_file_fd);

    // exiting the syscall
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status)) {
      write(2, "\n", 1);
      break;
    }

    ptrace(PTRACE_GETREGS, child, 0, &regs);
    len = snprintf(buff, sizeof(buff), " = %lld\n", (long long)regs.rax);
    write(2, buff, len);
  }

  if(WIFEXITED(status)) {
    printf("Child process exited with status %d\n", WEXITSTATUS(status));
  } else if(WIFSIGNALED(status)) {
    printf("Child process was terminated by signal %d\n", WTERMSIG(status));
  }

  close(mem_file_fd);

  return 0;
}

void print_syscall(struct user_regs_struct *regs, int mem_fd) {
  const syscall_info *si = NULL;
  long nr = (long)regs->orig_rax;
  char buff[1024], string_buff[64];
  int i = 0, nread = 0, read_success = RS_FAULT;
  size_t off = 0;
  unsigned long long params[6] = {regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9};

  si = syscall_lookup(nr);

  if(si == NULL) {
    // if the syscall is unknown print a generic firm
    appends(buff, sizeof(buff), &off, "<syscall-%ld>", nr);
    appends(buff, sizeof(buff), &off, "(0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx)", regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9);
  } else {
    appends(buff, sizeof(buff), &off, "%s(", si->name);
    while(i < 6 && si->args[i] != SATYPE_NONE) {
      if(i > 0) appends(buff, sizeof(buff), &off, ", ");
      switch(si->args[i]){
        case SATYPE_FLAGS:
          appends(buff, sizeof(buff), &off, "0x%llx", params[i]);
          break;
        case SATYPE_UNSIGNED:
          appends(buff, sizeof(buff), &off, "%llu", params[i]);
          break;
        case SATYPE_SIGNED:
          appends(buff, sizeof(buff), &off, "%d", (int)params[i]);
          break;
        case SATYPE_SIGNED64:
          appends(buff, sizeof(buff), &off, "%lld", params[i]);
          break;
        case SATYPE_STRING:
          read_success = read_cstring(mem_fd, params[i], sizeof(string_buff), string_buff, &nread);
          if(read_success != RS_FAULT) {
            appends(buff, sizeof(buff), &off, "0x%016llx = \"%s\"%s", params[i], string_buff, read_success == RS_OK ? "" : "...");
          } else {
            appends(buff, sizeof(buff), &off, "0x%016llx", params[i]);
          }
          break;
        case SATYPE_HEX:
          appends(buff, sizeof(buff), &off, "0x%llx", params[i]);
          break;
        case SATYPE_OCT:
          appends(buff, sizeof(buff), &off, "0%03llo", params[i]);
          break;
        case SATYPE_NONE:
        default:
          break;
      }
      i++;
    }
    appends(buff, sizeof(buff), &off, ")");
  }

  write(2, buff, off);
}

int read_cstring(int mem_fd, unsigned long long addr, int max, char *out, int *outlen) {
  ssize_t nread = 0;

  if(addr == 0) {
    *outlen = 0;
    out[0] = '\0';
    return RS_FAULT;
  }

  nread = pread(mem_fd, out, (size_t)max - 1, (off_t)addr);

  if(nread <= 0) {
    *outlen = 0;
    out[0] = '\0';
    return RS_FAULT;
  }

  // check if in the bytes read there is the null char ...
  char *null = memchr(out, '\0', (size_t)nread);
  // ... if found, return the read string
  if(null != 0) {
    // set the actual string length
    *outlen = null - out;
    return RS_OK;
  }

  out[nread] = '\0';
  *outlen = nread;
  return RS_TRUNC;
}

void appends(char *buf, size_t cap, size_t *off, const char *fmt, ...) {
  if(buf == NULL || fmt == NULL || *off >= cap)
    return;
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf+*off, cap-*off, fmt, ap);
  va_end(ap);

  if(n > 0) {
    *off += (size_t)n;
    // truncate the lenght
    if(*off >= cap) *off = cap-1;
  }
}
