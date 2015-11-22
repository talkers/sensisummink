/*
 * socket.c
 */

#include "include/config.h"

#include <stdio.h>
#include <sys/types.h>
#if defined( LINUX ) && defined( GLIBC )
#define __STRICT_ANSI__
#include <sys/socket.h>
#undef __STRICT_ANSI__
#else
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <netdb.h>
#ifdef SOLARIS
#include <sys/filio.h>
#ifndef SOLARIS24
#include <strings.h>
#endif
#endif /* SOLARIS */
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef IRIX
#include <bstring.h>
#endif

#ifdef SUNOS
#include <memory.h>
#include <vfork.h>
#define TIME_DEFINES
#define SOCKET_DEFINES
#endif

/* you -might- have to fiddle with this library and use the system one */

#if defined( IRIX ) && defined( WRAP )
#undef WRAP
#endif
#include "include/ansi.h"
#include "include/missing_headers.h"
#include "include/proto.h"

#ifdef IDENT
#include <signal.h>
#include <sys/wait.h>
#include "include/ident_socket.c"
#endif


/* interns */
void		get_player_input(player *);
int             main_descriptor, alive_descriptor;
file            hitells_msg, motd_msg, connect_msg, newban_msg, banned_msg, nonewbies_msg,
                newbie_msg, newpage1_msg, newpage2_msg, disclaimer_msg,
                sig_file, splat_msg, banish_file, banish_msg, banish_scan, full_msg, splat_msg;
void		accept_new_connection(void);


/* terminal defintitions */

struct terminal terms[] = {
   {"xterm", "\033[1m", "\033[m", "\033[H\033[2J"},
   {"vt220", "\033[1m", "\033[m", "\033[H\033[J"},
   {"vt100", "\033[1m", "\033[m", "50\033[;H\0332J"},
   {"vt102", "\033[1m", "\033[m", "50\033["},
   {"ansi", "\033[1m", "\033[0m", "50\033[;H\0332J"},
   {"wyse-30", "\033G4", "\033G0", ""},
   {"tvi912", "\033l", "\033m", "\032"},
   {"sun", "\033[1m", "\033[m", "\014"},
   {"adm", "\033)", "\033(", "1\032"},
   {"hp2392", "\033&dB", "\033&d@", "\033H\033J"},
   {"", "", "", ""}
};

#ifdef ANSI_COLS
#include "include/colour_socket.c"
#else
#include "include/nocolour_socket.c"
#endif


/* the functions */

/* close down sockets after use */

void            close_down_socket(void)
{
   shutdown(main_descriptor, 2);
   close(main_descriptor);
}

#ifdef INTERCOM
void close_only_main_fd(void)
{
  close(main_descriptor);
  return;
}
#endif

/* grab the main socket */

void            init_socket(void)
{
   struct sockaddr_in main_socket;
   int             dummy = 1;
   char           *oldstack;
   char *hostname;
   struct hostent *hp;

   oldstack = stack;
   
   /* grab the main socket */

   hostname=(char *)malloc(101);
#ifndef HAVE_BZERO
    memset(&main_socket, 0, sizeof(struct sockaddr_in));
#else 
    bzero((char *)&main_socket, sizeof(struct sockaddr_in));
#endif /* have_bzero */
   gethostname(hostname,100);

   hp=gethostbyname(hostname);
   if ( hp == NULL)
      handle_error("Error: Host machine does not exist!\n");
 
   main_socket.sin_family=hp->h_addrtype;
   main_socket.sin_port=htons(talker_port);

   main_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
   if (main_descriptor < 0)
   {
      log("boot", "Couldn't grab the socket!!!");
      exit(-1);
   }

   if (setsockopt(main_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy,
       sizeof(dummy)) < 0)
      handle_error("Couldn't setsockopt()");
   
#ifdef OSF
    if (ioctl(main_descriptor, (int) FIONBIO, &dummy) < 0 )
#else
    if (ioctl(main_descriptor, FIONBIO, &dummy) < 0 )
#endif
      handle_error("Can't set non-blocking");

/*
   main_socket.sin_addr.s_addr = INADDR_ANY;
*/

   if (bind(main_descriptor, (struct sockaddr *) & main_socket, sizeof(main_socket)) < 0)
   {
      log("boot", "Couldn't bind socket!!!");
      exit(-2);
   }


   if (listen(main_descriptor, 5) < 0)
      handle_error("Listen refused");

   vlog("boot", "Main socket bound and listening on port %d", talker_port);

   stack = oldstack;
}


/* tell the angel the server is alive */
void            do_alive_ping(void)
{
   static int      count = 5;

   count--;
   if (!count && alive_descriptor > 0)
   {
      count = 5;
      write(alive_descriptor, "SpAng!", 6);
   }
}


