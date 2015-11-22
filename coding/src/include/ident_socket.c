/*
 * ident_client.c
 *
 */

#define IDE_VERS "1.01"

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

#include "ident.h"

/*
#if defined(HAVE_UNISTDH)
#include <unistd.h>
#endif 
#if defined(HAVE_FILIOH)
#include <sys/filio.h>
#endif
#if !defined(OSF)
#include <sys/wait.h>
#endif 
*/

/* Local functions */

int init_ident_server(void);
void kill_ident_server(void);
void process_reply(int msg_size);
void read_ident_reply(void);
void send_ident_request(player *p, struct sockaddr_in *sadd);

/* Local Variables */

#define BUFFER_SIZE 2048      /* Significantly bigger than the
                               * equivalent in ident_server.c
			       */
int ident_toclient_fds[2];
int ident_toserver_fds[2];
int ident_server_pid = 0;
ident_identifier ident_id = 0;
char ident_buf_input[BUFFER_SIZE];
char ident_buf_output[BUFFER_SIZE];
char reply_buf[BUFFER_SIZE];
short int local_port;


/*
 * Start up the ident server
 */

int init_ident_server(void)
{
   int ret;
#if defined(NONBLOCKING)
   int dummy;
#if defined( DONT_USE_IOCTL )
   int fcntl_ret;
#endif /* DONT_USE_IOCTL */
#endif /* NONBLOCKING */
   fd_set fds;
   struct timeval timeout;
   char name_buffer[256];
   
   memset(name_buffer, 0, 256);
   sprintf(name_buffer, "-=> %s <=- Ident server", TALKER_NAME);
   
   local_port = (short int) talker_port;
   if (-1 == pipe(ident_toclient_fds))
   {
      switch (errno)
      {
         case EMFILE:
            log("boot", "init_ident_server: Too many fd's in use by"
	    		" process\n");
            exit(1);
         case ENFILE:
            log("boot", "init_ident_server: Too many fd's in use in"
	    		" system\n");
            exit(1);
         case EFAULT:
            log("boot", "init_ident_server: ident_toclient_fds invalid!\n");
            exit(1);
      }
   }
   if (-1 == pipe(ident_toserver_fds))
   {
      switch (errno)
      {
         case EMFILE:
            log("boot", "init_ident_server: Too many fd's in use by"
	    		" process\n");
            exit(1);
         case ENFILE:
            log("boot", "init_ident_server: Too many fd's in use in"
	    		" system\n");
            exit(1);
         case EFAULT:
            log("boot", "init_ident_server: ident_toserver_fds invalid!\n");
            exit(1);
      }
   }

#ifdef SOLARIS
   /* Ack never use VFORK on SOLARIS 2 it can have very nasty effects */
   ret = fork();
#elif IRIX
   ret = fork();  /* irix is buggy */
#else
   ret = vfork();
#endif /* SOLARIS */
   switch (ret)
   {
      case -1: /* Error */
         log("boot", "init_ident_server couldn't fork!\n");
         exit(1);
      case 0:   /* Child */
         close(IDENT_CLIENT_READ);
         close(IDENT_CLIENT_WRITE);
         close(0);
         dup(IDENT_SERVER_READ);
         close(IDENT_SERVER_READ);
         close(1);
         dup(IDENT_SERVER_WRITE);
         close(IDENT_SERVER_WRITE);
         execlp("bin/ident", name_buffer, 0);
         log("boot", "init_ident_server failed to exec ident\n");
         exit(1);
      default: /* Parent */
         ident_server_pid = ret;
         close(IDENT_SERVER_READ);
         close(IDENT_SERVER_WRITE);
         IDENT_SERVER_READ = IDENT_SERVER_WRITE = -1;
#if defined( NONBLOCKING )
#if defined( DONT_USE_IOCTL )
	 if ((fcntl_ret = fcntl(IDENT_CLIENT_READ, F_GETFL, &dummy)) < 0) {
	     log("error", "Ack! fcntl can't get flags of ident client read.");
	 }
#if defined( DEBUG_IDENT )
	 fprintf(stderr, "fcntl, returned (%d), dummy = (%d).\n", fcntl_ret, dummy); 
#endif /* DEBUG_IDENT */
	 if (fcntl(IDENT_CLIENT_READ, F_SETFL, (fcntl_ret == 0)?dummy:fcntl_ret) < 0) {
	     log("error", "Ack!  fcntl can't set non-blocking to ident client read.");
	 }
	 if ((fcntl_ret = fcntl(IDENT_CLIENT_WRITE, F_GETFL, &dummy)) < 0) {
	     log("error", "Ack! fcntl can't get flags of ident client write.");
	 }
#if defined( DEBUG_IDENT )
	fprintf(stderr, "fcntl, returned (%d), dummy = (%d).\n", fcntl_ret, dummy);
#endif /* DEBUG_IDENT */
	if (fcntl(IDENT_CLIENT_WRITE, F_SETFL, (fcntl_ret == 0)?dummy:fcntl_ret))
	{
	    log("error", "Ack!  fcntl can't set non-blocking to ident client write.");
	}
#else  /* !DONT_USE_IOCTL */
#ifdef OSF
	 if (ioctl(IDENT_CLIENT_READ, (int) FIONBIO, &dummy) < 0)
#else
	 if (ioctl(IDENT_CLIENT_READ, FIONBIO, &dummy) < 0)
#endif
	 {
	   log("error", "Ack! Can't set non-blocking to ident read.");
	 }
	 if (ioctl(IDENT_CLIENT_WRITE, FIONBIO, &dummy) < 0)
	 {
	   log("error", "Ack! Can't set non-blocking to ident write.");
	 }
#endif /* DONT_USE_IOCTL */
#endif /* NONBLOCKING */
   }

   FD_ZERO(&fds);
   FD_SET(IDENT_CLIENT_READ, &fds);
   timeout.tv_sec = 5;
   timeout.tv_usec = 0;
   if (0 >= (select(FD_SETSIZE, &fds, 0, 0, &timeout)))
   {
      log("boot", "init_ident_server: Timed out waiting for server"
      		  " connect\n");
      kill_ident_server();
      return 0;
   }

#if !defined( DONT_USE_IOCTL )
   ioctl(IDENT_CLIENT_READ, FIONREAD, &ret);
   while (ret != strlen(SERVER_CONNECT_MSG))
   {
      sleep(1);
      ioctl(IDENT_CLIENT_READ, FIONREAD, &ret);
   }
#endif /* DONT_USE_IOCTL */
   ret = read(IDENT_CLIENT_READ, ident_buf_input, ret);
   ident_buf_input[ret] = '\0';
   if (strcmp(ident_buf_input, SERVER_CONNECT_MSG))
   {
      fprintf(stderr, "From Ident: '%s'\n", ident_buf_input);
      log("boot", "init_ident_server: Bad connect from server, killing\n");
      kill_ident_server();
      return 0;
   }
   log("boot", "Ident Server Up and Running");

   return 1;
}

