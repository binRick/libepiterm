#ifndef LIBEPITERM_PTY_H
#define LIBEPITERM_PTY_H

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>

typedef struct libepiterm_pty
{
  int is_hypo;
  int master;
  void* user_data;
  /* Order of the above is important. */
  int slave;
  pid_t pid;
  char* tty;
  
} libepiterm_pty_t;

int libepiterm_pty_create(libepiterm_pty_t* restrict pty, int use_path, const char* shell, char* const argv[],
			  char *const envp[], char* (*get_record_name)(libepiterm_pty_t* pty),
			  struct termios* restrict termios, struct winsize* restrict winsize);
void libepiterm_pty_close(libepiterm_pty_t* restrict pty);

#endif