/* connect to the alive socket */

void            alive_connect(void)
{
   struct sockaddr_un sa;

   alive_descriptor = socket(PF_UNIX, SOCK_STREAM, 0);
   if (alive_descriptor < 0)
      handle_error("failed to make socket");

   sa.sun_family = AF_UNIX;
   strcpy(sa.sun_path, SOCKET_PATH);

#if defined( IRIX )
   if (connect(alive_descriptor, &sa, sizeof(sa)) < 0)
#else
   if (connect(alive_descriptor, (struct sockaddr *) & sa, sizeof(sa)) < 0)
#endif
   {
      close(alive_descriptor);
      alive_descriptor = -1;
      log("error", "Failed to connect to alive socket - ignoring");
      return;
   }
   do_alive_ping();

   log("boot", "Alive and kicking");

}



/* accept new connection on the main socket */

void            accept_new_connection()
{
   struct sockaddr_in incoming;
   struct hostent *hp;
   int             new_socket;
#ifdef GLIBC
    unsigned int   length;
#else
    int		   length;
#endif
   char           *numerical_address;
   player         *p;
   int             no1, no2, no3, no4, dummy=1;
   
   length = sizeof(incoming);
   new_socket = accept(main_descriptor, (struct sockaddr *)&incoming, &length);
   if ((new_socket < 0) && (errno == EINTR || errno == EAGAIN))
   {
      log("error", "EINTR accept trap");
      return;
   }
   if (new_socket < 0)
      handle_error("Error accepting new connection.");

#ifdef OSF
    if (ioctl(new_socket, (int) FIONBIO, &dummy) < 0)
#else
    if (ioctl(new_socket, FIONBIO, &dummy) < 0)
#endif
      handle_error("Can't set non-blocking");

   if (current_players == max_players)
   {
      write(new_socket, full_msg.where, full_msg.length);
      out_current += full_msg.length;
      out_pack_current++;
      return;
   }

   p = create_player();
   current_player = p;
   p->fd = new_socket;

   strncpy(p->num_addr, inet_ntoa(incoming.sin_addr), MAX_INET_ADDR - 2);
#ifdef LOLIGO
   numerical_address = p->num_addr;
#else
   hp = gethostbyaddr((char *) &(incoming.sin_addr.s_addr),
            sizeof(incoming.sin_addr.s_addr),
            AF_INET);
   if (hp)
      numerical_address = strdup(hp->h_name);
   else
      numerical_address = p->num_addr;
#endif
   strncpy(p->inet_addr, numerical_address, MAX_INET_ADDR - 2);
   sscanf(p->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);

   if (do_banish(p))
   {
      write(new_socket, banish_msg.where, banish_msg.length);
      out_current += banish_msg.length;
      out_pack_current++;
      destroy_player(p);
      return;
   } else if (time(0) < splat_timeout && no1 == splat1 && 
   	      (no2 == splat2 || splat2 == 255) && (no3 == splat3 || splat3 == 255) &&
   	      (no4 == splat4 || splat4 == 255))
   {
      write(new_socket, splat_msg.where, splat_msg.length);
      out_current += banish_msg.length;
      out_pack_current++;
      destroy_player(p);
      return;
   } else {
#ifdef IDENT
       send_ident_request(p, &incoming);
#endif
      connect_to_prog(p);
   }
   current_player = 0;
}

/* turn on and off echo for passwords */

void            password_mode_on(player * p)
{
   p->flags |= PASSWORD_MODE;
   p->mode |= PASSWORD;
#ifndef IRIX_BUG
    if (!(p->flags & DO_LOCAL_ECHO))
      tell_player(p, "\377\373\001");
#endif
}

void            password_mode_off(player * p)
{
   p->flags &= ~PASSWORD_MODE;
   p->mode &= ~PASSWORD;
#ifndef IRIX_BUG
    if (!(p->flags & DO_LOCAL_ECHO))
      tell_player(p, "\377\374\001");
#endif
}

/* do a backspace */

void            backspace(player * p)
{
   p->ibuffer[p->ibuff_pointer] = 0;
   if (p->ibuff_pointer > 0)
      p->ibuff_pointer--;
}