/* Shutdown the ident server */

void kill_ident_server(void)
{
   int status;

   close(IDENT_CLIENT_READ);
   close(IDENT_CLIENT_WRITE);
   IDENT_CLIENT_READ = -1;
   IDENT_CLIENT_WRITE = -1;
   kill(ident_server_pid, SIGTERM);
   waitpid(-1, &status, WNOHANG);
}

void send_ident_request(player *p, struct sockaddr_in *sadd)
{
   char *s;
   int bwritten;

   s = ident_buf_output;
   *s++ = CLIENT_SEND_REQUEST;
   memcpy(s, &ident_id, sizeof(ident_id));
   s += sizeof(ident_id);
   memcpy(s, &(local_port), sizeof(local_port));
   s += sizeof(local_port);
   memcpy(s, &(sadd->sin_family), sizeof(sadd->sin_family));
   s += sizeof(sadd->sin_family);
   memcpy(s, &(sadd->sin_addr.s_addr), sizeof(sadd->sin_addr.s_addr));
   s += sizeof(sadd->sin_addr.s_addr);
   memcpy(s, &(sadd->sin_port), sizeof(sadd->sin_port));
   s += sizeof(sadd->sin_port);
   bwritten = write(IDENT_CLIENT_WRITE, ident_buf_output,
                    (s - ident_buf_output));
   p->ident_id = ident_id++;
   if (bwritten < (s - ident_buf_output))
   {
      log("ident", "Client failed to write request, killing and restarting"
      		   " Server\n");
      kill_ident_server();
      sleep(3);
      init_ident_server();
      bwritten = write(IDENT_CLIENT_WRITE, ident_buf_output,
                       (s - ident_buf_output));
      if (bwritten < (s - ident_buf_output))
      {
         log("ident", "Restart failed\n");
      }
   } else
   {
#if defined(DEBUG_IDENT)
      vlog("ident_ids", "Player '%s', fd %d, ident_id %d\n",
          p->name[0] ? p->name : "<NOT ENTERED>",
	  p->fd,
	  p->ident_id);
#endif /* DEBUG_IDENT */
   }
   
#if defined(DEBUG_IDENT)
   fprintf(stderr, "Bytes Written %d, Should have sent %d\n",
	   bwritten, (int) (s - ident_buf_output)); 
   fprintf(stderr, "Client: %08X:%d\n",
           (int) ntohl(sadd->sin_addr.s_addr),
           ntohs(sadd->sin_port));
   fflush(stderr);
#endif
}

