/*
 * ident_server.c
 *
 * An implementation of rfc1413 that does lookups in the background for
 * another program.
 *
 */

/*
 * The majority of the code contained herein is Copyright 1995/1996 by
 * Neil Peter Charley.
 * Portions of the code (notably that for Solaris 2.x and BSD compatibility)
 * are Copyright 1996 James William Hawtin. Thanks also goes to James
 * for pointing out ways to make the code more robust.
 * ALL code contained herein is covered by one or the other of the above
 * Copyright's.
 *
 * Permission to use this code authored by the above persons is given
 * provided:
 *
 *   i) Full credit is given to the author(s).
 *
 *  ii) The code is not re-distributed in ANY form to persons other than
 *     yourself without the express permission of the author(s).
 *     (i.e. you can transfer it from account to account as long as it is
 *      to yourself)
 *
 * iii) You make EVERY effort to forward bugfixes to the author(s)
 *     NB: Neil is co-ordinating any bugfixes that go into this program, so
 *         please try and send them to him first.
 *
 */

#include "include/config.h"
#include "include/ident.h"

#include <unistd.h>
#if defined ( HAVE_FILIOH )
#include <sys/filio.h>
#endif /* HAVE_FILIOH */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef SOLARIS
#ifndef SOLARIS24
#include <strings.h>
#endif
#endif
#include <errno.h>
#include <sys/time.h>
#ifdef DONT_USE_IOCTL
#include <fcntl.h>
#else
#include <sys/ioctl.h>
#endif /* DONT_USE_IOCTL */
#include <sys/socket.h>
#include <sys/resource.h>
#include <signal.h>
#ifdef OSF
#include <stropts.h>
#endif
#ifdef IRIX
#include <bstring.h>
#endif

#ifdef SUNOS
#define VARARG_DEFINES
#define RLIMIT_DEFINES
#define SOCKET_DEFINES
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"

/* Extern Variables */

#ifndef GLIBC
 extern int errno;
#endif

#if !defined( OSF )
#if defined(BSD)
  /* Missing BSD PROTOTYPES */
  extern int	setrlimit(int, struct rlimit *);
  extern int	getrlimit(int, struct rlimit *);
  extern int	setitimer(int, struct itimerval *, struct itimerval *);
  extern int	fclose();
  extern int	select();
  extern time_t   time(time_t *);
  extern int	connect();
  extern int	shutdown(int, int);
  extern int	socket(int, int, int);
  extern int	strftime(char *, int, char *, struct tm *);
  extern void	perror(char *);
  extern int	fflush();
#ifndef DONT_USE_IOCTL
  extern int	ioctl();
#endif /* DONT_USE_IOCTL */
  extern int	fprintf();
  extern void	bzero(void *, int);
  extern void	bcopy(char *, char *, int);
  extern int      vfprintf(FILE *stream, char *format, ...);
  extern int	sigpause(int);
#endif /* BSD */
#else /* OSF */
/* extern int ioctl(int d, unsigned long request, void * arg);*/
#endif /* !OSF */

/* Local Function Prototypes */

void catch_sigterm(SIGTYPE);
void catch_sigalrm(SIGTYPE);
void check_connections(void);
void check_requests(void);
void closedown_request(int slot);
void do_request(ident_request *id_req);
void log(char *, ...);
void process_request(int slot);
void process_result(int slot);
void queue_request(ident_identifier id, short int local_port,
   struct sockaddr_in *sockadd);
void take_off_queue(int freeslot);
void write_request(ident_request *id_req);

/* Local Variables */

#define BUFFER_SIZE 1024
char scratch[4096];
char req_buf[BUFFER_SIZE];
ident_request idents_in_progress[MAX_IDENTS_IN_PROGRESS];
ident_request *first_inq = NULL;
ident_request *last_inq = NULL;
int debug = 0;
int status = 0;
int beats_per_second;
int req_size;

#define STATUS_RUNNING 1
#define STATUS_SHUTDOWN 2