/* handle telnet control codes - new version */
void    telnet_options(player *p)
{
  unsigned char c;

  if (read(p->fd, &c, 1) != 1)
    return;
  switch (c)
  {
  case EC:
    backspace(p);
    break;
  case EL:
    p->ibuff_pointer = 0;
    break;
  case IP:
    quit(p, 0);
    break;
    /* SOMEONE forgot to add this ~Mantis */
  case WILL:
  case DO:
    if (read(p->fd, &c, 1) != 1)
      return;
    switch (c)
    {
    case TELOPT_ECHO:
      if (!(p->flags & PASSWORD_MODE))
        p->flags |= DO_LOCAL_ECHO; /* start local echo */
    break;
    case TELOPT_SGA:
      break;
    case TELOPT_EOR:
      p->flags |= EOR_ON;
      p->flags &= ~IAC_GA_DO;
      tell_player(p, "\377\031");
      break;
    }
    break;
    /* SOMEONE forgot to add this ~Mantis */
  case WONT:
  case DONT:
    if (read(p->fd, &c, 1) != 1)
      return;
    switch (c)
    {
    case TELOPT_ECHO:
      p->flags &= ~DO_LOCAL_ECHO; /* stop local echo */
      break;
    case TELOPT_SGA:
      break;
    case TELOPT_EOR:
      p->flags &= ~EOR_ON;
      if (p->saved_flags & IAC_GA_ON)
        p->flags |= IAC_GA_DO;
      break;
    }
    break;
  }
}


/* gets any input from one player */

