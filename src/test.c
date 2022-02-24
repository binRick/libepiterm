#include <stdio.h>
/**/
#include "libepiterm/macros.h"
#include <libepiterm.h>
/**/
#define PTY_CB    0
#define JUSH      "/usr/local/bin/jush"
#define BASH      "/bin/bash"
#define FISH      "/bin/fish"
#define SH        "/bin/sh"
/**/
#define SHELL     BASH
/**/

static char * pty_callback(libepiterm_pty_t *pty){
  pty->user_data = strdup("MYDATA");
  fprintf(stderr,
          ">hypo? %s "
          " | "
          "utempted? %s | master/slave: %d/%d | tty: %s | user_data: %s | pty callback for pid %zu\n",
          ((pty->is_hypo) == 1 ? "Yes" : "No"),
          ((pty->utempted) == 1 ? "Yes" : "No"),
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
  fprintf(stderr, "Read %zu bytes.......\n", read_size);
  return(0);

  (void)from_epiterm;
}


int main(void){
  if (PTY_CB == 0) {
    try(libepiterm_121(SHELL, pty_callback, io_callback));
  }else{
    try(libepiterm_121(SHELL, NULL, io_callback));
  }
  return(0);

fail:
  perror("libepiterm");
  return(1);
}

