#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
/**/
//#include "/root/bash-loadable-wireguard/src/io/flingfd.c"
//#include "/root/bash-loadable-wireguard/src/io/flingfd.h"
/**/
#include "libepiterm/macros.h"
#include <libepiterm.h>
/**/
#define PTY_USER_DATA    "MY_PTY_123"
#define FD_LOG           "/tmp/ep.log"
#define JUSH             "/usr/local/bin/jush"
#define BASH             "/bin/bash"
#define FISH             "/bin/fish"
#define SH               "/bin/sh"
#define SHELL            BASH
/**/
int *fd_log;
/**/


int get_log_fd(){
  return(open(FD_LOG, O_CREAT | O_TRUNC | O_WRONLY));
}


static char * pty_callback(libepiterm_pty_t *pty){
  pty->user_data = strdup(PTY_USER_DATA);
  fprintf(stderr,
          ">hypo? %s "
          " | "
          "utempted? %s | master/slave: %d/%d"
          " | "
          "fd_log:%d"
          " | "
          "tty: %s | user_data: %s | pty callback for pid %zu\n",
          ((pty->is_hypo) == 1 ? "Yes" : "No"),
          ((pty->utempted) == 1 ? "Yes" : "No"),
          fd_log,
          pty->master, pty->slave,
          pty->tty,
          pty->user_data,
          pty->pid
          );
  return(NULL);
}


static int io_callback(int from_epiterm, char *read_buffer, size_t read_size,
                       char ** restrict write_buffer, size_t * restrict write_size){
  *write_buffer = read_buffer;
  *write_size   = read_size;

  size_t bytes_written = write(fd_log, (const void *)read_buffer, read_size);

  fprintf(stderr, "Read %zu bytes | wrote %zu bytes to %d/%s.......\n", read_size, bytes_written, fd_log, FD_LOG);
  return(0);

  (void)from_epiterm;
}

#define FD_PATH    "/tmp/fdfling1"


int main(void){
  fd_log = get_log_fd();
  try(libepiterm_121(SHELL, pty_callback, io_callback));
  return(0);

fail:
  perror("libepiterm");
  return(1);
}