void            get_player_input(player * p)
{
   int             chars_ready = 0, save_errno;
   char           *oldstack, c;

   oldstack = stack;

   if (ioctl(p->fd, FIONREAD, &chars_ready) == -1)
   {
      quit(p, 0);
      log("error", "PANIC on FIONREAD ioctl");
      perror("SpoooN (socket.c)");
      return;
   }
   if (!chars_ready)
   {
      if (sys_flags & VERBOSE)
        if (p->lower_name[0])
          vlog("connection", "%s went netdead.", p->name);
        else
          log("connection", "Connection went netdead on login.");
      quit(p, 0);
      return;
   }
   in_current += chars_ready;
   in_pack_current++;


   for (; !(p->flags & PANIC) && chars_ready; chars_ready--) {
     errno = save_errno = 0;
     if (read(p->fd, &c, 1) != 1) {
       save_errno = errno;
       if (errno == EINTR || errno == EAGAIN) {
         chars_ready++;
         continue;
       }
       else {
         vlog("read_error", "bad read(), not EINTR/EAGAIN.  Was: %s (%d)",
         	strerror(save_errno), save_errno);
         quit(p, 0);
         return;
       }
     }
		/* OLD VERSION  for (errno=0; !(p->flags & PANIC) && chars_ready; chars_ready--) 
     		if (read(p->fd, &c, 1) != 1) {
       		vlog("read_error", "Errno: %d, Name:%s, input:%s", errno, p->name, p->ibuffer);
       		if(errno!=11) {
         	quit(p, 0);
       		}
       		return;
     		} */
     else
       switch (c) {
         case -1:
         case 1:
           p->flags &= ~(LAST_CHAR_WAS_R | LAST_CHAR_WAS_N);
           telnet_options(p);
           return;
           break;
         case '\n':
           if (!(p->flags & LAST_CHAR_WAS_R)) {
             p->flags |= LAST_CHAR_WAS_N;
#ifdef ANTIPIPE
               p->anticrash = 0;
#endif
             p->flags |= INPUT_READY;
             p->ibuffer[p->ibuff_pointer] = 0;
             p->ibuff_pointer = 0;
             p->column = 0;
             return;
           }
           break;
         case '\r':
           if (!(p->flags & LAST_CHAR_WAS_N)) {
             p->flags |= LAST_CHAR_WAS_R;
#ifdef ANTIPIPE
               p->anticrash = 0;
#endif
             p->flags |= INPUT_READY;
             p->ibuffer[p->ibuff_pointer] = 0;
             p->ibuff_pointer = 0;
             p->column = 0;
             return;
           }
           break;
         default:
           p->flags &= ~(LAST_CHAR_WAS_R | LAST_CHAR_WAS_N);
#ifdef IRIX
            if (c == 8 || c == 127 || c == 9) {
#else
            if (c == 8 || c == 127 || c == -9) {
#endif
             backspace(p);
             break;
           }
           if ((c > 31) && (p->ibuff_pointer < (IBUFFER_LENGTH - 3))) {
             p->ibuffer[p->ibuff_pointer] = c;
             p->ibuff_pointer++;
             if ((!(p->flags & PASSWORD_MODE)) && (p->flags & DO_LOCAL_ECHO)) {
               if (write(p->fd, &c, 1) < 0 && errno != EINTR && errno != EAGAIN) {
                 log("error", "Echoing back to player.\n");
                 quit(p, 0);
                 return;
               }
               out_current++;
               out_pack_current++;
             }
           } 
           else {
#ifdef ANTIPIPE
               /* ANTI-PIPING bit */
               p->anticrash++;
               if (p->anticrash > MAX_ANTIPIPE) {
                 tell_player(p, "\n\n Auto chuckout to prevent piping.\n\n");
                 p->flags |= TRIED_QUIT;
                 quit(p, "");
                 return;
               }
#endif
           }
           break;
       }
   }
}


/* this routine is called when idle */
void            scan_sockets(void)
{
   fd_set          fset;
   player         *scan;

   action = fline;
   
   FD_ZERO(&fset);

   FD_SET(main_descriptor, &fset);
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
#ifdef ROBOTS
      if (!((scan->fd < 0) || (scan->flags & ROBOT) || (scan->flags & PANIC)))
#else
      if (!((scan->fd < 0) || (scan->flags & PANIC)))
#endif
         FD_SET(scan->fd, &fset);
   }
   
#ifdef INTERCOM
   action = fline;
   if(intercom_fd > -1)
   {
     if(intercom_last < time(NULL))
       kill_intercom();
     else
       FD_SET(intercom_fd, &fset);
   }
   else
     intercom_fd = establish_intercom_server();
#endif

#ifdef IDENT
   action = fline;
   if(IDENT_CLIENT_READ != -1)
     FD_SET(IDENT_CLIENT_READ, &fset);
#endif

   action = fline;
   if (select(FD_SETSIZE, &fset, 0, 0, 0) == -1)
      return;
      
   action = fline;
   if (FD_ISSET(main_descriptor, &fset))
      accept_new_connection();

   action = fline;
   for (scan = flatlist_start; scan; scan = scan->flat_next)
#ifdef ROBOTS
      if (!(scan->fd < 0 || scan->flags & ROBOT || scan->flags & (PANIC|INPUT_READY))
#else
      if (!(scan->fd < 0 || scan->flags & (PANIC | INPUT_READY))
#endif
     && FD_ISSET(scan->fd, &fset))
         get_player_input(scan);

#ifdef INTERCOM
   action = fline;
   if (intercom_fd > -1 && FD_ISSET(intercom_fd, &fset))
     parse_incoming_intercom();
#endif
   
#ifdef IDENT
   action = fline;
   if(IDENT_CLIENT_READ!=-1) 
     if(FD_ISSET(IDENT_CLIENT_READ, &fset)) 
       read_ident_reply();
#endif

}


/**/

/* generic routine to write to one player */

void tell_player(player * p, char *str)
{
   file output;
   char *oldstack, *script;

   oldstack = stack;
   if (((p->fd) < 0) || (p->flags & PANIC) ||
       (!(p->location) && current_player != p))
   {
      return;
   }
   if (!(sys_flags & PANIC))
   {
      if (!test_receive(p))
         return;
   }
   output = process_output(p, str);
   if (p->script)
   {
      script = stack;
      sprintf(stack, "emergency/%s_emergency", p->lower_name);
      stack = end_string(stack);
      log(script, str);
      stack = script;
   }
   if (p->flags & SCRIPTING)
   { 
      script = stack;
      sprintf(stack, "scripts/%s", p->script_file);
      stack = end_string(stack);
      log(script, str);
      script = oldstack;
   }
   if (write(p->fd, output.where, output.length) < 0 && errno != EINTR && errno != EAGAIN) {
      quit(p, 0);
   }
   out_current += output.length;
   out_pack_current++;
   stack = oldstack;
}


/* small derivative of tell player to save typing */
void            tell_current(char *str)
{
   if (!current_player)
      return;
   tell_player(current_player, str);
}


/* non blockable raw tell */
void            non_block_tell(player * p, char *str)
{
   file            output;
   char           *script, *oldstack;

   oldstack = stack;
   if (((p->fd) < 0) || (p->flags & PANIC))
      return;
   output = process_output(p, str);
   if (p->script)
   {
      script = stack;
      sprintf(stack, "emergency/%s_emergency", p->lower_name);
      stack = end_string(stack);
      log(script, str);
      stack = script;
   }
   if (write(p->fd, output.where, output.length) < 0 && errno != EINTR && errno != EAGAIN) {
      quit(p, 0);
   }
   out_current += output.length;
   out_pack_current++;
   stack = oldstack;
}


/* general routine to send a prompt */

void            do_prompt(player * p, char *str)
{
   char           *oldstack;

   oldstack = stack;
   strcpy(stack, str);
   stack = strchr(stack, 0);;
   if (p->flags & IAC_GA_DO)
   {
      *stack++ = (char) IAC;
      *stack++ = (char) GA;
   }
   if (p->flags & EOR_ON)
   {
      *stack++ = (char) IAC;
      *stack++ = (char) EOR;
   }
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}