int main(int argc, char *argv[])
{
#if !defined(NOALARM)
   struct itimerval new_timer, old_timer;
#endif /* !NOALARM */
   struct rlimit rlp;
#if defined(USE_SIGACTION)
   struct sigaction sigact;
#endif
   struct sockaddr_in sadd;

   getrlimit(RLIMIT_NOFILE, &rlp);
   rlp.rlim_cur = rlp.rlim_max;
   setrlimit(RLIMIT_NOFILE, &rlp);

   /* First things first, let the client know we're alive */
   write(1, SERVER_CONNECT_MSG, strlen(SERVER_CONNECT_MSG));

   /* Need this to keep in sync with the client side
    *
    * <start><ident_id><local_port><sin_family><s_addr><sin_port>
    */
   req_size = sizeof(char) + sizeof(ident_identifier) + sizeof(short int)
                + sizeof(sadd.sin_family) + sizeof(sadd.sin_addr.s_addr)
                + sizeof(sadd.sin_port);

   /* Set up signal handling */
#if defined( USE_SIGACTION )
#if defined( USE_SIGEMPTYSET )
  sigemptyset(&(sigact.sa_mask));
#if !defined( FREEBSD ) && !defined( GLIBC )
  sigact.sa_sigaction = 0;
#endif
#else
  sigact.sa_mask = 0;
#if defined ( LINUX )
   sigact.sa_restorer = (void *) 0;
#endif
#endif /* USE_SIGEMPTYSET */
 sigact.sa_handler = catch_sigterm;
 sigaction(SIGTERM, &sigact, (struct sigaction *) 0);
#if !defined ( NOALARM )
  sigact.sa_handler = catch_sigalrm;
  sigaction(SIGALRM, &sigact, (struct sigaction *) 0);
#endif /* !NOALARM */
 sigact.sa_handler = SIG_IGN;
 sigaction(SIGPIPE, &sigact, (struct sigaction *) 0);
#else /* !USE_SIGACTION */
 signal(SIGTERM, catch_sigterm);
#if !defined(NOALARM)
  signal(SIGALRM, catch_sigalrm);
#endif /* !NOALARM */
 signal(SIGPIPE, SIG_IGN);
#endif /* USE_SIGACTION */

#if !defined(NOALARM)
   beats_per_second = 5;
   /* Set up a timer to wake us up now and again */
   new_timer.it_interval.tv_sec = 0;
   new_timer.it_interval.tv_usec = 1000000 / beats_per_second;
   new_timer.it_value.tv_sec = 0;
   new_timer.it_value.tv_usec = new_timer.it_interval.tv_usec;
   if (0 > setitimer(ITIMER_REAL, &new_timer, &old_timer))
   {
      perror("ident");
   }
#endif /* !NOALARM */
#if defined(HAVE_BZERO)
#ifdef SUNOS
    bzero((char *)&idents_in_progress[0], MAX_IDENTS_IN_PROGRESS * sizeof(ident_request));
#elif OSF
    bzero((void *)&idents_in_progress[0], MAX_IDENTS_IN_PROGRESS * sizeof(ident_request));
#else
    bzero(&idents_in_progress[0], MAX_IDENTS_IN_PROGRESS * sizeof(ident_request));
#endif
#else /* !HAVE_BZERO */
   memset(&idents_in_progress[0], 0,
        MAX_IDENTS_IN_PROGRESS * sizeof(ident_request));
#endif /* HAVE_BZERO */
   /* Now enter the main loop */
   status = STATUS_RUNNING;
   while (status != STATUS_SHUTDOWN)
   {
      if (1 == getppid())
      {
      /* If our parent is now PID 1 (init) the talker must have died without
       * killing us, so we have no business still being here
       *
       * time to die...
       */
         exit(0);
      }
      check_requests();
      check_connections();
#if !defined(NOALARM)
      sigpause(0);
#endif /* !NOALARM */
   }

   return 0;
}

/* Catch a SIGTERM, this means to shutdown */

