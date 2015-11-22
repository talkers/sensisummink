/*
 * glue.c
 */

#include "include/config.h"

#include <sys/types.h>
#include <sys/stat.h>
#if defined( LINUX ) && defined ( GLIBC )
#define __STRICT_ANSI__
#include <sys/socket.h>
#undef __STRICT_ANSI__
#else
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <stdarg.h>
#include <sys/ioctl.h>

#ifdef SUNOS
#include <varargs.h>
#define SOCKET_DEFINES
#define TIME_DEFINES
#define RLIMIT_DEFINES
#define VARARG_DEFINES 
#endif

#ifdef INTERCOM
#ifdef SOLARIS
#include <sys/filio.h>
#endif /* SOLARIS */
#include <sys/un.h>
#endif /* INTERCOM */

#ifdef CRASH_RECOVER
#include <setjmp.h>
#endif

#include "include/missing_headers.h"
#include "include/proto.h"
#ifdef INTERCOM
#include "include/intercom.h"
#endif

#ifdef SOLARIS
 extern char *sys_errlist[];
#endif

#define ABS(x) (((x)>0)?(x):-(x))

/* interns */
char           *tens_words[] = {"", "ten", "twenty", "thirty", "forty", "fifty",
                                "sixty", "seventy", "eighty", "ninety"};
char           *units_words[] = {"none", "one", "two", "three", "four", "five",
                                 "six", "seven", "eight", "nine"};
char           *teens[] = {"ten", "eleven", "twelve", "thirteen", "fourteen",
                           "fifteen", "sixteen", "seventeen", "eighteen",
                           "nineteen"};

char           *months[12] = {"January", "February", "March", "April", "May",
                              "June", "July", "August", "September", "October",
                              "November", "December"};
char shutdown_reason[256] = "";
void do_backup(void);
void close_down(void);

#ifdef INTERCOM
 void start_intercom(void);
#ifdef SUNOS
  static void send_to_intercom(player *,char *,...);
#else
  static void send_to_intercom(player *,const char *,...);
#endif
 nameban *nameban_anchor=0;
 nameban *check_intercom_banished_name(char *);
 player *make_dummy_intercom_player(void);
#endif

#ifdef CRASH_RECOVER
#include "include/crashrec_glue.c"
#endif

/* print up birthday */

char           *birthday_string(time_t bday)
{
   static char     bday_string[50];
   struct tm      *t;
   t = localtime(&bday);
   if ((t->tm_mday) > 10 && (t->tm_mday) < 20)
      sprintf(bday_string, "%dth of %s", t->tm_mday, months[t->tm_mon]);
   else
      switch ((t->tm_mday) % 10)
      {
    case 1:
       sprintf(bday_string, "%dst of %s", t->tm_mday, months[t->tm_mon]);
       break;
    case 2:
       sprintf(bday_string, "%dnd of %s", t->tm_mday, months[t->tm_mon]);
       break;
    case 3:
       sprintf(bday_string, "%drd of %s", t->tm_mday, months[t->tm_mon]);
       break;
    default:
       sprintf(bday_string, "%dth of %s", t->tm_mday, months[t->tm_mon]);
       break;
      }
   return bday_string;
}

/* short system time - used for timed operations (currently just backups) */
char	       *short_time(void)
{
   time_t	t;
   static char	time_string[9];
   memset(time_string, 0, sizeof(time_string));
   t = time(0);
   strftime(time_string, 9, "%H:%M:%S", localtime(&t));
   return time_string;
}


/* return a string of the system time */

char           *sys_time(void)
{
   time_t          t;
   static char     time_string[25];
   t = time(0);
   strftime(time_string, 25, "%H:%M:%S - %d/%m/%y", localtime(&t));
   return time_string;
}

/* returns converted user time */

char           *convert_time(time_t t)
{
   static char     time_string[50];
   strftime(time_string, 49, "%I.%M:%S %p - %a, %d %B", localtime(&t));
   return time_string;
}

/* get local time for all those americans :) */

char           *time_diff(int diff)
{
   time_t          t;
   static char     time_string[50];

   t = time(0) + 3600 * diff;
   strftime(time_string, 49, "%I.%M:%S %p - %a, %d %B", localtime(&t));
   return time_string;
}

char           *time_diff_sec(time_t last_on, int diff)
{
   static char     time_string[50];
   time_t             sec_diff;

   sec_diff = (3600 * diff) + last_on;
   strftime(time_string, 49, "%I.%M:%S %p - %a, %d %B", localtime(&sec_diff));
   return time_string;
}



