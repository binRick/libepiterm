#define _GNU_SOURCE
#include "loop.h"
#include "macros.h"

#include <alloca.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>



static volatile sig_atomic_t winched = 0;
static volatile sig_atomic_t chlded = 0;



static void sigwinch()
{
  winched = 1;
}


static void sigchld()
{
  chlded = 1;
}


static ssize_t uninterrupted_write(int fd, void* buffer, size_t size)
{
  ssize_t wrote;
  size_t ptr = 0;
  
  while (ptr < size)
    {
      wrote = write(fd, ((char*)buffer) + ptr, size - ptr);
      if (wrote < 0)
	{
	  if (errno == EINTR)
	    continue;
	  return -1;
	}
      ptr += (size_t)wrote;
    }
  
  return (ssize_t)ptr;
}



int libepiterm_loop(libepiterm_term_t** restrict terms, size_t termn,
		    int (*io_callback)(libepiterm_term_t* restrict read_term, char* read_buffer,
				       size_t read_size, int* restrict write_fd,
				       char** restrict write_buffer, size_t* restrict write_size),
		    int (*winch_callback)(void))
{
  size_t i, length;
  int fd, events_max, epoll_fd = -1;
  struct epoll_event* events;
  struct sigaction action;
  libepiterm_term_t* event;
  char buffer[1024];
  char* output;
  ssize_t n, got;
  sigset_t sigset;
  int saved_errno;
  
  events_max = termn > (size_t)INT_MAX ? INT_MAX : (int)termn;
  events = alloca((size_t)events_max * sizeof(struct epoll_event));
  
  memset(events, 0, sizeof(struct epoll_event));
  events->events = EPOLLIN;
  try (epoll_fd = epoll_create1(0));
  for (i = 0; i < termn; i++)
    try (events->data.ptr = terms[i], epoll_ctl(epoll_fd, EPOLL_CTL_ADD, terms[i]->pty.master, events));
  
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGWINCH, &action, NULL));
  
  sigemptyset(&sigset);
  action.sa_handler = sigchld;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGCHLD, &action, NULL));
  
  while (!chlded)
    {
      if (winched)
	{
	  winched = 0;
	  try (winch_callback());
	}
      
      n = (ssize_t)epoll_wait(epoll_fd, events, events_max, -1);
      if ((n < 0) && (errno == EINTR))
	continue;
      fail_if (n < 0);
      
      while (n--)
	{
	  event = events[n].data.ptr;
	  try (got = TEMP_FAILURE_RETRY(read(event->hypo.in, buffer, sizeof(buffer))));
	  if (got == 0)
	    continue;
	  try (io_callback(event, buffer, (size_t)got, &fd, &output, &length));
	  try (uninterrupted_write(fd, output, length));
	}
    }
  
  close(epoll_fd);
  return 0;
 fail:
  saved_errno = errno;
  if (epoll_fd >= 0)
    close(epoll_fd);
  return errno = saved_errno, -1;
}