void catch_sigterm(SIGTYPE)
{
   status = STATUS_SHUTDOWN;
}

#if !defined(NOALARM)
/* Catch a SIGALRM, we do nothing on this */

void catch_sigalrm(SIGTYPE)
{
}
#endif /* !NOALARM */


/* Check our actual connections out for activity */

void check_connections(void)
{
   fd_set fds_write;
   fd_set fds_read;
   int i;
   struct timeval timeout;
   time_t now;

   FD_ZERO(&fds_write);   /* These are for connection being established */
   FD_ZERO(&fds_read);    /* These are for a reply being ready */

   /*   FD_SET(0, &fds_read);*/
   for (i = 0 ; i < MAX_IDENTS_IN_PROGRESS ; i++)
   {
      if (idents_in_progress[i].local_port)
      {
         if (idents_in_progress[i].flags & IDENT_CONNREFUSED)
	 {
	    process_result(i);
         } else if (!(idents_in_progress[i].flags & IDENT_CONNECTED))
         {
            FD_SET(idents_in_progress[i].fd, &fds_write);
	 } else
         {
            FD_SET(idents_in_progress[i].fd, &fds_read);
         }
      } else
      {
      /* Free slot, so lets try to fill it */
         take_off_queue(i);
      }
   }

#if defined(NOALARM)
   timeout.tv_sec = 1;
   timeout.tv_usec = 0;
#else /* !NOALARM */
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;
#endif /* NOALARM */
   i = select(FD_SETSIZE, &fds_read, &fds_write, 0, &timeout);
   switch (i)
   {
      case -1:
#if defined(DEBUG_IDENT_TOO)
	 fprintf(stderr, "ident: select failed\n");
#endif /* DEBUG_IDENT_TOO */
         break;
      case 0:
         break;
      default:
         for (i = 0 ; i < MAX_IDENTS_IN_PROGRESS ; i++)
         {
            if (FD_ISSET(idents_in_progress[i].fd, &fds_write))
            {
               /* Has now connected, so send request */
               idents_in_progress[i].flags |= IDENT_CONNECTED;
               write_request(&idents_in_progress[i]);
            } else if (FD_ISSET(idents_in_progress[i].fd, &fds_read))
            {
               /* Reply is ready, so process it */
               idents_in_progress[i].flags |= IDENT_REPLY_READY;
               process_result(i);
            }
         }
   }

   now = time(NULL);
   for (i = 0 ; i < MAX_IDENTS_IN_PROGRESS ; i++)
   {
      if (idents_in_progress[i].local_port)
      {
         if (now > (idents_in_progress[i].request_time + IDENT_TIMEOUT))
         {
            /* Request has timed out, whether on connect or reply */
	    idents_in_progress[i].flags |= IDENT_TIMEDOUT;
	    process_result(i);
         }
      }
   }
}


/* Check for requests from the client */

void check_requests(void)
{
   char msgbuf[129];
   int toread;
   int bread;
   static int bufpos = 0;
   int i;

#ifdef DONT_USE_IOCTL
   fd_set stdinset;
   struct timeval timeout;

   FD_ZERO(&stdinset);
   FD_SET(0, &stdinset);
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;
   i = select(1, &stdinset, 0, 0, &timeout);
   switch(i) {
     case -1:
#if defined( DEBUG_IDENT_TOO)
	fprintf(stderr, "ident: select failed\n");
#endif /* DEBUG_IDENT_TOO */
     case 0:
#if defined( DEBUG_IDENT_TOO )
	fprintf(stderr, "Nothing ready from talker\n");
#endif
	return;
     default:
	toread = 128;
	break;
   }
#else /* DONT_USE_IOCTL */
   ioctl(0, FIONREAD, &toread);
   if (toread <= 0)
   {
      return;
   }
   if (toread > 128)
   {
      toread = 128;
   }
#endif /* DONT_USE_IOCTL */

   bread = read(0, msgbuf, toread);
   for (i = 0 ; i < bread ;)
   {
      req_buf[bufpos++] = msgbuf[i++];
      if (bufpos == req_size)
      {
         process_request(bufpos);
         bufpos = 0;
      }
   }
}