/* converts time into words */

char           *word_time(int t)
{
   static char     time_string[100], *fill;
   int             days, hrs, mins, secs;
   if (!t)
      return "no time at all";
   days = t / 86400;
   hrs = (t / 3600) % 24;
   mins = (t / 60) % 60;
   secs = t % 60;
   fill = time_string;
   if (days)
   {
      sprintf(fill, "%d day", days);
      while (*fill)
    fill++;
      if (days != 1)
    *fill++ = 's';
      if (hrs || mins || secs)
      {
    *fill++ = ',';
    *fill++ = ' ';
      }
   }
   if (hrs)
   {
      sprintf(fill, "%d hour", hrs);
      while (*fill)
    fill++;
      if (hrs != 1)
    *fill++ = 's';
      if (mins && secs)
      {
    *fill++ = ',';
    *fill++ = ' ';
      }
      if ((mins && !secs) || (!mins && secs))
      {
    strcpy(fill, " and ");
    while (*fill)
       fill++;
      }
   }
   if (mins)
   {
      sprintf(fill, "%d minute", mins);
      while (*fill)
    fill++;
      if (mins != 1)
    *fill++ = 's';
      if (secs)
      {
    strcpy(fill, " and ");
    while (*fill)
       fill++;
      }
   }
   if (secs)
   {
      sprintf(fill, "%d second", secs);
      while (*fill)
    fill++;
      if (secs != 1)
    *fill++ = 's';
   }
   *fill++ = 0;
   return time_string;
}

/* returns a number in words */

char           *number2string(int n)
{
   int             hundreds, tens, units;
   static char     words[50];
   char           *fill;
   if (n >= 1000)
   {
      sprintf(words, "%d", n);
      return words;
   }
   if (!n)
      return "none";
   hundreds = n / 100;
   tens = (n / 10) % 10;
   units = n % 10;
   fill = words;
   if (hundreds)
   {
      sprintf(fill, "%s hundred", units_words[hundreds]);
      while (*fill)
    fill++;
   }
   if (hundreds && (units || tens))
   {
      strcpy(fill, " and ");
      while (*fill)
    fill++;
   }
   if (tens && tens != 1)
   {
      strcpy(fill, tens_words[tens]);
      while (*fill)
    fill++;
   }
   if (tens != 1 && tens && units)
      *fill++ = ' ';
   if (units && tens != 1)
   {
      strcpy(fill, units_words[units]);
      while (*fill)
    fill++;
   }
   if (tens == 1)
   {
      strcpy(fill, teens[(n % 100) - 10]);
      while (*fill)
    fill++;
   }
   *fill++ = 0;
   return words;
}

/* point to after a string */

char           *end_string(char *str)
{
   str = strchr(str, 0);
   str++;
   return str;
}

/* get gender string function */

char           *get_gender_string(player * p)
{
   switch (p->gender)
   {
    case MALE:
    return "him";
    break;
      case FEMALE:
    return "her";
    break;
      case PLURAL:
    return "them";
    break;
      case OTHER:
    return "it";
    break;
      case VOID_GENDER:
    return "it";
    break;
   }
   return "(this is frogged)";
}

/* get gender string for possessives */

char           *gstring_possessive(player * p)
{
   switch (p->gender)
   {
    case MALE:
    return "his";
    break;
      case FEMALE:
    return "her";
    break;
      case PLURAL:
    return "their";
    break;
      case OTHER:
    return "its";
    break;
      case VOID_GENDER:
    return "its";
    break;
   }
   return "(this is frogged)";
}


/* more gender strings */

char           *gstring(player * p)
{
   switch (p->gender)
   {
    case MALE:
    return "he";
    break;
      case FEMALE:
    return "she";
    break;
  case PLURAL:
    return "they";
    break;
      case OTHER:
    return "it";
    break;
      case VOID_GENDER:
    return "it";
    break;
   }
   return "(this is frogged)";
}

char *havehas(player *p)
{
  switch (p->gender)
    {
    case PLURAL:
      return "have";
      break;
    default:
      return "has";
      break;
    }
}
char *isare(player *p)
{
  switch (p->gender)
    {
    case PLURAL:
      return "are";
      break;
    default:
      return "is";
      break;
    }
}

char *waswere(player *p)
{
  switch (p->gender)
    {
    case PLURAL:
      return "were";
      break;
    default:
      return "was";
      break;
    }
}


