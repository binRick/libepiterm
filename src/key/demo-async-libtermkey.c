#define _XOPEN_SOURCE    600
/**/
#include "../../../chan/chan.c"
#include "../../../chan/queue.c"
#include "../../../log/log.c"
/**/
#include <poll.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
/**/

#include "../../../env/env.h"
#include "termkey.h"
/**/
#define KEY_SIZE    50
#define DEBUG       -1
#define SELECTOR    ""
/**/
void * f16_handler(void *, void *);

char *CALLBACK_FUNCTIONS[] = {
  f16_handler
};
char *F16[] = {
  "Alt-Ctrl-Shift-F16",
  "Alt-Ctrl-Shift-",
  "Ctrl-Shift-F12",
  "Alt-Ctrl-Shift-"
};

char *SELECTORS[] = {
  F16
};
/**/
typedef struct {
  char     *name;
  size_t   matches_qty;
  unsigned last_active;
  bool     active;
  void ** (*fn)(void *ctx, void *arg);
  char     *matches[];
} SelectorMatch;

int                  QTY                 = 1;
struct SelectorMatch *selector_matches[] = {
  { "f16", 0, 0, true, CALLBACK_FUNCTIONS[0], SELECTORS }
};

/**/
char *SELECTOR_ENV, *DEBUG_MODE_ENV;


static void on_key(TermKey *tk, TermKeyKey *key){
  char buffer[50];

  termkey_strfkey(tk, buffer, sizeof buffer, key, TERMKEY_FORMAT_LONGMOD);

  for (int ii = 0; ii < QTY; ii++) {
    if (!selector_matches[ii]) {
      continue;
    }else{
      fprintf(stderr, "selector_match #%d -> %s\n", ii, selector_matches[ii]);
    }
  }
//struct SelectorMatch* ptr = selector_matches;
//struct SelectorMatch* endPtr = selector_matches + sizeof(selector_matches)/sizeof(selector_matches[0]);

/*
 * for (size_t s = 0; s < sizeof(selector_matches) / sizeof(selector_matches[0]); s++) {
 *  fprintf(stderr, "selector_match #%d\n", s);
 * //    for (size_t i = 0; i < sizeof(selector_matches[s]->matches) / sizeof(selector_matches[s]->matches[0]); s++) {
 * if (strcmp(buffer, SELECTORS[ii]) == 0) {
 *      fprintf(stderr, "MATCHE SELECTOR #%d!    ->       %s\n", i, buffer);
 *    }else{
 *      if (DEBUG == 1) {
 *        fprintf(stderr, "\nUNMATCHE SELECTOR #%d!    ->       %s\n", i, buffer);
 *      }
 * //    }
 * }
 * }
 */
}


void termkey_selector_monitor(){
  TERMKEY_CHECK_VERSION;
  SELECTOR_ENV   = env_get_or("SELECTOR", "");
  DEBUG_MODE_ENV = env_get_or("DEBUG", "");
  TermKey *tk = termkey_new(0, 0);

  if (!tk) {
    fprintf(stderr, "Cannot allocate termkey instance\n");
    exit(1);
  }

  struct pollfd fd;

  fd.fd     = 0; /* the file descriptor we passed to termkey_new() */
  fd.events = POLLIN;

  TermKeyResult ret;
  TermKeyKey    key;

  int           running  = 1;
  int           nextwait = -1;

  while (running) {
    if (poll(&fd, 1, nextwait) == 0) {
      // Timed out
      if (termkey_getkey_force(tk, &key) == TERMKEY_RES_KEY) {
        on_key(tk, &key);
      }
    }

    if (fd.revents & (POLLIN | POLLHUP | POLLERR)) {
      termkey_advisereadable(tk);
    }

    while ((ret = termkey_getkey(tk, &key)) == TERMKEY_RES_KEY) {
      on_key(tk, &key);

      if (  key.type == TERMKEY_TYPE_UNICODE
         && key.modifiers & TERMKEY_KEYMOD_CTRL
         && (key.code.codepoint == 'C' || key.code.codepoint == 'c')) {
        running = 0;
      }
    }

    if (ret == TERMKEY_RES_AGAIN) {
      nextwait = termkey_get_waittime(tk);
    }else{
      nextwait = -1;
    }
  }

  termkey_destroy(tk);
} /* termkey_selector_monitor */


void * f16_handler(void *abc, void *def){
  log_debug("f16.....................");
}


int main(int argc, char *argv[]){
  termkey_selector_monitor();
}