void process_request(int toread)
{
   int i;
   struct sockaddr_in sockadd;
   short int local_port;
   ident_identifier ident_id;

   for (i = 0 ; i < toread ;)
   {
      switch (req_buf[i++])
      {
         case CLIENT_SEND_REQUEST:
            memcpy(&ident_id, &req_buf[i], sizeof(ident_id));
            i += sizeof(ident_id);
            memcpy(&local_port, &req_buf[i], sizeof(local_port));
            i += sizeof(local_port);
            memcpy(&(sockadd.sin_family), &req_buf[i],
                   sizeof(sockadd.sin_family));
            i += sizeof(sockadd.sin_family);
            memcpy(&(sockadd.sin_addr.s_addr), &req_buf[i],
                 sizeof(sockadd.sin_addr.s_addr));
            i += sizeof(sockadd.sin_addr.s_addr);
            memcpy(&(sockadd.sin_port), &req_buf[i],
                   sizeof(sockadd.sin_port));
            i += sizeof(sockadd.sin_port);

#if defined(DEBUG_IDENT_TOO)
            fprintf(stderr, "Server: Id      = %d\n"
                            "        Address = %08X:%d\n"
		            "Bytes needed %d, Bytes to recieve %d\n",
                    ident_id,
                    (int) ntohl(sockadd.sin_addr.s_addr),
                    ntohs(sockadd.sin_port),
		    i, toread);
            fflush(stderr);
#endif /* DEBUG_IDENT_TOO */

            queue_request(ident_id, local_port, &sockadd);
            break;
         case CLIENT_CANCEL_REQUEST:
            memcpy(&ident_id, &req_buf[i], sizeof(ident_id));
            i += sizeof(ident_id);
            break;
         default:
#if defined(DEBUG_IDENT_TOO)
            fprintf(stderr, "Ident_Server: Unknown message from client:"
                      " '%d'\n",
                   req_buf[i]);
#endif /* DEBUG_IDENT_TOO */
          break;
      }
   }
}


/* Queue up a request from the client and/or send it off */

void queue_request(ident_identifier id, short int local_port,
   struct sockaddr_in *sockadd)
{
   ident_request *new_id_req;

   new_id_req = (ident_request *) MALLOC(sizeof(ident_request));
   new_id_req->ident_id = id;
   new_id_req->local_port = local_port;
   new_id_req->next = NULL;
   memcpy(&(new_id_req->sock_addr), sockadd, sizeof(struct sockaddr_in));
   if (!first_inq)
   {
      first_inq = new_id_req;
   } else if (!last_inq)
   {
      last_inq = new_id_req;
      first_inq->next = last_inq;
   } else
   {
      last_inq->next = new_id_req;
      last_inq = new_id_req;
   }
}

void take_off_queue(int freeslot)
{
   ident_request *old;

   if (first_inq)
   {
      memcpy(&idents_in_progress[freeslot], first_inq, sizeof(ident_request));
      old = first_inq;
      first_inq = first_inq->next;
      FREE(old);
      do_request(&idents_in_progress[freeslot]);
   }
}

void do_request(ident_request *id_req)
{
   int dummy;
   int ret;
   struct sockaddr_in sa;

#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "Server: Doing request %d\n",
           id_req->ident_id);
#endif /* DEBUG_IDENT_TOO */
   if (0 > (id_req->fd = socket(PF_INET, SOCK_STREAM, 0)))
   {
#if defined(DEBUG_IDENT_TOO)
      log("ident", "Couldn't get new fd for request\n");
#endif /* DEBUG_IDENT_TOO */
      /* Erk, put on pending queue */
      queue_request(id_req->ident_id, id_req->local_port,
               &(id_req->sock_addr));
      id_req->local_port = 0;
      return;
   }