char *plural_es(player *p)
{
   /* returns 'es' for a single player */
   switch (p->gender) {
     case PLURAL:
       return "";
       break;
     default:
       return "es";
       break;
   }
}


char *numeric_s(int foo)
{
  if(foo==1 || foo==-1)
    return "";
  else
    return "s";
}

char *numeric_es(int foo)
{
  if(foo==1 || foo==-1)
    return "";
  else
    return "s";
}

char *single_s(player *p)
{
    /* for use when you want an s returns for a SINGULAR player */
    switch (p->gender)
    {
      case PLURAL:
	return "";
	break;
      default:
	return "s";
	break;
    }
}


char *trailing_s(player *p)
{
   /* for ruddles' etc */
   if(tolower(p->name[strlen(p->name)-1])=='s')
     return "";
   else
     return "s";
}


/* returns the 'full' name of someone, that is their pretitle and name */

char           *full_name(player * p)
{
   static char     fname[MAX_PRETITLE + MAX_NAME];
   if ((!(sys_flags & NO_PRETITLES)) && (p->residency & BASE) && p->pretitle[0])
   {
      sprintf(fname, "%s %s", p->pretitle, p->name);
      return fname;
   }
   return p->name;
}



/* log errors and things to file */

void            log(char *filename, char *string)
{
   int             fd, length;

   sprintf(stack, "logs/%s.log", filename);
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
     sprintf(stack, "%s (failed) - %s\n", filename, string);
     printf(stack);
     return;
   }
   length = lseek(fd, 0, SEEK_END);
   if (length > MAX_LOG_SIZE)
   {
      close(fd);
#if defined( FREEBSD )
      fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
      fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
      if(fd<0) {
        sprintf(stack, "%s (failed) - %s\n", filename, string);
        printf(stack);
        return;
      }
   }
   sprintf(stack, "%s - %s\n", sys_time(), string);
   if (!(sys_flags & NO_PRINT_LOG))
      printf(stack);
   write(fd, stack, strlen(stack));
   close(fd);
}



/* what happens when *shriek* an error occurs */
void            handle_error(char *error_msg)
{
   if (sys_flags & PANIC)
   {
      stack = stack_start;
      log("error", "Immediate PANIC shutdown.");
      exit(-1);
   }
   sys_flags |= PANIC;

   /*
    * sprintf(dump,"gcore %d",getpid()); system(dump);
    */

   stack = stack_start;

   if ( sys_flags & UPDATE )
      sys_flags &= ~NO_PRINT_LOG;

   log("error", error_msg);
   log("boot", "Abnormal exit from error handler");

   /* dump possible useful info */

   log("dump", "------------ Starting dump");

   sprintf(stack_start, "Errno set to %d, %s", errno, sys_errlist[errno]);
   stack = end_string(stack_start);
   log("dump", stack_start);

   if (current_player)
   {
      log("dump", current_player->name);
      if (current_player->location)
      {
    sprintf(stack_start, "player %s.%s",
       current_player->location->owner->lower_name,
       current_player->location->id);
    stack = end_string(stack_start);
    log("dump", stack_start);
      } else
    log("dump", "No room of current player");

      sprintf(stack_start, "flags %d saved %d residency %d", current_player->flags,
         current_player->saved_flags, current_player->residency);
      stack = end_string(stack_start);
      log("dump", stack_start);
      log("dump", current_player->ibuffer);
   } else
      log("dump", "No current player !");
   if (current_room)
   {
      sprintf(stack_start, "current %s.%s", current_room->owner->lower_name,
         current_room->id);
      stack = end_string(stack_start);
      log("dump", stack_start);
   } else
      log("dump", "No current room");

   sprintf(stack_start, "global flags %d, players %d", sys_flags, current_players);
   stack = end_string(stack_start);
   log("dump", stack_start);

   sprintf(stack_start, "action %s", action);
   stack = end_string(stack_start);
   log("dump", stack_start);

   log("dump", "---------- End of dump info");

   raw_wall("\n\n"
"      -=> *WIBBLE* Something bad has happened. Trying to save files <=-\007\n\n\n");

   close_down();
   exit(-1);
}


/* function to convert seamlessly to caps (ish) */

char           *caps(char *str)
{
   static char     buff[500];
   strncpy(buff, str, 498);
   buff[0] = toupper(buff[0]);
   return buff;
}


/* load a file into memory */

