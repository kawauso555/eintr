#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434   /* System call # on most architectures */
#endif

static
void signal_handler(int num)
{
  puts("caught signal");
}

static int
pidfd_open(pid_t pid, unsigned int flags)
{
  return syscall(__NR_pidfd_open, pid, flags);
}

#define TIMEOUT (120 * 1000)	/* TimeOut = 120sec */


int main (int argc, char ** argv)
{
  pid_t pid = fork ();
  int status;
  sigset_t mask;
  int pfd;
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = signal_handler;
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGURG, &sa, NULL) < 0)
    {
      perror("sigaction");
      return -1;
    }

  printf ("my process id:%d\n", getpid());
  
  if (pid == 0)
    {
      /* Child Process */
      execv("/bin/sleep", argv);
    }
  else
    {
      struct pollfd pollfd;
      int pidfd, ready;

      printf ("Created process pid:%d\n", pid);
      
      pfd = pidfd_open(pid, 0);
      if (pfd == -1)
	{
	  perror("pidfd_open");
	  return -1;
	}

      pollfd.fd = pfd;
      pollfd.events = POLLIN;

      ready = poll (&pollfd, 1, TIMEOUT);
      if (ready < 0)
	{
	  perror("poll");
	  return -1;
	}
      else if (ready == 0)
	{
	  printf ("pid:%d Timeout\n",pid);
	}
      else
	{
	  printf ("process: %d done\n", pid);
	}
      /* Parent Process */
      pid_t dpid = waitpid (pid, &status, 0);
      printf ("pid:%d done, exit status: %d\n", dpid,  WEXITSTATUS(status));
      close (pfd);
    }
  return 0;
}
      
