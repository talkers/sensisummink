/* Ew-too guardian angel  ( the newer one) */

#include "include/config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#if defined( LINUX ) && defined( GLIBC )
#define __STRICT_ANSI__
#include <sys/socket.h>
#undef __STRICT_ANSI__
#else
#include <sys/socket.h>
#endif
#include <sys/un.h>
#include <sys/ioctl.h>
#if !defined ( LINUX ) && !defined ( OSF )
#include <sys/filio.h>
#endif /* LINUX */
#ifdef IRIX
#include <bstring.h>
#endif

#ifdef SUNOS
#include <memory.h>
#define TIME_DEFINES
#define SOCKET_DEFINES
#endif

#include "include/missing_headers.h"

char		angel_name[256];
char		server_name[256];
/*char           *angel_name = "-=> SensiSummink <=- Guardian Angel watching "
                               "port";
char           *server_name = "-=> SensiSummink <=- Server on port";
*/
char           *stack, *stack_start;
int            fh = 0, die = 0, crashes = 0, syncing = 0;
long int       time_out = 0, t = 0;
int no_tty=0;
char		*sys_time(void);

/* return a string of the system time */

char *sys_time()
{
   time_t st;
   static char time_string[25];

   st = time(0);
   strftime(time_string, 25, "%H:%M:%S - %d/%m/%y", localtime(&st));
   return time_string;
}


/* log errors and things to file */

void log(char *file, char *string)
{
   int fd, length;

   sprintf(stack, "logs/%s.log", file);
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   length = lseek(fd, 0, SEEK_END);
   if (length > MAX_LOG_SIZE)
   {
      close(fd);
#if defined( FREEBSD )
      fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
      fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
                
   }
   sprintf(stack, "%s - %s\n", sys_time(), string);
   if (!no_tty)
      printf(stack);
   write(fd, stack, strlen(stack));
   close(fd);
}


void error(char *str)
{
   log("angel", str);
   exit(-1);
}

void sigpipe(int dummy)
{
   error("Sigpipe received.");
}
void sighup(int dummy)
{
   kill(fh, SIGHUP);
   die = 1;
}
void sigquit(int dummy)
{
   error("Quit signal received.");
}
void sigill(int dummy)
{
   error("Illegal instruction.");
}
void sigfpe(int dummy)
{
   error("Floating Point Error.");
}
void sigbus(int dummy)
{
   error("Bus Error.");
}
void sigsegv(int dummy)
{
   error("Segmentation Violation.");
}
void sigsys(int dummy)
{
   error("Bad system call.");
}
void sigterm(int dummy)
{
   error("Terminate signal received.");
}
void sigxfsz(int dummy)
{
   error("File descriptor limit exceeded.");
}
void sigchld(int dummy)
{
   log("angel", "Received SIGCHLD");
   return;
}


/* Woo woo, the main function thang */