file            load_file_verbose(char *filename, int verbose)
{
   file            f;
   int             d;
   char           *oldstack;

   oldstack = stack;

   d = open(filename, O_RDONLY);
   if (d < 0)
   {
      if(verbose)
        vlog("error", "Can't find file:%s", filename);
      f.where = (char *) MALLOC(1);
      *(char *) f.where = 0;
      f.length = 0;
      return f;
   }
   f.length = lseek(d, 0, SEEK_END);
   lseek(d, 0, SEEK_SET);
   f.where = (char *) MALLOC(f.length + 1);
   memset(f.where, 0, f.length + 1);
   if (read(d, f.where, f.length) < 0)
   {
      vlog("error", "Error reading file:%s", filename);
      f.where = (char *) MALLOC(1);
      *(char *) f.where = 0;
      f.length = 0;
      return f;
   }
   close(d);
   if (sys_flags & VERBOSE)
      vlog("boot", "Loaded file:%s", filename);

   stack = oldstack;
   *(f.where + f.length) = 0;
   return f;
}

file            load_file(char *filename)
{
   return load_file_verbose(filename, 1);
}

int save_file(file *f, char *filename)
{
	FILE *fp;

	if (NULL == (fp = fopen(filename, "w")))
		return 0;

	if (-1 == fwrite(f->where, f->length, 1, fp))
		vlog("files", "Failed to save %s\n", filename);
	fclose(fp);
	return (1);
}

/* convert a string to lower case */
/*
 * STUPIDITY!!!!!
 * PLS NOTE!!!    *str++=tolower(*str); does -=NOT=- do what you would expect
 * on OSF machines!  It gets confused.  never copy something to itself while
 * moving the pointer!!!
 */
void            lower_case(char *str)
{
   while(*str) {
     *str = tolower(*str);
     str++;
   }
}

/* fns to block signals */

void            sigpipe(int dummy)
{
   if (current_player)
   {
      log("sigpipe", "Closing connection due to sigpipe");
      shutdown(current_player->fd, 0);
      close(current_player->fd);
   } else
   {
      log("sigpipe", "Eeek! sigpipe but no current_player");
   }
#ifndef USE_SIGACTION
   signal(SIGPIPE, sigpipe);
#endif
   return;
}

void            sighup(int dummy)
{
   log("boot", "Terminated by hangup signal");
   close_down();
   exit(0);
}
void            sigquit(int dummy)
{
   handle_error("Quit signal received.");
}
void            sigill(int dummy)
{
   handle_error("Illegal instruction.");
}
void            sigfpe(int dummy)
{
   handle_error("Floating Point Error.");
}
void            sigbus(int dummy)
{
   handle_error("Bus Error.");
}
void            sigsegv(int dummy)
{
   handle_error("Segmentation Violation.");
}
#ifndef NO_SIGSYS
void            sigsys(int dummy)
{
   handle_error("Bad system call.");
}
#endif /* no_sigsys */
void            sigterm(int dummy)
{
   handle_error("Terminate signal received.");
}
void            sigxfsz(int dummy)
{
   handle_error("File descriptor limit exceeded.");
}
void            sigusr1(int dummy)
{
/* dyathink he could have made this a bit longer? */
   fork_the_thing_and_sync_the_playerfiles();
#ifndef USE_SIGACTION
   signal(SIGUSR1, sigusr1);
#endif /* use_sigaction */
}


void            sigchld(int dummy)
{
/*
   log("error", "WIbble, server's child died");
*/
#ifndef USE_SIGACTION
   signal(SIGCHLD, sigchld);
#endif /* use_sigaction */
   return;
}



/* save all players */
void save_all(void)
{
   player *scan;
   
   for(scan=flatlist_start; scan; scan=scan->flat_next)
     save_player(scan);
}


/* close down sequence */
void            close_down(void)
{
   player         *scan, *old_current;
   struct itimerval new, old;
   
   raw_wall("\007\n\n");
   command_type |= (HIGHLIGHT|PERSONAL|WARNING);
   if (shutdown_count == 0)
      raw_wall(shutdown_reason);

   raw_wall("\n\n\n          ---====>>>> Program shutting down NOW <<<<====---"
       "\n\n\n");
   command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);

   new.it_interval.tv_sec = 0;
   new.it_interval.tv_usec = 0;
   new.it_value.tv_sec = 0;
   new.it_value.tv_usec = new.it_interval.tv_usec;
   if (setitimer(ITIMER_REAL, &new, &old) < 0)
      handle_error("Can't set timer.");
   if (sys_flags & VERBOSE || sys_flags & PANIC)
      log("boot", "Timer Stopped");

   if (sys_flags & VERBOSE || sys_flags & PANIC)
      log("boot", "Saving all players.");
   sys_flags |= INVIS_SAVE;
   save_all();
   sys_flags &= ~INVIS_SAVE;
   if (sys_flags & VERBOSE || sys_flags & PANIC)