void read_ident_reply(void)
{
   int bread;
   int toread;
   int i;
   static int bufpos = 0;

   ioctl(IDENT_CLIENT_READ, FIONREAD, &toread);
   if (toread <= 0)
   {
      return;
   }
   bread = read(IDENT_CLIENT_READ, ident_buf_input, BUFFER_SIZE - 20);
   ident_buf_input[bread] = '\0';

   for (i = 0 ; i < bread ; )
   {
      reply_buf[bufpos++] = ident_buf_input[i++];
      if ((bufpos > (sizeof(char) + sizeof(ident_identifier)))
          && (reply_buf[bufpos - 1] == '\n'))
      {
	 process_reply(bufpos);
         bufpos = 0;
#if defined(HAVE_BZERO)
	 bzero(reply_buf, BUFFER_SIZE);
#else
	 memset(reply_buf, 0, BUFFER_SIZE);
#endif /* HAVE_BZERO */
      }
   }
}

void process_reply(int msg_size)
{
   char *s;
   int i;
   ident_identifier id;
   player *scan;

   for (i = 0 ; i < msg_size ;)
   {
      switch (reply_buf[i++])
      {
         case SERVER_SEND_REPLY:
            memcpy(&id, &reply_buf[i], sizeof(ident_identifier));
            i += sizeof(ident_identifier);
#if defined(DEBUG_IDENT)
	    vlog("ident_ids", "Got reply for ident_id %d\n", 
	        id);
#endif /* DEBUG_IDENT */
            for (scan = flatlist_start ; scan ; scan = scan->flat_next)
            {
               if (scan->ident_id == id)
               {
#if defined(DEBUG_IDENT)
		  vlog("ident_ids", "Matched ident_id %d to Player '%s',"
		  		   " fd %d\n",
		      id,
		      scan->name[0] ? scan->name : "<NOT ENTERED>",
		      scan->fd);
#endif /* DEBUG_IDENT */
                  break;
               }
            }
#if defined(DEBUG_IDENT)
	    fprintf(stderr, "Client: Got reply '%s'\n", &reply_buf[i]);
#endif /* DEBUG_IDENT */
            s = strchr(&reply_buf[i], '\n');
            if (s)
            {
               *s++ = '\0';
            } else
            {
	       s = strchr(reply_buf, '\0');
	       *s++ = '\n';
	       *s = '\0';
            }
	    if (scan)
	    {
               strncpy(scan->userID, &reply_buf[i],
			(MAX_REMOTE_USER < (s - &reply_buf[i]) ? 
				MAX_REMOTE_USER : (s - &reply_buf[i])));
#if defined(DEBUG_IDENT)
	       vlog("ident_ids", "Write ident_id %d, Reply '%s' to player '%s'"
	       		        " fd %d\n",
		   id,
		   scan->userID,
		   scan->name[0] ? scan->name : "<NOT ENTERED>",
		   scan->fd);
#endif /* DEBUG_IDENT */
            } else
            {
               /* Can only assume connection dropped from here and we still
                * somehow got a reply, throw it away
                */
#if defined(DEBUG_IDENT)
	       vlog("ident_ids", "Threw away response for ident_id %d\n",
	           id);
#endif /* DEBUG_IDENT */
            }
	    while (reply_buf[i] != '\n')
	    {
	       i++;
	    }
            break;
         default:
#if defined(DEBUG_IDENT_TOO)
            vlog("ident", "Bad reply from server '%d'\n", reply_buf[i]);
#endif /* DEBUG_IDENT_TOO */
            i++;
      }
   }
}


/* ident version */
void	ident_version(void)
{
  sprintf(stack, " -=> Ident server V1.01 by Athanasius and Oolon enabled.\n");
  stack = strchr(stack, 0);
}