#ifdef DONT_USE_IOCTL
   if(fcntl(id_req->fd, F_GETFL, &dummy) < 0) {
#if defined( DEBUG_IDENT_TOO )
	log("ident","Can't get id_req->id flags.(%d)\n", errno);
#endif /* DEBUG_IDENT_TOO */
   }
   else
   {
     if(fcntl(id_req->fd, F_SETFL, (dummy|O_NONBLOCK)) < 0) {
#if defined( DEBUG_IDENT_TOO )
	log("ident", "Can't set non-blocking on request sock\n");
#endif /* DEBUG_IDENT_TOO */
     }
   }
#else /* DONT_USE_IOCTL */
#ifdef OSF
   if (ioctl(id_req->fd, (int) FIONBIO, (caddr_t)&dummy) < 0)
#else
   if (ioctl(id_req->fd, FIONBIO, (caddr_t)&dummy) < 0)
#endif
   {
#if defined(DEBUG_IDENT_TOO)
      log("ident", "Can't set non-blocking on request sock\n");
#endif /* DEBUG_IDENT_TOO */
      /* Do without? */
   }
#endif /* DONT_USE_IOCTL */

   id_req->request_time = time(NULL);
   sa.sin_family = id_req->sock_addr.sin_family;
   sa.sin_addr.s_addr = id_req->sock_addr.sin_addr.s_addr;
   sa.sin_port = htons(IDENT_PORT);
   ret = connect(id_req->fd, (struct sockaddr *) &sa, sizeof(sa));
   if(ret!=0
#if defined(EINPROGRESS)
   		&& errno!=EINPROGRESS
#endif
   	)
    {
      if (errno == ECONNREFUSED)
      {
#if defined(DEBUG_IDENT_TOO)
         fprintf(stderr, "ID %d, CONNREFUSED\n", id_req->ident_id);
#endif /* DEBUG_IDENT_TOO */
         id_req->flags |= IDENT_CONNREFUSED;
      } else
      {
#if defined(DEBUG_IDENT_TOO)
         log("ident", "Error on connect, NOT CONNREFUSED, errno = %d\n",
             errno);
#endif /* DEBUG_IDENT_TOO */
      }
#if defined(EINPROGRESS)
   } else if (errno!=EINPROGRESS)
#else /* !EINPROGRESS */
   } else
#endif 
   {
      id_req->flags |= IDENT_CONNECTED;
      write_request(id_req);
   }
}

void write_request(ident_request *id_req)
{
   sprintf(scratch, "%d,%d\n", ntohs(id_req->sock_addr.sin_port),
               id_req->local_port);
   if (0 > write(id_req->fd, scratch, strlen(scratch)))
   {
      id_req->flags |= IDENT_CONNREFUSED;
   } else
   {
      id_req->flags |= IDENT_WRITTEN;
   }

#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "Server: Sending request '%s'\n", scratch);
   fflush(stderr);
#endif /* DEBUG_IDENT_TOO */
}

/* Get a result from an identd and send it to the client in the
 * form:
 *
 * <SERVER_SEND_REPLY><ident_id><reply text>'\n'
 */