#ifdef DYNAMIC
      log("boot", "Syncing to disk (dfiles and cache stuff).");
#else
      log("boot", "Syncing to disk.");
#endif
   sync_all();

   old_current = current_player;
   current_player = 0;
   sync_notes(0);
   current_player = old_current;

   if (sys_flags & PANIC)
      raw_wall("\n\n              ---====>>>> Files sunc (phew !) <<<<====---"
          "\007\n\n\n");
   for (scan = flatlist_start; scan; scan = scan->flat_next) 
     if(scan->fd>=0)
      close(scan->fd);

#ifdef INTERCOM
   kill_intercom();
#endif

   close_down_socket();
   
   unlink("PID_FILE");

   if (!(sys_flags & PANIC))
   {
      log("boot", "Program exited normally.");
      exit(0);
   }
}


/* the boot sequence */

void boot(void)
{
   char *oldstack;
   int i;
   struct rlimit   rlp;
   struct itimerval new, old;
#ifdef USE_SIGACTION
   struct sigaction sa;
#endif /* use_sigaction */

   oldstack = stack;

   /* tidyness ;) */
   printf("\n");
   
   up_date = time(0);

/*ifdef LINUX*/
   getrlimit(RLIMIT_NOFILE, &rlp);
   rlp.rlim_cur = rlp.rlim_max;
   setrlimit(RLIMIT_NOFILE, &rlp);
/*endif  */
   max_players = 210;
   if (sys_flags & VERBOSE)
      vlog("boot", "Got %d file descriptors, Allocated %d for players",
         (int)rlp.rlim_cur, max_players);

#ifndef NO_RLIMIT_RSS
   getrlimit(RLIMIT_RSS, &rlp);
   rlp.rlim_cur = MAX_RES;
   setrlimit(RLIMIT_RSS, &rlp);
#endif /* no_rlimit_rss */

   flatlist_start = 0;
   for (i = 0; i < 27; i++)
      hashlist[i] = 0;

   stdout_player = (player *) MALLOC(sizeof(player));
   memset(stdout_player, 0, sizeof(player));

   srand(time(0));

   load_files(1);
#ifdef IDENT
    init_ident_server();
#endif
   
   /* socket bit now so we can ping the angel whilst booting */
   if (!(sys_flags & SHUTDOWN))
     alive_connect();
   
   init_plist();
   init_parser();
   init_rooms();
   init_notes();
   init_help();
#ifdef ROBOTS
   init_robots();
#endif

   if (!(sys_flags & SHUTDOWN))
   {
      new.it_interval.tv_sec = 0;
      new.it_interval.tv_usec = (1000000 / TIMER_CLICK);
      new.it_value.tv_sec = 0;
      new.it_value.tv_usec = new.it_interval.tv_usec;
#ifdef USE_SIGACTION
      sa.sa_handler = actual_timer;
#ifdef USE_SIGEMPTYSET
       sigemptyset(&sa.sa_mask);
#else /* use_sigemptyset */
       sa.sa_mask = 0;
#endif /* use_sigemptyset */
      sa.sa_flags = 0;
      if ((int) sigaction(SIGALRM, &sa, 0) < 0)
#else /* use_sigaction */
      if ((int) signal(SIGALRM, actual_timer) < 0)
#endif /* use_sigaction */
         handle_error("Can't set timer signal.");
      if (setitimer(ITIMER_REAL, &new, &old) < 0)
         handle_error("Can't set timer.");
      if (sys_flags & VERBOSE)
    log("boot", "Timer started.");
   }

#ifdef USE_SIGACTION
   sa.sa_handler = sigpipe;
#ifdef USE_SIGEMPTYSET
    sigemptyset(&sa.sa_mask);
#else /* use_sigemptyset */
    sa.sa_mask = 0;
#endif /* use_sigemptyset */
   sa.sa_flags = 0;
   sigaction(SIGPIPE, &sa, 0);
   sa.sa_handler = sighup;
   sigaction(SIGHUP, &sa,0);
   sa.sa_handler = sigquit;
   sigaction(SIGQUIT, &sa, 0);
   sa.sa_handler = sigill;
   sigaction(SIGILL, &sa, 0);
   sa.sa_handler = sigfpe;
   sigaction(SIGFPE, &sa, 0);
   sa.sa_handler = sigbus;
   sigaction(SIGBUS, &sa, 0);
   sa.sa_handler = sigsegv;
   sigaction(SIGSEGV, &sa, 0);
#ifndef NO_SIGSYS
   sa.sa_handler = sigsys;
   sigaction(SIGSYS, &sa, 0);
#endif /* no_sigsys */
   sa.sa_handler = sigterm;
   sigaction(SIGTERM, &sa, 0);
   sa.sa_handler = sigxfsz;
   sigaction(SIGXFSZ, &sa, 0);
   sa.sa_handler = sigusr1;
   sigaction(SIGUSR1, &sa, 0);
   sa.sa_handler = sigchld;
   sigaction(SIGCHLD, &sa, 0);
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
   signal(SIGUSR1, sigusr1);
   signal(SIGCHLD, sigchld);
#endif /* use_sigaction */

#ifdef INTERCOM
   intercom_port = talker_port-1;
#endif

   if (!(sys_flags & SHUTDOWN))
      init_socket();

   current_players = 0;

   stack = oldstack;
}


