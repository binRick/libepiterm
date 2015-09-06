#define _XOPEN_SOURCE  700
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <utempter.h> /* -lutempter */



#define t(...)  do if (called = #__VA_ARGS__, __VA_ARGS__) goto fail; while (0)
static const char* called = NULL;



static volatile sig_atomic_t winched = 0;
static volatile sig_atomic_t chlded = 0;


static void sigwinch(int signo)
{
  winched = 1;
  (void) signo;
}

static void sigchld(int signo)
{
  chlded = 1;
  (void) signo;
}


static void copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  ioctl(whence,  TIOCGWINSZ, &winsize);
  ioctl(whither, TIOCSWINSZ, &winsize);
}


int main(void)
{
  int master, slave, sttyed = 0, r, n;
  const char* slavename;
  struct sigaction action;
  sigset_t sigset;
  pid_t pid;
  const char* shell;
  struct passwd* pw;
  struct termios termios;
  struct termios saved_termios;
  fd_set fds;
  char buffer[1024];
  ssize_t got;
  
  shell = getenv("SHELL");
  if ((shell == NULL) || (access(shell, X_OK) < 0))
    {
      pw = getpwuid(getuid());
      if (pw)
	shell = pw->pw_shell;
    }
  if (shell == NULL)
    shell = "/bin/sh";
  
  t (master = posix_openpt(O_RDWR | O_NOCTTY), master < 0);
  t (grantpt(master));
  t (unlockpt(master));
  t (slavename = ptsname(master), slavename == NULL);
  t (slave = open(slavename, O_RDWR | O_NOCTTY), slave < 0);
  
  t (pid = fork(), pid < 0);
  if (pid == 0)
    goto child;
  
  t (tcgetattr(STDIN_FILENO, &termios));
  t (tcsetattr(master, TCSANOW, &termios));
  copy_winsize(master, STDIN_FILENO);
  saved_termios = termios;
  cfmakeraw(&termios);
  t (tcsetattr(STDIN_FILENO, TCSANOW, &termios));
  sttyed = 1;
  
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  t (sigaction(SIGWINCH, &action, NULL));
  
  sigemptyset(&sigset);
  action.sa_handler = sigchld;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  t (sigaction(SIGCHLD, &action, NULL));
  
  t (!utempter_add_record(master, "libepiterm"));
  
  while (!chlded)
    {
      if (winched)
	{
	  winched = 0;
	  copy_winsize(master, STDIN_FILENO);
	}
      
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      FD_SET(master, &fds);
      
      t (r = select(master + 1, &fds, NULL, NULL, NULL), r < 0);
      
      if (FD_ISSET(master, &fds))
	{
	  t (got = read(master, buffer, sizeof(buffer)), got <= 0);
	  t (write(STDOUT_FILENO, buffer, (size_t)got) < 0);
	}
      if (FD_ISSET(STDIN_FILENO, &fds))
	{
	  t (got = read(STDIN_FILENO, buffer, sizeof(buffer)), got <= 0);
	  t (write(master, buffer, (size_t)got) < 0);
	}
    }
  
  t (tcsetattr(STDIN_FILENO, TCSADRAIN, &saved_termios));
  t (close(master));
  return 0;
  
#undef t
#define t(...)  if (called = #__VA_ARGS__, __VA_ARGS__) goto cfail
 child:
  for (r = 0, n = getdtablesize(); r < n; r++)
    if ((r != slave) && (r != 64))
      close(r);
  t (setsid() < 0);
  t (ioctl(slave, TIOCSCTTY, 0));
  t ((slave != STDIN_FILENO) && dup2(slave, STDIN_FILENO) < 0);
  t ((slave != STDIN_FILENO) && close(slave));
  t (dup2(STDIN_FILENO, STDOUT_FILENO) < 0);
  t (dup2(STDIN_FILENO, STDERR_FILENO) < 0);
  execl(shell, shell, NULL);
 cfail:
  dprintf(64, "%s: %s: %s\r\n", shell, called, strerror(errno));
  return 1;
  
 fail:
  fprintf(stderr, "%s: %s\r\n", called, strerror(errno));
  if (sttyed)
    tcsetattr(STDIN_FILENO, TCSADRAIN, &saved_termios);
  return 1;
}