void main(int argc, char *argv[])
{
   int status;
   char binary_path[256];
   int alive_fd=-1, sock_fd, dieing;
#ifdef GLIBC
    unsigned int	length;
#else
    int			length;
#endif
   FILE *angel_pid_fd;
   struct sockaddr_un sa;
   char dummy;
   fd_set fds;
   struct timeval timeout;
#ifdef USE_SIGACTION
   struct sigaction siga;
#endif /* use_sigaction */

   /* process names */
   memset(binary_path, 0, 256);
   memset(angel_name, 0, 256);
   memset(server_name, 0, 256);
   sprintf(binary_path, "bin/%s", TALKER_BINARY);
   sprintf(angel_name, "-=> %s <=- Guardian Angel watching port", TALKER_NAME);
   sprintf(server_name, "-=> %s <=- Server on port", TALKER_NAME);
   stack_start = (char *) malloc(1000);
   stack = stack_start;

   if (chdir(ROOT) < 0)
      error("Can't change to root directory.\n");

   if (strcmp(angel_name, argv[0]))
   {
      if (!argv[1])
      {
         sprintf(stack, "%d", DEFAULT_PORT);
         execlp("bin/angel", angel_name, stack, 0);
      } else
      {
         argv[0] = angel_name;
         execvp("bin/angel", argv);
      }
      error("exec failed");
   }
   if (nice(5) < 0)
      error("Failed to renice");

   /* maybe do the pid file here? */
   unlink(ANGEL_PID);
   if(!(angel_pid_fd = fopen(ANGEL_PID,"w"))) {
     fprintf(stderr, "Unable to create pid log file %s\n", ANGEL_PID);
     exit(1);
   }
   fprintf(angel_pid_fd, "%d\n", (int) getpid());
   fclose(angel_pid_fd);
   
   t = time(0);
   time_out = t + 60;

#ifdef USE_SIGACTION
   siga.sa_handler = sigpipe;
#ifdef USE_SIGEMPTYSET
    sigemptyset(&siga.sa_mask);
#else
    siga.sa_mask = 0;
#endif
   siga.sa_flags = 0;
   sigaction(SIGPIPE, &siga, 0);
   siga.sa_handler = sighup;
   sigaction(SIGHUP, &siga, 0);
   siga.sa_handler = sigquit;
   sigaction(SIGQUIT, &siga, 0);
   siga.sa_handler = sigill;
   sigaction(SIGILL, &siga, 0);
   siga.sa_handler = sigfpe;
   sigaction(SIGFPE, &siga, 0);
   siga.sa_handler = sigbus;
   sigaction(SIGBUS, &siga, 0);
   siga.sa_handler = sigsegv;
   sigaction(SIGSEGV, &siga, 0);
#ifndef NO_SIGSYS
   siga.sa_handler = sigsys;
   sigaction(SIGSYS, &siga, 0);
#endif /* no_sigsys */
   siga.sa_handler = sigterm;
   sigaction(SIGTERM, &siga, 0);
   siga.sa_handler = sigxfsz;
   sigaction(SIGXFSZ, &siga, 0);
   siga.sa_handler = sigchld;
   sigaction(SIGCHLD, &siga, 0);
#else /* use_sigaction */
   signal(SIGPIPE, sigpipe);
   signal(SIGHUP, sighup);
   signal(SIGQUIT, sigquit);
   signal(SIGILL, sigill);
   signal(SIGFPE, sigfpe);
   signal(SIGBUS, sigbus);
   signal(SIGSEGV, sigsegv);
   signal(SIGSYS, sigsys);
   signal(SIGTERM, sigterm);
   signal(SIGXFSZ, sigxfsz);
   signal(SIGCHLD, sigchld);
#endif /* use_sigaction */

   while (!die)
   {
      t = time(0);
      if (crashes >= 4 && time_out >= t)
      {
         log("error", "Total barf.. crashing lots... Giving up");
         log("error", "Question is, what now ??");
         unlink(ANGEL_PID);
         exit(-1);
      } else if (time_out < t)
      {
         time_out = t + 30;
         crashes = 0;
      }
      crashes++;
      log("angel", "Forking to boot server");
      dieing = 0;
      fh = fork();
      switch (fh)
      {
         case 0:
            setsid();
            argv[0] = server_name;
            execvp(binary_path, argv);
            error("failed to exec server");
            break;
         case -1:
            error("Failed to fork()");
            break;
         default:
            no_tty = 1;
            unlink(SOCKET_PATH);
            sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
            if (sock_fd < 0)
               error("failed to create socket");
            sa.sun_family = AF_UNIX;
            strcpy(sa.sun_path, SOCKET_PATH);
#if defined( IRIX )
            if (bind(sock_fd, &sa, sizeof(sa)) < 0)
#else
            if (bind(sock_fd, (struct sockaddr *) & sa, sizeof(sa)) < 0)
#endif
               error("failed to bind");
            if (listen(sock_fd, 1) < 0)
               error("failed to listen");
            timeout.tv_sec = 120;
            timeout.tv_usec = 0;
            FD_ZERO(&fds);
            FD_SET(sock_fd, &fds);
            if (select(FD_SETSIZE, &fds, 0, 0, &timeout) <= 0)
            {
               kill(fh, SIGKILL);
               log("angel", "Killed server before connect");
               waitpid(fh, &status, 0);
            } 
            else
            {
               length = sizeof(sa);
#if defined( IRIX )
               alive_fd = accept(sock_fd, &sa, &length);
#else
               alive_fd = accept(sock_fd, (struct sockaddr *) & sa, &length);
#endif
               if (alive_fd < 0)
                  error("bad accept");
               close(sock_fd);
               while (waitpid(fh, &status, WNOHANG) <= 0)
               {
                  timeout.tv_sec = 300;
                  timeout.tv_usec = 0;
                  FD_ZERO(&fds);
                  FD_SET(alive_fd, &fds);
                  if (select(FD_SETSIZE, &fds, 0, 0, &timeout) <= 0)
                  {
                     if (errno != EINTR)
                     {
                        if (dieing)
                        {
                           kill(fh, SIGKILL);
                           log("angel", "Server KILLED");
                        } else
                        {
                           kill(fh, SIGTERM);
                           log("angel", "Server TERMINATED");
                           dieing = 1;
                        }
                     }
                  } else
                  {
                     if (ioctl(alive_fd, FIONREAD, &length) < 0)
                        error("bad FIONREAD");
                     if (!length)
                     {
                        kill(fh, SIGKILL);
                        log("angel", "Server disconnected");
                        dieing = 1;
                     } else
                     {
                        for (; length; length--)
                        {
                           read(alive_fd, &dummy, 1);
                        }
                     }
                  }
               }
            }
            close(alive_fd);
            switch ((status & 255))
            {
               case 0:
                  log("angel", "Server exited safely");
                  break;
               case 127:
                  sprintf(stack, "Server stopped due to signal %d.",
                          (status >> 8) & 255);
                  while (*stack)
                     stack++;
                  stack++;
                  log("angel", stack_start);
                  stack = stack_start;
                  break;
               default:
                  sprintf(stack, "Server terminated due to signal %d.",
                          status & 127);
                  while (*stack)
                     stack++;
                  stack++;
                  log("angel", stack_start);
                  stack = stack_start;
                  if (status & 128)
                     log("angel", "Core dump produced");
                  break;
            }
         break;
      }
   }
   unlink(ANGEL_PID);
}