/* got to have a main to control everything */

void main(int argc, char *argv[])
{
   FILE *pid_fd;

   action = "boot";
   /*
    * if (mallopt(M_MXFAST,1024)) { perror("spoon:"); exit(0); }
    */


#ifdef MALLOC_DEBUG
   malloc_debug(2);
#endif

   stack_start = (char *) MALLOC(STACK_SIZE);
   align(stack_start);
   memset(stack_start, 0, STACK_SIZE);
   stack = stack_start;

   if (argc == 3)
   {
      if (!strcasecmp("update", argv[1]))
      {
         if (!strcasecmp("rooms", argv[2]))
         {
            log("boot", "Program booted for file rooms update.");
            sys_flags |= SHUTDOWN | UPDATEROOMS;
         } else if (!strcasecmp("flags", argv[2]))
         {
            log("boot", "Program booted for flags update");
            sys_flags |= SHUTDOWN | UPDATEFLAGS;
         } else
         {
            log("boot", "Program booted for file players update.");
            sys_flags |= SHUTDOWN | UPDATE;
         }
      }
   }
   if (argc == 2)
      talker_port = atoi(argv[1]);

   if (!talker_port)
      talker_port = DEFAULT_PORT;

   if (chdir(ROOT))
   {
      printf("Can't change to root directory.\n");
      exit(1);
   }
   boot();
   
#ifdef INTERCOM
   start_intercom();
#endif

   if (sys_flags & UPDATE)
      do_update(0);
   else if (sys_flags & UPDATEFLAGS)
      do_update(0);
   else if (sys_flags & UPDATEROOMS)
      do_update(1);
   sys_flags |= NO_PRINT_LOG;

   if ( !(sys_flags & UPDATE) )
   {
      /*
      fclose(stdout);
      fclose(stderr);
      */
   }
   
   /* ident server and pid files */
   if(!(sys_flags & SHUTDOWN)) {
      unlink(PID_FILE);
      if(!(pid_fd = fopen(PID_FILE,"w"))) {
         fprintf(stderr, "Unable to create pid log file %s\n", PID_FILE);
         exit(1);
      }
      fprintf(pid_fd, "%d\n", (int) getpid());
      fclose(pid_fd);
   }

   /* time to put in the anti crash code if required */
#ifdef CRASH_RECOVER
    setjmp(recover_jmp_env);
    signal(SIGSEGV, mid_program_error);
    signal(SIGBUS, mid_program_error);
#endif
   
   while (!(sys_flags & SHUTDOWN))
   {
      errno = 0;
/* this used to be here in 3.1a
      if (backup)
	do_backup();*/
	
      /* new version of backup - check time against backup_time 
         This idea could very easily be expanded to have an array of timed
         operations - as done in Cult */
      if(!strncasecmp(BACKUPS_TIME, short_time(), 8)) {
        while(!strncasecmp(BACKUPS_TIME, short_time(), 8));
        do_backup();
      }

      if (stack != stack_start)
      {
#ifdef OSF	
          sprintf(stack_start, "Lost stack reclaimed %ld bytes\n",
                 (long) stack - (long) stack_start);
#else
          sprintf(stack_start, "Lost stack reclaimed %d bytes\n",
                 (int) stack - (int) stack_start);
#endif
         stack = end_string(stack_start);
         log("stack", stack_start);
         stack = stack_start;
      }
      action = "scan sockets";
      current_player = 0;
      current_room = 0;
      scan_sockets();
      action = "processing players";
      current_player = 0;
      current_room = 0;
      process_players();
#ifdef ROBOTS
      action = "processing robots";
      current_player = 0;
      current_room = 0;
      process_robots();
#endif
      action = "timer_function";
      current_player = 0;
      current_room = 0;
      timer_function();
      action = "sigpause";
      sigpause(0);
      action = "ping";
      do_alive_ping();
   }
   close_down();
}


