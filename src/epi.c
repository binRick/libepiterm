#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
/**/
#include "../../env/env.h"
/**/
#include "../../log/log.c"
#include "../../string/stringbuffer.c"
#include "../../vtparse/vtparse/vtparse.c"
#include "../../vtparse/vtparse/vtparse.h"
#include "../../vtparse/vtparse/vtparse_table.c"
#include "../../vtparse/vtparse/vtparse_table.h"
/**/
#include "libepiterm/macros.h"
#include <libepiterm.h>
/**/
#define VTPARSER_ENABLED     1
#define DEFAULT_LOG_LEVEL    LOG_DEBUG
#define PTY_USER_DATA        "MY_PTY_123"
#define FD_LOG               "/tmp/ep.log"
#define JUSH                 "/usr/local/bin/jush"
#define BASH                 "/bin/bash"
#define FISH                 "/bin/fish"
#define SH                   "/bin/sh"
#define SHELL                BASH
/**/
int *fd_log;
/**/

unsigned char       buf[1024];
unsigned int        buf_pos, did_read, i;
unsigned char const *data;
vtparse_t           parser;


void init_vtparse(){
  vtparse_init(&parser);
}


int get_log_fd(){
  return(open(FD_LOG, O_CREAT | O_TRUNC | O_WRONLY));
}


static char * pty_callback(libepiterm_pty_t *pty){
  pty->user_data = strdup(PTY_USER_DATA);
  log_debug(">hypo? %s "
            " | "
            "utempted? %s | master/slave: %d/%d"
            " | "
            "fd_log:%d"
            " | "
            "tty: %s | user_data: %s | pty callback for pid %zu",
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
  buf_pos       = 0;
  *write_buffer = read_buffer;
  *write_size   = read_size;
  log_info("Read %zu bytes from pty", read_size);
  int bytes_written = write(fd_log, (const void *)read_buffer, read_size);


  void print_vtparser_response(){
    if (parser.num_intermediate_chars > 0) {
      log_debug(
        AC_MAGENTA "%d Intermediate chars:" AC_RESETALL
        "",
        parser.num_intermediate_chars
        );
      for (i = 0; i < parser.num_intermediate_chars; i++) {
        log_debug(
          AC_RED "  0x%02x ('%c')" AC_RESETALL
          "", parser.intermediate_chars[i],
          parser.intermediate_chars[i]
          );
      }
    }

    if (parser.num_params > 0) {
      log_trace("%d Parameters:", parser.num_params);
      for (i = 0; i < parser.num_params; i++) {
        log_trace("\t%d", parser.params[i]);
      }
    }
  }


  void read_vtparser_response(){
    log_trace("Received action %s", vtparse_action_str(parser.action));
    if (parser.data_begin != parser.data_end) {
      for (data = parser.data_begin; data < parser.data_end; data++) {
        log_trace(
          AC_RESETALL AC_GREEN "Char: 0x%02x ('%c')"
          "",
          *data, *data
          );
      }
    } else if (parser.ch != 0) {
      log_debug(
        AC_RESETALL AC_BLUE "Char: 0x%02x ('%c')" AC_RESETALL
        "",
        parser.ch, parser.ch
        );
    }else{
      log_trace("vtparse> unhandled! parser.data_begin:%d | parser.data_end: %d", parser.data_begin, parser.data_end);
    }
    print_vtparser_response();
  }


  void vtparser(){
    do {
      buf_pos += vtparse_parse(&parser, read_buffer + buf_pos, read_size - buf_pos);
      if (!vtparse_has_event(&parser)) {
        log_trace("vtparser has no events!");
        break;
      }else{
        log_trace("vtparser has events!");
        read_vtparser_response();
      }
      log_trace("vtparser parsed %d/%d bytes", buf_pos, read_size);
    } while (read_size > 0);
  }

  if (VTPARSER_ENABLED == 1) {
    vtparser();
  }
/*
 */

  log_debug("wrote %d bytes to %s.......", bytes_written, FD_LOG);
  return(0);

  (void)from_epiterm;
} /* io_callback */


int main(void){
  log_set_level(DEFAULT_LOG_LEVEL);
  init_vtparse();
  fd_log = get_log_fd();
  try(libepiterm_121(SHELL, pty_callback, io_callback));
  return(0);

fail:
  perror("libepiterm");
  return(1);
}