void process_result(int slot)
{
   char reply[BUFFER_SIZE];
   int bread;
   char *s;
   char *t = NULL;
   char *reply_text = NULL;

   s = scratch;
   *s++ = SERVER_SEND_REPLY;
   memcpy(s, &(idents_in_progress[slot].ident_id),
          sizeof(ident_identifier));
   s += sizeof(ident_identifier);
   strcpy(s, "<ERROR>");
   reply_text = s;
#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "Server: Processing result...\n");
   fflush(stderr);
#endif /* DEBUG_IDENT_TOO */
   if (idents_in_progress[slot].flags & IDENT_CONNREFUSED)
   {
      /* Connection was refused */
      strcpy(s, "<CONN REFUSED>");
      t = strchr(s, '\0');
   } else if (!(idents_in_progress[slot].flags & IDENT_CONNECTED))
   {
      /* No connection was established */
      strcpy(s, "<CONN FAILED>");
      t = strchr(s, '\0');
#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "    Not Connected\n");
#endif /* DEBUG_IDENT_TOO */
   } else if (!(idents_in_progress[slot].flags & IDENT_WRITTEN))
   {
      /* Connection made but no message written */
      strcpy(s, "<DIDN'T SEND>");
      t = strchr(s, '\0');
#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "    Not Written\n");
#endif /* DEBUG_IDENT_TOO */
   } else if (!(idents_in_progress[slot].flags & IDENT_REPLY_READY))
   {
      /* Request written but didn't get reply before timeout */
      strcpy(s, "<TIMED OUT>");
      t = strchr(s, '\0');
#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "    Not Ready for Read\n");
#endif /* DEBUG_IDENT_TOO */
   } else
   {
      /* Got a reply, phew! */
      /* BUFFER_SIZE - 20 (== 1004 atm) should be plenty as RFC1413
       * specifies that a USERID reply should not have a user id field
       * of more than 512 octets.
       * Additionally RFC1413 specifies that:
       * "Clients should feel free to abort the connection if they
       *  receive 1000 characters without receiving an <EOL>"
       *
       * NB: <EOL> ::= "015 012"  ; CR-LF End of Line Indicator
       * I assume that under C this will be converted to '\n'
       */
#if defined(HAVE_BZERO)
      bzero(reply, BUFFER_SIZE);
#else /* !HAVE_BZERO */
      memset(reply, 0, BUFFER_SIZE);
#endif /* HAVE_BZERO */
      bread = read(idents_in_progress[slot].fd, reply, BUFFER_SIZE - 20);
      reply[bread] = '\0';

      /* Make sure the reply is '\n' terminated */
      t = strchr(reply, '\n');
      if (t)
      {
         t++;
#if defined(HAVE_BZERO)
	 bzero(t, &reply[BUFFER_SIZE] - t - 1);
#else
	 memset(t, 0, &reply[BUFFER_SIZE] - t - 1);
#endif /* HAVE_BZERO */
      } else
      {
         reply[1001] = '\n';
#if defined(HAVE_BZERO)
	 bzero(&reply[1002], BUFFER_SIZE - 1002 - 1);
#else
	 memset(&reply[1002], 0, BUFFER_SIZE - 1002 - 1);
#endif /* HAVE_BZERO */
      }
#if defined(DEBUG_IDENT_TOO)
   fprintf(stderr, "Server: Got reply '%s'\n",
           reply);
#endif /* DEBUG_IDENT_TOO */

      if (!(t = strstr(reply, "USERID")))
      {
         /* In this case the reply MUST be in the form:
          *
          * <port-pair> : ERROR : <error-type>
          *
          * (see RFC1413, if a later one exists for the ident protocol
          *  then this code should be updated)
	  *
	  * We will reply to our client with the '<error-type>' text
	  *
          */
#if defined(DEBUG_IDENT)
         log("ident_ids", "SERVER: slot %d, ident_id %d, ACTUAL reply '%s'",
         	slot, idents_in_progress[slot].ident_id, reply);
#endif /* DEBUG_IDENT */
	 if (!(t = strstr(reply, "ERROR")))
	 {
	 /* Reply doesn't contain 'USERID' *or* 'ERROR' */
	    strcpy(s, "<ERROR>");
	 } else
	 {
#if defined(DEBUG_IDENT_TOO)
	    log("ident", "Reply: '%s'", t);
#endif /* DEBUG_IDENT_TOO */
            t = strchr(t, ':');
	    /*
	     * <port-pair> : ERROR : <error-type>
	     *                     ^
	     *             t-------|
	     */
	    if (!t)
	    {
	       /* Couldn't find the colon after 'ERROR' */
	       strcpy(s, "<ERROR>");
	    } else
	    {
	       *s++ = '<';
	    }
	 }
      } else
      {
         /* Got a reply of the form:
          *
          * <port-pair> : USERID : <opsys-field> : <user-id>
	  *               ^
	  *         t-----|
          *
	  * We will reply to our client with the '<user-id>' text
	  *
          */

#if defined(DEBUG_IDENT)
         log("ident_ids", "SERVER: slot %d, ident_id %d, ACTUAL reply '%s'",
   	     slot, idents_in_progress[slot].ident_id, reply);
	 log("ident", "Reply: '%s'", t);
#endif /* DEBUG_IDENT */
         t = strchr(t, ':');
	 if (!t)
	 {
	    /* Couldn't find the : after USERID 
	     *
             * <port-pair> : USERID : <opsys-field> : <user-id>
	     *                      ^
	     *           t----------|
	     */
	    strcpy(s, "<ERROR>");
	 } else
	 {
            t++;
            t = strchr(t, ':');
	    if (!t)
	    {
	       /* Couldn't find the : after <opsys-field>
	        *
                * <port-pair> : USERID : <opsys-field> : <user-id>
	        *                                      ^
	        *                           t----------|
	        */
	       strcpy(s, "<ERROR>");
	    }
	 }
      }
      if (t)
      {
      /* t is now at the : before the field we want to return */
	 /* Skip the : */
         t++;
	 /* Skip any white space */
         while ((*t == ' ' || *t == '\t') && *t != '\0')
         {
            t++;
         }
	 if (*t != '\0')
	 {
	 /* Found start of the desired text */
            sprintf(s, "%s", t);
            t = strchr(s, '\0');
            t--;
	    /* The reply SHOULD be '\n' terminated (RFC1413) */
            while (*t == ' ' || *t == '\t' || *t == '\n' || *t == '\r')
            {
               t--;
            }
	    /* t now at end of text we want */
	    /* Move forward to next char */
            t++;
	    /* If char before s is a '<' we put it there ourselves to wrap
	     * an 'ERROR' return in <>. So we need to put the > on the
	     * end
	     */
            if (*(s - 1) == '<')
            {
               *t++ = '>';
            }
	 } else
	 {
	    strcpy(s, "<ERROR>");
	    t = 0;
	 }
      }
   }
   /* Make sure t is pointing to the NULL terminator of the string */
   if (!t)
   {
      t = strchr(s, '\0');
   }
   /* A newline terminates the message */
   *t++ = '\n';
   *t = '\0';
   /* Now actually send the reply to the client */
   write(1, scratch, (t - scratch));
#if defined(DEBUG_IDENT)
   log("ident_ids", "SERVER: slot %d, ident_id %d, reply '%s'",
   	slot, idents_in_progress[slot].ident_id, reply_text);
#endif /* DEBUG_IDENT */
#if defined(DEBUG_IDENT_TOO)
   *(t + 1) = '\0';
   fprintf(stderr, "    Sending reply '%s' of %d bytes\n", s, (int) (t - scratch));
   fflush(stderr);
#endif /* DEBUG_IDENT_TOO */
   closedown_request(slot);
}

void closedown_request(int slot)
{
   idents_in_progress[slot].local_port = 0;
   shutdown(idents_in_progress[slot].fd, 2);
   close(idents_in_progress[slot].fd);
}


/* log using stdarg variable length arguments */

void log(char *str, ...)
{
   va_list args;
   FILE *fp;
   char *fmt;
   struct tm *tim;
   char the_time[] = "xx/xx/xx - xx:xx.xx : ";
   char fname[21];
   time_t current_time;

   va_start(args, str);

   current_time = time(0);
   tim = localtime(&current_time);
   strftime((char *)the_time, strlen(the_time), "%d/%m/%y - %T : ", tim);
   sprintf(fname, "logs/%s.log", str);

   fp = fopen(fname, "a");
   if (!fp)
   {
      fprintf(stderr, "Eek! Can't open file to log: %s\n", fname);
      return;
   }
   fprintf(fp, "%s", the_time);
   fmt = va_arg(args, char *);
   vfprintf(fp, fmt, args);
   va_end(args);
   fclose(fp);
}