/* support function - read in backup script templates and output usable by
   server ones - returns 0 for failures */
int convert_backup_script_templates(void)
{
  file temp, out;
  char *oldstack = stack, *step;
  int item=0;
  
  temp = load_file("files/stuff/daily.backup.1.template");
  if(!temp.length || !temp.where || !*temp.where)
    return 0;
    
  /* step through file, change @'s as appropriate */
  step = temp.where;
  while(*step) {
    if(*step=='@') {
      switch(item) {
        case 0:
          #ifdef DYNAMIC
            strcpy(stack, "# Line unused for dynamic playerfiles.");
          #else
            strcpy(stack, "rm -f files/players/backup_write");
          #endif
          stack = strchr(stack, 0);
          break;
        case 1:
          #ifdef DYNAMIC
            strcpy(stack, "files/dynamic");
          #else
            strcpy(stack, "files/players");
          #endif
          stack = strchr(stack, 0);
          break;
      }
      item++;
      step++;
    }
    else
      *stack++=*step++;
  }
  /* we've finished with temp.where now. */
  FREE(temp.where);
  *stack++=0;
  out.length=strlen(oldstack);
  out.where = oldstack;
  if(!save_file(&out, "backup/daily.backup.pt.1")) {
    stack = oldstack;
    return 0;
  }
  
  stack = oldstack;
  item=0;
  
  /* and here we go again, different file. */
  temp = load_file("files/stuff/daily.backup.2.template");
  if(!temp.length || !temp.where || !*temp.where)
    return 0;
    
  /* step through file, change @'s as appropriate */
  step = temp.where;
  while(*step) {
    if(*step=='@') {
      switch(item) {
        case 0:
          strcpy(stack, BACKUPS_DIR);
          stack = strchr(stack, 0);
          break;
        case 1:
          strcpy(stack, ROOT);
          stack = strchr(stack, 0);
          break;
      }
      item++;
      step++;
    }
    else
      *stack++=*step++;
  }
  /* we've finished with temp.where now. */
  FREE(temp.where);
  *stack++=0;
  out.length=strlen(oldstack);
  out.where = oldstack;
  if(!save_file(&out, "backup/daily.backup.pt.2")) {
    stack = oldstack;
    return 0;
  }
  
  chmod("backup/daily.backup.pt.1", (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));
  chmod("backup/daily.backup.pt.2", (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP));
  stack = oldstack;
  return 1;
}

void do_backup(void)
{
  char fname[256];
  
  if(!convert_backup_script_templates()) {
    log("error", "Failed to create backup scripts from templates!");
    return;
  }

  raw_wall("\n\n-=> Syncing all the files for the daily backups.... <=-\n"
           "-=> Program pausing to save files                   <=-\n\n");
  raw_wall("-=> Validating all rooms                            <=-\n");
  dynamic_validate_rooms((player *)0, (char *)0);
  raw_wall("-=> Defragmenting rooms                             <=-\n");
  dynamic_defrag_rooms((player *)0,(char *)0);
  raw_wall("-=> Handling mail and news                          <=-\n");
  sync_notes(0);
  raw_wall("-=> Saving all resident users                       <=-\n");
  sys_flags |= INVIS_SAVE;
  save_all();
  sys_flags &= ~INVIS_SAVE;
#ifdef DYNAMIC
  raw_wall("-=> Syncing dynamic players cache                   <=-\n");
#endif
  raw_wall("-=> Syncing all dynamic files                       <=-\n");
  sync_all();
  strcpy(fname,ROOT);
  strcat(fname,"backup/daily.backup.pt.1");
  system(fname);
  raw_wall("-=> Backup complete                                 <=-\n\n\n");
  switch((int) fork())
    {
    case 0:
      fname[strlen(fname)-1]='2';
      system(fname);
      exit(0);
    default:
      break;
    }
}


void free_files(void)
{
   /* this must ALWAYS be the first one! */
   if (banish_file.where)
      FREE(banish_file.where);
   if (banish_msg.where)
      FREE(banish_msg.where);
   if (banish_scan.where)
      FREE(banish_scan.where);
   if (banned_msg.where)
      FREE(banned_msg.where);
   if (connect_msg.where)
      FREE(connect_msg.where);
   if (disclaimer_msg.where)
      FREE(disclaimer_msg.where);
   if (full_msg.where)
      FREE(full_msg.where);
   if (motd_msg.where)
      FREE(motd_msg.where);
   if (newban_msg.where)
      FREE(newban_msg.where);
   if (newbie_msg.where)
      FREE(newbie_msg.where);
   if (newpage1_msg.where)
      FREE(newpage1_msg.where);
   if (newpage2_msg.where)
      FREE(newpage2_msg.where);
   if (nonewbies_msg.where)
      FREE(nonewbies_msg.where);
   if (splat_msg.where)
      FREE(splat_msg.where);
   if (hitells_msg.where)
      FREE(hitells_msg.where);
   if (sig_file.where)
      FREE(sig_file.where);
}


/* x tells this whether or not to load the banish file */
void load_files(int x)
{
   if (x==1) banish_file = load_file("files/stuff/banish");
   banish_msg = load_file("files/messages/banish.msg");
   banish_scan = load_file("files/stuff/banish_scan");
   banned_msg = load_file("files/messages/banned.msg");
   connect_msg = load_file("files/messages/connect.msg");
   disclaimer_msg = load_file("files/messages/disclaimer.msg");
   full_msg = load_file("files/messages/full.msg");
   motd_msg = load_file("files/messages/motd.msg");
   newban_msg = load_file("files/messages/newban.msg");
   newbie_msg = load_file("files/messages/newbie.msg");
   newpage1_msg = load_file("files/messages/newpage1.msg");
   newpage2_msg = load_file("files/messages/newpage2.msg");
   nonewbies_msg = load_file("files/messages/nonew.msg");
   splat_msg = load_file("files/messages/splat.msg");
   hitells_msg = load_file("files/messages/hitells.msg");
   sig_file = load_file("files/messages/signature.msg");
}


/* reload everything */

void reload(player * p, char *str)
{
   tell_player(p, " Loading help\n");
   init_help();
   tell_player(p, " Loading messages\n");
   free_files();
   load_files(1);
   tell_player(p, " Done\n");
}



/* a thing for a user to manually trigger off backups */
void		backup_command(player *p, char *str)
{
   tell_player(p, " Done.\n");
   do_backup();
}


/* our vararg versions of things! */


/* and something cunning to use the above and cut down typing */
void	vlog(char *filename, char *format, ...)
{
  va_list	arguments;
  char		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(oldstack);
  log(filename, oldstack);
  stack = oldstack;
}


/* shout to su's - vararg version */
void            vsu_wall(char *format, ...)
{
  va_list	arguments;
  char		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
}


/* shout to sus bar one */
void		vsu_wall_but(player *p, char *format, ...)
{
  va_list	arguments;
  char		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  su_wall_but(p, oldstack);
  stack = oldstack;
}


/* tell_player shortcut past stacks */
void	vtell_player(player *p, char *format, ...)
{
  va_list 	arguments;
  char 		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* tell_player shortcut past stacks */
void	vtell_room(room *r, char *format, ...)
{
  va_list 	arguments;
  char 		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  tell_room(r, oldstack);
  stack = oldstack;
}


/* tell_player shortcut past stacks */
void	vtell_room_but(player *p, room *r, char *format, ...)
{
  va_list 	arguments;
  char 		*oldstack;
  
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  tell_room_but(p, r, oldstack);
  stack = oldstack;
}


/* tell_current shortcut past stacks */
void	vtell_current(char *format, ...)
{
  va_list 	arguments;
  char 		*oldstack;
  
  if(!current_player)
    return;
  oldstack = stack;
  va_start(arguments, format);
  vsprintf(stack, format, arguments);
  va_end(arguments);
  stack = end_string(stack);
  tell_player(current_player, oldstack);
  stack = oldstack;
}


/* intercom code plugin */
#ifdef INTERCOM
#include "include/intercom_glue.c"
#endif
