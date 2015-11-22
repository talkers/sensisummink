/*
 * Plists.c
 */

#include "include/config.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef FREEBSD228
#include <malloc.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined( LINUX ) && defined ( GLIBC )
#define __STRICT_ANSI__
#include <netinet/in.h>
#undef __STRICT_ANSI__
#else
#include <netinet/in.h>
#endif
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <setjmp.h>
#ifdef DYNAMIC
#include <dirent.h>
#endif
#ifdef IRIX
#include <crypt.h>
#endif

#ifdef SUNOS
#define SOCKET_DEFINES
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"
#include "include/proto.h"
#ifdef ANSI_COLS
#include "include/colours.h"
#endif
#ifdef IDENT
#include "include/ident.h"
#endif


/* interns */
#ifdef DYNAMIC
cache		dynamic_cache[MAX_DYNAMIC_CACHE];
file		dynamic_tempfile;
#endif

void            error_on_load(int);
void		hard_load_files(void);
char            player_loading[MAX_NAME + 2];
jmp_buf         jmp_env;
saved_player  **saved_hash[26];
int             update[26];
void            newbie_check_name(player *, char *);
void		link_to_program(player *);
saved_player  **birthday_list = 0;
char            uncompact_table[7][16] = {
   {0, ' ', '\n', 'a', 'e', 'h', 'i', 'n', 'o', 's', 't', 1, 2, 3, 4, 5},
   {'b', 'c', 'd', 'f', 'g', 'j', 'k', 'l', 'm', 'p', 'q', 'r', 'u', 'v', 'w', 'x'},
   {'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N'},
   {'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"', '#', '@'},
   {'~', '&', 39,  '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4'},
   {'5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '[', '\\', ']', '^', 6},
   {'`', '_', '{', '|', '}', '%', '$', -1, 0, 0, 0, 0, 0, 0, 0, 0}};
char            compact_table[128] = {
   1, 60, 61, 62, 102, 101, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 63, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
   47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 91, 92, 93, 94, 97, 96, 3, 16, 17, 18, 4,
   19, 20, 5, 6, 21, 22, 23, 24, 7, 8, 25, 26, 27, 9, 10, 28, 29, 30, 31, 32, 33, 98, 99, 100, 64
};


/* stores a nibble */
char           *store_nibble(char *dest, int n)
{
   static int      toggle = 0;
   if (toggle) {
      *dest++ |= (char) n;
      toggle = 0;
   } else {
      if (!n) {
	 *dest = 0;
	 dest++;
	 return dest;
      }
      n <<= 4;
      *dest = (char) n;
      toggle = 1;
   }
   return dest;
}

/* retrieves a nibble */
char           *get_nibble(int *n, char *source)
{
   static int      toggle = 0;
   if (toggle) {
      *n = ((int) *source++) & 15;
      toggle = 0;
   } else {
      *n = (((int) *source) >> 4) & 15;
      if (!*n)  source++;
      else      toggle = 1;
   }
   return source;
}

/* returns new source */
char           *get_string(char *dest, char *source)
{
   char            c = 1;
   int             n = 0, table = 0;
   for (; c; table = 0) {
      c = 1;
      while (c && c < 7) {
	 source = get_nibble(&n, source);
	 c = uncompact_table[table][n];
	 if (c && c < 7) table = (int) c;
      }
      *dest++ = c;
   }
   return source;
}

/* returns new destination */
char           *store_string(char *dest, char *source)
{
   int             n, row, tmp = 1;
   for (; tmp; source++) {
      tmp = (int) *source;
      switch (tmp) {
	 case 0:
	    row = 0;
	    n = 0;
	    break;
	 case '\n':
	    row = 0;
	    n = 2;
	    break;
	 case -1:
	    row = 6;
	    n = 7;
	    break;
	 default:
	    row = (compact_table[tmp - 32]) >> 4;
	    n = (compact_table[tmp - 32]) & 15;
	    break;
      }
      switch (row) {
	 case 0:
	    dest = store_nibble(dest, n);
	    break;
	 case 1:
	    dest = store_nibble(dest, 11);
	    dest = store_nibble(dest, n);
	    break;
	 case 2:
	    dest = store_nibble(dest, 12);
	    dest = store_nibble(dest, n);
	    break;
	 case 3:
	    dest = store_nibble(dest, 13);
	    dest = store_nibble(dest, n);
	    break;
	 case 4:
	    dest = store_nibble(dest, 14);
	    dest = store_nibble(dest, n);
	    break;
	 case 5:
	    dest = store_nibble(dest, 15);
	    dest = store_nibble(dest, n);
	    break;
	 case 6:
	    dest = store_nibble(dest, 15);
	    dest = store_nibble(dest, 15);
	    dest = store_nibble(dest, n);
	    break;
      }
   }
   return dest;
}


/* store 4-byte word in player file */
char           *store_int(char *dest, int source)
{
   int             i;
   union {
      char	      c[4];
      int             i;
   }               u;
   u.i = htonl(source);
   for (i = 0; i < 4; i++) *dest++ = u.c[i];
   return dest;
}

/* retrieve 4-byte int from player file */
char           *get_int(int *dest, char *source)
{
   int             i;
   union {
      char	      c[4];
      int             i;
   }               u;
   for (i = 0; i < 4; i++) u.c[i] = *source++;
   *dest = ntohl(u.i);
   return source;
}

/* get int safe - used by kind permission of Oolon
   Allows easy addition to the playerfiles and provides some
   protection against file corruptions */
char *get_int_safe(int *no, char *r, file data)
{
  if ((no) && (r))
#ifdef OSF
     if (((long) r - (long) data.where)<data.length)
#else
     if (((int) r - (int) data.where)<data.length)
#endif
      return get_int(no, r);
    else {
       *no=0;
       return r;
    }
  else {
    if (no) *no=0;
    return r;
  }
}

/* get string safe - as above, but for string types :-) */
char *get_string_safe(char *str, char *r, file data)
{
  if ((str) && (r))
#ifdef OSF
     if (((long) r - (long) data.where)<data.length)
#else
     if (((int) r - (int) data.where)<data.length)
#endif
      return get_string(str, r);
    else {
      *str=0;
      return r;
    }
  else {
    if (str) *str=0;
    return r;
  }
}


/* update functions .. a complete database traversal */

/* crypts the password */

char           *update_password(char *oldpass)
{
   char            key[9];
   strncpy(key, oldpass, 8);
   return crypt(key, "SP");
}

void            players_update_function(player * p)
{
   char em[MAX_EMAIL];

   p->flags |= NO_SAVE_LAST_ON;
   
   lower_case(p->email);
   
   if(p->email[0]==0)
      strcpy(em, "     <NO EMAIL SET>");
   else if (p->email[0] == ' ' && p->email[1]==0)
      strcpy(em, "     <VALIDATED AS SET>");
   else
      strcpy(em, p->email);

    if (p->residency & ADMIN)
       printf("%-18s -- %-40s >(ADMIN)\n", p->name, em);
    else if (p->residency & SU)
       printf("%-18s -- %-40s >(SU)\n", p->name, em);
    else if (p->residency & PSU)
       printf("%-18s -- %-40s >(PSEUDO)\n", p->name, em);
    else
       printf("%-18s -- %s\n", p->name, em); 
}

void            flags_update_function(player * p)
{
   printf("%-18s -- %-40s\n", p->name, bit_string(p->residency));
}

void            rooms_update_function(player * p)
{
   room           *r;
   saved_player   *sp;
   sp = p->saved;
   r = sp->rooms;
   while (r)
   {
      if (r->flags & OPEN)
      {
    decompress_room(r);
    printf("-=> %s.%s (%s)\n", r->owner->lower_name, r->id, r->name);
    if (r->exits.where)
       printf(r->exits.where);
      }
      r = r->next;
   }
}


void            do_update(int rooms)
{
   player         *p;
   saved_player   *scan, **hash;
   int             i, j, fd;

   fd = open("/dev/null", O_WRONLY);
   if(fd<0)
     handle_error("Couldnt open /dev/null!");

   p = (player *) MALLOC(sizeof(player));

   for (j = 0; j < 26; j++)
   {
      /* printf("Updating %c\n",j+'a'); */
      hash = saved_hash[j];
      for (i = 0; i < HASH_SIZE; i++, hash++)
      {
         for (scan = *hash; scan; scan = scan->next)
         {
            if (scan->residency != STANDARD_ROOMS
                && (scan->residency != BANISHD))
            {
               memset((char *) p, 0, sizeof(player));
               p->fd = fd;
               p->script = 0;
               p->location = (room *) - 1;
               restore_player(p, scan->lower_name);
               if (rooms)
                  rooms_update_function(p);
               else if (sys_flags & UPDATE)
                  players_update_function(p);
               else
                  flags_update_function(p);
               save_player(p);
            }
         }
      }
   }
   close(fd);
}


/* return the top player in a hash list */

saved_player   *find_top_player(char c, int h)
{
#ifdef IRIX
    if (c > 25) /* chars in irix are unsigned */
#else
    if ((c < 0) || (c > 25))
#endif
      return 0;
   if ((h < 0) || (h > HASH_SIZE))
      return 0;
   return (*(saved_hash[(int)c] + h));
}


/* birthdays !!! */

void            do_birthdays(void)
{
   player         *p;
   saved_player   *scan, **hash, **list;
   int             i, j, fd;
   time_t         t;
   struct tm      *date, *bday;
   char           *oldstack;

   fd = open("/dev/null", O_WRONLY);
   if(fd<0)
     handle_error("COuldnt open /dev/null!");

   oldstack = stack;
   align(stack);
   list = (saved_player **) ALFIX stack; 

   t = time(0);
   date = localtime(&t);

   p = (player *) MALLOC(sizeof(player));

   for (j = 0; j < 26; j++)
   {
      hash = saved_hash[j];
      for (i = 0; i < HASH_SIZE; i++, hash++)
    for (scan = *hash; scan; scan = scan->next)
       if (scan->residency != STANDARD_ROOMS
           && (!(scan->residency & BANISHD)))
       {
          memset((char *) p, 0, sizeof(player *));
          restore_player(p, scan->lower_name);
          bday = localtime((time_t *)&(p->birthday));
          if ((bday->tm_mon == date->tm_mon) &&
         (bday->tm_mday == date->tm_mday))
          {
        *((saved_player **) ALFIX stack) = scan; 
        stack += sizeof(saved_player *);
        p->age++;
        save_player(p);
          }
       }
   }
   *((saved_player **) ALFIX stack) = 0; 
   stack += sizeof(saved_player *);
   if (birthday_list)
      FREE(birthday_list);

#ifdef OSF
    i = (long) stack - (long) list;
#else
    i = (int) stack - (int) list;
#endif
   if (i > 4)
   {
      birthday_list = (saved_player **) MALLOC(i);
      memcpy(birthday_list, list, i);
   } else
      birthday_list = 0;

   close(fd);
}

/* saved player stuff */

/* see if a saved player exists (given lower case name) */

saved_player   *find_saved_player(char *name)
{
   saved_player  **hash, *list;
   int             sum = 0;
   char           *c;

   if (!isalpha(*name))
      return 0;
   hash = saved_hash[((int) (tolower(*name)) - (int) 'a')];
   for (c = name; *c; c++)
   {
      if (isalpha(*c))
         sum += (int) (tolower(*c)) - 'a';
      else
    return 0;
   }
   list = *(hash + (sum % HASH_SIZE));
   for (; list; list = list->next)
      if (!strcmp(name, list->lower_name))
         return list;
   return 0;
}


/* hard load and save stuff (ie to disk and back) */



/* load in all the player files */

void            hard_load_files()
{
   char            c;
   int             i, hash_length;
   char           *oldstack;
#ifdef USE_SIGACTION
   struct sigaction sa;
   sa.sa_handler = error_on_load;
#ifdef USE_SIGEMPTYSET
    sigemptyset(&sa.sa_mask);
#else
    sa.sa_mask = 0;
#endif /* use_sigemptyset */
   sa.sa_flags = 0;
   sigaction(SIGSEGV, &sa, 0);
   sigaction(SIGSEGV, &sa, 0);
#else /* use_sigaction */
   signal(SIGSEGV, error_on_load);
   signal(SIGBUS, error_on_load);
#endif /* use_sigaction */
   oldstack = stack;
   hash_length = HASH_SIZE * sizeof(saved_player *);
   for (i = 0; i < 26; i++)
   {
      saved_hash[i] = (saved_player **) MALLOC(hash_length);
      memset((void *) saved_hash[i], 0, hash_length);
   }
   for (c = 'a'; c <= 'z'; c++) {
      hard_load_one_file(c);
      do_alive_ping();
   }
}


/* fork and sync the playerfiles */

void            fork_the_thing_and_sync_the_playerfiles(void)
{
   int             fl;
   fl = fork();
   if (fl == -1)
   {
      log("error", "Forked up!");
      return;
   }
   if (fl > 0)
      return;
   sync_all();
   exit(0);
}

/* flicks on the update flag for a particular player hash */

void            set_update(char c)
{
   update[(int) c - (int) 'a'] = 1;
}


/* routines to save a player to the save files */

/* makes the save data onto the stack */
/* Please refer to the comment above the data section in load_player */
file            construct_save_data(player * p)
{
   file            d;
#ifdef ANSI_COLS
    int count;
#endif
   
   d.where = stack;

   stack = store_string(stack, p->name);
   stack = store_string(stack, p->prompt);
   stack = store_string(stack, p->converse_prompt);
   stack = store_string(stack, p->email);
   /*ifndef IRIX leave this in for non IRIX'ers migrating old files. 
     if (p->password[0] == -1)
       p->password[0] = 0;
   endif*/
   stack = store_string(stack, p->password);
   stack = store_string(stack, p->title);
   stack = store_string(stack, p->plan);
   stack = store_string(stack, p->description);
   stack = store_string(stack, p->enter_msg);
   stack = store_string(stack, p->pretitle);
   stack = store_string(stack, p->ignore_msg);
   stack = store_string(stack, p->room_connect);
   stack = store_int(stack, p->term_width);
   stack = store_int(stack, p->word_wrap);
   stack = store_int(stack, p->max_rooms);
   stack = store_int(stack, p->max_exits);
   stack = store_int(stack, p->max_autos);
   stack = store_int(stack, p->max_list);
   stack = store_int(stack, p->max_mail);
   stack = store_int(stack, p->gender);
   stack = store_int(stack, p->no_shout);
   stack = store_int(stack, p->total_login);
   stack = store_int(stack, p->term);
   stack = store_int(stack, p->birthday);
   stack = store_int(stack, p->age);
   stack = store_int(stack, p->jetlag);
   stack = store_int(stack, p->sneezed);
   stack = store_int(stack, p->sflags);
   
#ifdef ANSI_COLS
    for(count=0;count<MAX_COLS;count++)
      stack = store_int(stack, p->colours[count]);
#endif

#ifdef OSF
    d.length = (long) stack - (long) d.where;
#else
    d.length = (int) stack - (int) d.where;
#endif
   stack = d.where;
   return d;
}



/* load and do linking */

int             restore_player(player * p, char *name)
{
   return restore_player_title(p, name, 0);
}

int             restore_player_title(player * p, char *name, char *title)
{
   int             did_load, found_lower;
   char           *n;
   
#ifdef ANSI_COLS
    int count;
#endif

   strncpy(p->name, name, MAX_NAME - 2);
   strncpy(p->lower_name, name, MAX_NAME - 2);
   lower_case(p->lower_name);
   if (!strcmp(p->name, p->lower_name))
      p->name[0] = toupper(p->name[0]);
   found_lower = 0;
   n = p->name;
   while (*n)
   {
      if (*n >= 'a' && *n <= 'z')
      {
         found_lower = 1;
      }
      n++;
   }
   if (!found_lower)
   {
      n = p->name;
      n++;
      while (*n)
      {
         *n = *n - ('A' - 'a');
         n++;
      }
   }

/* Set up a default player structure, methinks */

   sprintf(stack, "Sensi%s> ", VERSION);
   strncpy(p->prompt, stack, MAX_PROMPT-1);
   strncpy(p->converse_prompt, "(Converse) ->", MAX_PROMPT-1);
   strcpy(p->enter_msg, "enters in a standard kind of way.");
   
   p->repeat_string[0] = 0;
   p->repeat_command = 0;
   p->failpasswd = 0;
   p->command_used = 0;
   p->tfreplytime = 0;

#ifdef ANTISPAM
    p->ibuffer_copy[0] = 0;
    p->matched_ibuffer_count = 0;
#endif
   
#ifdef ANSI_COLS
    for(count=0; count<MAX_COLS; count++) 
      p->colours[count] = 0;
#endif

   p->term_width = 79;
   p->column = 0;
   p->word_wrap = 10;
   p->total_login = 0;

   p->gender = VOID_GENDER;
   p->no_shout = 180;

   p->saved_flags = PRIVATE_EMAIL | TAG_ECHO | MAIL_INFORM | NEWS_INFORM | TAG_SHOUT | TAG_PERSONAL | IAC_GA_ON | NOEPREFIX;
   p->sflags = LIST_FAILS;

   strncpy(p->title, "the newbie, so treat me nicely.", MAX_TITLE);
   strncpy(p->description, "Isn't it time I wrote my own description ?",
      MAX_DESC);
   strncpy(p->plan, "I must write myself a proper plan sometime ...", MAX_PLAN);

   p->max_rooms = 2;
   p->max_exits = 5;
   p->max_autos = 5;
   p->max_list = 25;
   p->max_mail = 10;

   p->birthday = 0;
   p->age = 0;
   p->jail_timeout = 0;

   p->script = 0;
   strcpy(p->script_file, "dummy.log");
   p->assisted_by[0] = 0;

   p->residency = 0;

   strcpy(p->ignore_msg, "");
   p->jetlag = 0;

   did_load = load_player(p);

   strcpy(p->assisted_by, "");

   if (title && *title)
   {
      strncpy(p->title, title, MAX_TITLE);
      p->title[MAX_TITLE] = 0;
   }
   if ((p->saved_flags & IAC_GA_ON) && (!(p->flags & EOR_ON)))
      p->flags |= IAC_GA_DO;
   else
      p->flags &= ~IAC_GA_DO;

   if (p->residency == 0 && did_load == 1)
      p->residency = STANDARD_ROOMS;

   if (p->saved_flags & SAVEDFROGGED)
      p->flags |= FROGGED;

   /* got to have someone here I'm afraid..  */

   if (!strcmp("admin", p->lower_name))
   {
      p->residency = HCADMIN_INIT;
   }
   if (p->residency & PSU)
      p->no_shout = 0;




   /* integrity .. sigh */

   p->saved_residency = p->residency;

   if ((p->word_wrap) > ((p->term_width) >> 1) || p->word_wrap < 0)
      p->word_wrap = (p->term_width) >> 1;
   if (p->term > 9)
      p->term = 0;

   return did_load;
}


/* current player stuff */

/* create an abstract player into the void hash list */

player         *create_player()
{
   player         *p;

   p = (player *) MALLOC(sizeof(player));
   memset((char *) p, 0, sizeof(player));

   if (flatlist_start)
      flatlist_start->flat_previous = p;
   p->flat_next = flatlist_start;
   flatlist_start = p;

   p->hash_next = hashlist[0];
   hashlist[0] = p;
   p->hash_top = 0;
   p->timer_fn = 0;
   p->timer_count = -1;
   p->edit_info = 0;
   p->logged_in = 0;
#ifdef IDENT
    p->ident_id = AWAITING_IDENT;
    strcpy(p->userID, IDENT_NOTYET);
#endif
   return p;
}

/* unlink p from all the lists */

void            punlink(player * p)
{
   player         *previous, *scan;

   /* reset the session p */

   p_sess = 0;

   /* first remove from the hash list */

   scan = hashlist[p->hash_top];
   previous = 0;
   while (scan && scan != p)
   {
      previous = scan;
      scan = scan->hash_next;
   }
   if (!scan)
      log("error", "Bad hash list");
   else if (!previous)
      hashlist[p->hash_top] = p->hash_next;
   else
      previous->hash_next = p->hash_next;

   /* then remove from the flat list */

   if (p->flat_previous)
      p->flat_previous->flat_next = p->flat_next;
   else
      flatlist_start = p->flat_next;
   if (p->flat_next)
      p->flat_next->flat_previous = p->flat_previous;

   /* finally remove from the location list */

   if (p->location)
   {
      previous = 0;
      scan = p->location->players_top;
      while (scan && scan != p)
      {
    previous = scan;
    scan = scan->room_next;
      }
      if (!scan)
    log("error", "Bad Location list");
      else if (!previous)
    p->location->players_top = p->room_next;
      else
    previous->room_next = p->room_next;
   }
}

/* remove a player from the current hash lists */

void            destroy_player(player * p)
{
   if ((p->fd) > -1)
   {
#ifdef ANSI_COLS
       if(!(p->flags & PANIC))
         vtell_player(p, "%s\n", NOR);
#endif
      shutdown(p->fd, 0);
      close(p->fd);
   }
   if (p->name[0] && p->location)
      current_players--;
   punlink(p);
   if (p->edit_info)
      finish_edit(p);
   FREE(p);
}

/* get person to choose hitelss type */
void		hitells_type(player *p, char *str)
{
  int count;
  
  if(!*str) {
    tell_player(p, "VT100 assumed.\n");
    p->term = 3;
    p->input_to_fn = 0;
    link_to_program(p);
    return;
  }
  else if(!strcasecmp(str, "off")) {
    tell_player(p, "Termtype turned off.\n");
    p->term = 0;
    p->input_to_fn = 0;
    link_to_program(p);
    return;
  }
    
  for(count=0;terms[count].name[0]!='\0'; count++)
    if(!strcasecmp(str, terms[count].name)) {
      vtell_player(p, "Termtype now set to %s.\n", terms[count].name);
      p->term = count+1;
      p->input_to_fn = 0;
      link_to_program(p);
      return;
    }
  do_prompt(p, "Please select a valid terminal setting [vt100]: ");
  p->input_to_fn = hitells_type;
}


/* get person to agree to disclaimer */

void            agree_disclaimer(player * p, char *str)
{
   p->input_to_fn = 0;
   if (!strcasecmp(str, "continue"))
   {
      p->saved_flags |= AGREED_DISCLAIMER;
      if (p->saved)
         p->saved->saved_flags |= AGREED_DISCLAIMER;
      if (p->residency==NON_RESIDENT) {
        tell_player(p, hitells_msg.where);
        do_prompt(p, "Please select a terminal setting [vt100]: ");
        p->input_to_fn = hitells_type;
        return;
      }
      link_to_program(p);
      return;
   }
   if(!strcasecmp(str, "end")) {
     tell_player(p, "\n Disconnecting from program.\n\n");
     p->flags |= TRIED_QUIT;
     quit(p, "");
     return;
   }
   do_prompt(p, "Enter 'continue' or 'end': ");
   p->input_to_fn = agree_disclaimer;
}

/* links a person into the program properly */

void            link_to_program(player * p)
{
   char           *oldstack;
   saved_player   *sp;
   player         *search, *previous, *scan;
   room           *r, *rm;
   int             hash;
   time_t         t;
   struct tm      *log_time;
   oldstack = stack;


   search = hashlist[((int) (p->lower_name[0])) - (int) 'a' + 1];
   for (; search; search = search->hash_next)
   {
      if (!strcmp(p->lower_name, search->lower_name))
      {
         if (p->residency == NON_RESIDENT)
         {
            tell_player(p, "\n Sorry there is already someone on the "
                           "program with that name.\n Please try again, "
                           "but use a different name.\n\n");
            p->flags |= TRIED_QUIT;
            quit(p, "");
            stack = oldstack;
            return;
         } else
         {
            tell_player(p, "\n You were already on the program !!\n\n"
                           " Closing other connection.\n\n");
            p->total_login = search->total_login;
            if (search->jail_timeout > 0) p->jail_timeout=search->jail_timeout;
	    if (search->saved_flags & SAVEDJAIL) p->saved_flags |= SAVEDJAIL;
            search->flags |= RECONNECTION;
            p->flags |= RECONNECTION;

            if (search->location)
            {
               previous = 0;
               scan = search->location->players_top;
               while (scan && scan != search)
               {
                  previous = scan;
                  scan = scan->room_next;
               }
               if (!scan)
                  log("error", "Bad Location list");
               else if (!previous)
                  search->location->players_top = search->room_next;
               else
                  previous->room_next = search->room_next;
            }
	    
            search->location = 0;
            quit(search, 0);
         }
      }
   }


   /* do the disclaimer biz  */

   if (!(p->saved_flags & AGREED_DISCLAIMER))
   {
  /*    if (!(p->saved && p->password[0] == 0))
      {*/
         tell_player(p, disclaimer_msg.where);
         do_prompt(p, "Enter 'continue' or 'end': ");
         p->input_to_fn = agree_disclaimer;
         stack = oldstack;
         return;
     /* }*/
   }
   /* remove player from non name hash list */

   previous = 0;
   scan = hashlist[0];
   while (scan && scan != p)
   {
      previous = scan;
      scan = scan->hash_next;
   }
   if (!scan)
      log("error", "Bad non-name hash list");
   else if (!previous)
      hashlist[0] = p->hash_next;
   else
      previous->hash_next = p->hash_next;

   /* now place into named hashed lists */

   hash = (int) (p->lower_name[0]) - (int) 'a' + 1;
   p->hash_next = hashlist[hash];
   hashlist[hash] = p;
   p->hash_top = hash;

   if (p->flags & SITE_LOG)
       vlog("site", "%s - %s",p->name,p->inet_addr);

   t = time(0);
   log_time = localtime(&t);

   p->flags |= PROMPT;
   p->timer_fn = 0;
   p->timer_count = -1;

   p->mode = NONE;
   if (p->residency != NON_RESIDENT)
   {
      p->logged_in = 1;
      sp = p->saved;
      tell_player(p, motd_msg.where);
      if (p->saved_flags & CONVERSE)
         p->mode |= CONV;
   } else
   {
      tell_player(p, newbie_msg.where);
      tell_player(p, motd_msg.where);
   }
   current_players++;
   p->on_since = time(0);
   logins++;

   p->shout_index = 50;
   if (p->residency != NON_RESIDENT) {
      p->flags |= FORCE_COLOUR;
      player_flags(p);
      p->flags &= ~FORCE_COLOUR;
   }

   if (p->saved_flags & SAVEDJAIL || (p->jail_timeout > 0))
   {
      if (p->saved_flags & SAVEDJAIL) p->jail_timeout = -1;
      trans_to(p, "system.prison");
   } 
   else if (p->saved_flags & TRANS_TO_HOME || *p->room_connect)
   {
      sp = p->saved;
      if (!sp)
         tell_player(p, " Double Eeek (room_connect)!\n");
      else
      {
         if (p->saved_flags & TRANS_TO_HOME)
         {
            for (r = sp->rooms; r; r = r->next)
               if (r->flags & HOME_ROOM)
               {
                  sprintf(oldstack, "%s.%s", r->owner->lower_name, r->id);
                  stack = end_string(oldstack);
                  trans_to(p, oldstack);
                  stack = oldstack;
                  break;
               }
         } 
         else
         {
            rm = convert_room(p, p->room_connect);
#ifdef ROBOTS
	    if (rm && ((rm->flags & CONFERENCE && possible_move(p, rm, 1)) || p->residency & ROBOT_PRIV))
#else
            if (rm && (rm->flags & CONFERENCE && possible_move(p, rm, 1)))
#endif
               trans_to(p, p->room_connect);
         }
         if (!(p->location))
            tell_player(p, " -=> Tried to connect you to a room, but failed"
                           " !!\n\n");
      }
   }
   if (!(p->location))
      trans_to(p, ENTRANCE_ROOM);

   if (p->flags & RECONNECTION)
   {
      do_inform(p, "[%s reconnects] %s");
      vtell_room(p->location, " %s's image shimmers for a moment as %s re-connects.\n",
              p->name, gstring(p));
      p->flags &= ~RECONNECTION;
   } 
   else
   {
      if (p->gender==PLURAL)
	  do_inform(p, "[%s have connected] %s");	  
      else
	  do_inform(p, "[%s has connected] %s");

      if (p->gender==PLURAL)
	vtell_room(p->location, " %s appear as a wobbly image that soon solidifies.\n",
                p->name);
      else
	vtell_room(p->location, " %s appears as a wobbly image that soon solidifies.\n",
                p->name);
   }
   if (p->saved)
   {
      decompress_list(p->saved);
      p->saved_flags = p->saved->saved_flags;
   }
   stack = oldstack;
}


/* get new gender */
void            enter_gender(player * p, char *str)
{
   switch (tolower(*str))
   {
   case 'm':
     p->gender = MALE;
     tell_player(p, " Gender set to Male.\n");
     break;
   case 'f':
     p->gender = FEMALE;
     tell_player(p, " Gender set to Female.\n");
     break;
   case 'p':
     p->gender = PLURAL;
     strncpy(p->title, "the newbies, so treat us nicely.", MAX_TITLE);
     strncpy(p->description, "Isn't it time we wrote our own descriptions ?",
	     MAX_DESC);
     strncpy(p->plan, "We must write ourselves a proper plan sometime ...",
	     MAX_PLAN);
     strcpy(p->enter_msg, "enter in a standard kind of way.");
     tell_player(p, " Gender set to Plural.\n");
     break;
   case 'n':
     p->gender = OTHER;
     tell_player(p, " Gender set to well, erm, something.\n");
     break;
   default:
     tell_player(p, " No gender set.\n");
     break;
   }
   p->input_to_fn = 0;
   link_to_program(p);
}


/* time out */

void            login_timeout(player * p)
{
   tell_player(p, "\n\n Connection Timed Out ...\n\n");
   quit(p, 0);
}


/* newbie stuff */

void            newbie_get_gender(player * p, char *str)
{
   tell_player(p, "\n\n The program requires that you enter your gender.\n"
     " This is used solely for the purposes of correct english and grammer.\n"
      " If you object to this, then simply type 'n' for not applicable.\n\n");
   do_prompt(p, "Enter (M)ale, (F)emale, (P)lural, or (N)ot applicable: ");
   p->input_to_fn = enter_gender;
}

void            got_new_name(player * p, char *str)
{
   char           *oldstack, *cpy;
   int             length = 0;
   oldstack = stack;

   for (cpy = str; *cpy; cpy++)
      if (isalpha(*cpy))
      {
    *stack++ = *cpy;
    length++;
      }
   *stack++ = 0;
   length++;
   if (length > (MAX_NAME - 2))
   {
      tell_player(p, " Sorry, that name is too long, please enter something "
        "shorter.\n\n");
      do_prompt(p, "Please enter a name: ");
      p->input_to_fn = got_new_name;
      if (sys_flags & VERBOSE)
        vlog("connection", "Name too long : %s\n", str);
      stack = oldstack;
      return;
   }
   if (length < 3)
   {
      tell_player(p, " Thats a bit short, try something longer.\n\n");
      do_prompt(p, "Please enter different name: ");
      p->input_to_fn = got_new_name;
      stack = oldstack;
      return;
   }
   
   /* reserved name checks */
   if(!strcasecmp("everyone", oldstack) || !strcasecmp("newbies", oldstack) ||
#ifdef ROBOTS
      !strcasecmp("robots", oldstack) ||
#endif
      !strcasecmp("sus", oldstack) || !strcasecmp("who", oldstack) ||
      !strcasecmp("friends", oldstack) || !strcasecmp("quit", oldstack))
   {
     tell_player(p, "\n Sorry but that name is reserved.\n"
		    " Choose a different name ...\n\n");
     do_prompt(p, "Please enter different name: ");
     p->input_to_fn = got_new_name;
     stack = oldstack;
     return;
   }
   
   if (restore_player_title(p, oldstack, 0))
   {
      tell_player(p, " Sorry, there is already someone who uses the "
                     "program with that name.\n\n");
      do_prompt(p, "Please enter different name: ");
      p->input_to_fn = got_new_name;
      stack = oldstack;
      return;
   }
   newbie_get_gender(p, str);
   stack = oldstack;
}


void            newbie_got_name_answer(player * p, char *str)
{
   switch (tolower(*str))
   {
    case 'y':
    newbie_get_gender(p, str);
    break;
      case 'n':
    tell_player(p, "\n\n Ok, then, please enter a new name ...\n\n");
    do_prompt(p, "Enter a name: ");
    p->input_to_fn = got_new_name;
    break;
      default:
    tell_player(p, " Please answer with Y or N.\n");
    newbie_check_name(p, str);
    break;
   }
}


void            newbie_check_name(player * p, char *str)
{
   vtell_player(p, "\n\n You entered the name '%s' when you first logged in.\n"
   " Is this the name that you wish to be known as on the program ?\n\n",
      p->name);
   do_prompt(p, "Answer Y or N: ");
   p->input_to_fn = newbie_got_name_answer;
}


void            newbie_start(player * p, char *str)
{
   tell_player(p, newpage2_msg.where);
   do_prompt(p, "Hit return to continue: ");
   p->input_to_fn = newbie_check_name;
}


/* test password */

int             check_password(char *password, char *entered, player * p)
{
   char            key[9];
   strncpy(key, entered, 8);
   return (!strncmp(crypt(key, p->lower_name), password, 11));
}


void            got_password(player * p, char *str)
{
   p->input_to_fn = 0;
   password_mode_off(p);

   if (!check_password(p->password, str, p))
   {
      p->failpasswd++;
      if(p->failpasswd==3) {
        tell_player(p, "\n\n Hey !! that ain't right !\n"
                     " Wrong password .... closing connection.\n\n");
      
        if (p->residency & SU)
           vlog("sufailpass", "Password fail: %s - %s", p->inet_addr, p->name);
        else
           vlog("connection", "Password fail: %s - %s", p->inet_addr, p->name);

        p->flags |= (NO_SAVE_LAST_ON|TRIED_QUIT);
        quit(p, "");
        return;
      }
      tell_player(p, "  Password error, please retype.\n");
      password_mode_on(p);
      do_prompt(p, "Please enter your password: ");
      p->input_to_fn = got_password;
      p->timer_fn = login_timeout;
      p->timer_count = 60;
      return;
   }
   if (p->gender < 0)
   {
      p->input_to_fn = enter_gender;
      tell_player(p, "\n You have no gender set.\n");
      do_prompt(p, "Please choose M(ale), F(female) or N(ot applicable): ");
      return;
   }
   link_to_program(p);
}


/* calls here when the player has entered their name */
void            got_name(player * p, char *str)
{
   int             t;
   char           *oldstack, *cpy, *space;
   int             length = 0, is_space = 0, nologin;
   player         *search;

   oldstack = stack;

   for (cpy = str; *cpy && *cpy != ' '; cpy++)
      if (isalpha(*cpy))
      {
         *stack++ = *cpy;
         length++;
      }
   if (*cpy == ' ')
      is_space = 1;
   *stack++ = 0;
   space = stack;
   length++;

   if (length > (MAX_NAME - 2))
   {
      tell_player(p, " Sorry, that name is too long, please enter something "
                     "shorter.\n\n");
      do_prompt(p, "Please enter a name: ");
      p->input_to_fn = got_name;

      if (sys_flags & VERBOSE)
	vlog("connection", "Name too long : %s\n", str);
      stack = oldstack;
      return;
   }

   if (length < 3)
   {
      tell_player(p, " Thats a bit short, try something longer.\n\n");
      do_prompt(p, "Please enter a name: ");
      p->input_to_fn = got_name;
      stack = oldstack;
      return;
   }

   if (!strcasecmp("who", oldstack))
   {
      swho(p, 0);
      p->input_to_fn = got_name;
      do_prompt(p, "\nPlease enter a name: ");
      stack = oldstack;
      return;
   }

   if (!strcasecmp("quit", oldstack))
   {
      p->flags |= TRIED_QUIT;
      quit(p, "");
      stack = oldstack;
      return;
   }
   
   /* reserved name checks */
   if(!strcasecmp("everyone", oldstack) || !strcasecmp("newbies", oldstack) ||
#ifdef ROBOTS
      !strcasecmp("robots", oldstack) ||
#endif
      !strcasecmp("friends", oldstack) || !strcasecmp("sus", oldstack))
   {
     tell_player(p, "\n Sorry but that name is reserved.\n"
		    " Choose a different name ...\n\n");
     do_prompt(p, "Please enter a name: ");
     p->input_to_fn = got_name;
     stack = oldstack;
     return;
   }
   
   if (is_space)
      while (*++cpy == ' ');
   if (restore_player_title(p, oldstack, is_space ? cpy : 0))
   {
      if (p->residency & BANISHD)
      {
	tell_player(p, banned_msg.where);
	p->flags |= TRIED_QUIT;
	quit(p, "");
	stack = oldstack;
	return;
      }
      if (p->residency == STANDARD_ROOMS)
      {
	tell_player(p, "\n Sorry but that name is reserved.\n"
		    " Choose a different name ...\n\n");
	do_prompt(p, "Please enter a name: ");
	p->input_to_fn = got_name;
	stack = oldstack;
	return;
      }
      t = time(0);
      if (p->sneezed > t)
      {
	nologin = p->sneezed - t;
	stack = oldstack;
	vtell_player(p, "\n Sorry, you have been prevented from logging on for "
		"another %d seconds (and counting ...)\n\n", nologin);
        p->flags |= TRIED_QUIT;
	quit(p, "");
	return;
      } else
	p->sneezed = 0;
/* login limits checks here */
      if (!(p->residency & (LOWER_ADMIN | ADMIN)))
      {
	if (current_players >= max_players)
         {
	   tell_player(p, full_msg.where);
	   p->flags |= TRIED_QUIT;
	   quit(p, "");
         }
      }
      vtell_player(p, "\n Password needed for '%s'\n\n", p->name);
      stack = oldstack;
 
      if (p->password[0]!=0)
      {
	password_mode_on(p);
	do_prompt(p, "Please enter your password: ");
	p->input_to_fn = got_password;
	p->timer_count = 60;
	p->timer_fn = login_timeout;
	stack = oldstack;
	return;
      }
      stack = oldstack;
      link_to_program(p);
      tell_player(p, "\n You have no password !!!\n"
	" Please set one as soon as possible with the 'password' command.\n"
        " If you don't your character will not save\n");
      p->input_to_fn = 0;
      return;
   }
   p->input_to_fn = 0;

   if (p->flags & CLOSED_TO_NEWBIES)
   {
      tell_player(p, newban_msg.where);
      p->flags = TRIED_QUIT;
      quit(p, "");
      stack = oldstack;
      return;
   }
   if (sys_flags & CLOSED_TO_NEWBIES)
   {
      tell_player(p, nonewbies_msg.where);
      p->flags |= TRIED_QUIT;
      quit(p, "");
      stack = oldstack;
      return;
   }
   search = hashlist[((int) (p->lower_name[0])) - (int) 'a' + 1];
   for (; search; search = search->hash_next)
      if (!strcmp(p->lower_name, search->lower_name))
      {
    tell_player(p, "\n Sorry there is already someone on the program "
           "with that name.\nPlease try again, but use a "
           "different name.\n\n");
    p->flags |= TRIED_QUIT;
    quit(p, "");
    stack = oldstack;
    return;
      }
   tell_player(p, newpage1_msg.where);
   do_prompt(p, "Hit return to continue: ");
   p->input_to_fn = newbie_start;
   p->timer_count = 1800;
   stack = oldstack;
}


/* a new player has connected */
void            connect_to_prog(player * p)
{
   player *cp;

   cp = current_player;
   current_player = p;
   tell_player(p, "\377\373\031");   /* send will EOR */
   tell_player(p, connect_msg.where);
   do_prompt(p, "Please enter your name: ");
   p->input_to_fn = got_name;
   p->timer_count = 60000000; /* SPANG */
   p->timer_fn = login_timeout;
   current_player = cp;
}

/* tell player motd */

void            motd(player * p, char *str)
{
   if (!p->residency)
      tell_player(p, newbie_msg.where);
   tell_player(p, motd_msg.where);
}


/* various associated routines */

/* find one player given a (partial) name */

/* little subroutinette */

int             match_player(char *str1, char *str2)
{
   for (; *str2; str1++, str2++)
   {
      if (*str1 != *str2 && *str2 != ' ')
      {
    if (!(*str1) && *str2 == '.' && !(*(str2 + 1)))
       return 1;
    return 0;
      }
   }
   if (*str1)
      return -1;
   return 1;
}


/* command to view res files */

void            view_saved_lists(player * p, char *str)
{
   saved_player   *scan, **hash;
   int             i, j;
   char           *oldstack;
   oldstack = stack;
   if (!*str || !isalpha(*str))
   {
      tell_player(p, " Argument is a letter.\n");
      return;
   }
   strcpy(stack, "[HASH] [NAME]               12345678901234567890123456789012\n");
   stack = strchr(stack, 0);
   hash = saved_hash[((int) (tolower(*str)) - (int) 'a')];
   for (i = 0; i < HASH_SIZE; i++, hash++)
      for (scan = *hash; scan; scan = scan->next)
      {
    sprintf(stack, "[%d]", i);
    j = strlen(stack);
    stack = strchr(stack, 0);
    for (j = 7 - j; j; j--)
       *stack++ = ' ';
    strcpy(stack, scan->lower_name);
    j = strlen(stack);
    stack = strchr(stack, 0);
    for (j = 21 - j; j; j--)
       *stack++ = ' ';
    switch (scan->residency)
    {
       case STANDARD_ROOMS:
          strcpy(stack, "Standard room file.");
          break;
       case BANISHD:
          strcpy(stack, "BANISHED (Name Only)");
          break;
       default:
          if (scan->residency & BANISHD)
             strcpy(stack, "BANISHED");
          else
             strcpy(stack, bit_string(scan->residency));
          break;
    }
    stack = strchr(stack, 0);
    *stack++ = '\n';
      }
   *stack++ = 0;
   pager(p, oldstack, 1);
   stack = oldstack;
}

/* external routine to check updates */

void            check_updates(player * p, char *str)
{
   char           *oldstack;
   int             i;
   oldstack = stack;
   strcpy(stack, "abcdefghijklmnopqrstuvwxyz\n");
   stack = strchr(stack, 0);
   for (i = 0; i < 26; i++)
      if (update[i])
    *stack++ = '*';
      else
    *stack++ = ' ';
   *stack++ = '\n';
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}


void            player_flags(player * p)
{
   char           *oldstack, *str;
   int		newmails=0;
   oldstack = stack;
   str = stack;
   if (p->residency == NON_RESIDENT)
   {
      log("error", "You've sponged Chris. Tried to player_flags a non-resi");
      return;
   }
   if (!p->saved)
      tell_player(p, " Eeeeeeek ! No saved bits !\n");
   else
   {
      vtell_player(p, "\n --\n Last logged in %s from %s.\n",
         convert_time(p->saved->last_on), p->saved->last_host);
      strcpy(stack, " You are ");
      stack = strchr(stack, 0);
      if (p->saved_flags & HIDING)
      {
    strcpy(stack, "in hiding, ");
    stack = strchr(stack, 0);
      }
      if (p->saved_flags & BLOCK_SHOUT)
      {
    strcpy(stack, "blocking shouts, ");
    stack = strchr(stack, 0);
      }
      if (p->saved_flags & BLOCK_TELLS)
      {
    strcpy(stack, "blocking tells, ");
    stack = strchr(stack, 0);
      }
      
      if (p->saved_flags & CONVERSE)
      {
    strcpy(stack, "in converse mode, ");
    stack = strchr(stack, 0);
      }
      if (p->saved_flags & NOPREFIX)
      {
    strcpy(stack, "ignoring prefixes, ");
    stack = strchr(stack, 0);
      }
      if ((str = strrchr(oldstack, ',')))
      {
    *str++ = '.';
    *str++ = '\n';
    *str++ = 0;
    stack = strchr(stack, 0);
    stack++;
    tell_player(p, oldstack);
      }
      stack = oldstack;
   }
   
   newmails = new_mail_check(p);
   if (newmails)
   {
      command_type |= (HIGHLIGHT|PERSONAL|WARNING);
      if(newmails==1)
        tell_player(p, " You have 1 piece of unread mail.\n");
      else
        vtell_player(p, " You have %d pieces of unread mail.\n");
      command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
   }
   if (p->residency & PSU && sys_flags & CLOSED_TO_NEWBIES)
     {
      command_type |= (HIGHLIGHT|PERSONAL|WARNING);
      vtell_player(p, " %s is currently closed to newbies.\n", TALKER_NAME);
      command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
    }       
   tell_player(p, " --\n");
   stack = oldstack;
}


void            player_flags_verbose(player * p, char *str)
{
   char           *oldstack, *wibble, *argh;
   player         *p2;
   oldstack = stack;

   if (*str && (p->residency & SU || p->residency & ADMIN))
   {
      p2 = find_player_absolute_quiet(str);
      if (!p2)
      {
         tell_player(p, " No-one on of that name.\n");
         stack = oldstack;
         return;
      }
   } else
      p2 = p;

   if (p2->residency == NON_RESIDENT)
   {
      tell_player(p, " You aren't a resident, so your character won't be "
                     "saved when you log off.\n");
      stack = oldstack;
      return;
   }
   strcpy(stack, "\n --\n");
   stack = strchr(stack, 0);
   argh = stack;
   strcpy(stack, " You are ");
   stack = strchr(stack, 0);

   if (p2->saved_flags & HIDING)
   {
      strcpy(stack, "in hiding, ");
      stack = strchr(stack, 0);
   }

   if (p2->saved_flags & BLOCK_SHOUT)
   {
      strcpy(stack, "ignoring shouts, ");
      stack = strchr(stack, 0);
   }

   if (p2->saved_flags & BLOCK_TELLS)
   {
      strcpy(stack, "ignoring tells, ");
      stack = strchr(stack, 0);
   }
if (p2->sflags & BLOCK_FRIEND)
      {
    strcpy(stack, "blocking friend tells, ");
    stack = strchr(stack, 0);
      }
      if (p2->sflags & BLOCK_TF_REPLIES)
      {
    strcpy(stack, "blocking friend replies, ");
    stack = strchr(stack, 0);
      }
      if (p2->sflags & LIST_FAILS)
      {
    strcpy(stack, "seeing failed friend tell reports, ");
    stack = strchr(stack, 0);
      }
   if (p2->saved_flags & CONVERSE)
   {
      strcpy(stack, "in converse mode, ");
      stack = strchr(stack, 0);
   }

   if (p2->saved_flags & NOPREFIX)
   {
      strcpy(stack, "ignoring prefixes, ");
      stack = strchr(stack, 0);
   }

   if ((wibble = strrchr(oldstack, ',')))
   {
      *wibble++ = '.';
      *wibble++ = '\n';
      *wibble = 0;
   } else
      stack = argh;

   if (p2->saved_flags & PRIVATE_EMAIL)
      strcpy(stack, " Your email is private.\n");
   else
      strcpy(stack, " Your email is public for all to see.\n");
   stack = strchr(stack, 0);

   if (p2->saved_flags & TRANS_TO_HOME)
   {
      strcpy(stack, " You will be taken to your home when you log in.\n");
      stack = strchr(stack, 0);
   } else if (*p2->room_connect)
   {
      sprintf(stack, " You will try to connect to room '%s' when you log"
         " in\n", p2->room_connect);
      stack = strchr(stack, 0);
   }

   if (p2->saved_flags & NO_ANONYMOUS)
      strcpy(stack, " You won't receive anonymous mail.\n");
   else
      strcpy(stack, " You are currently able to receive anonymous mail.\n");
   stack = strchr(stack, 0);
   
   if (p2->saved_flags & NEWS_INFORM)
      strcpy(stack, " You will get informed of new news postings.\n");
   else
      strcpy(stack, " You will not get informed of new news postings.\n");
   stack = strchr(stack, 0);
   
   if(p2->saved_flags & SNEWS_INFORM && p2->residency & SU) {
     strcpy(stack, " You will get informed of new news postings.\n");
     stack = strchr(stack, 0);
   }
   else if(p2->residency & SU) {
     strcpy(stack, " You will not get informed of new news postings.\n");
     stack = strchr(stack, 0);
   }
   

   if (p2->saved_flags & IAC_GA_ON)
      strcpy(stack, " Iacga prompting is turned on.\n");
   else
      strcpy(stack, " Iacga prompting is turned off.\n");
   stack = strchr(stack, 0);

   if (p2->saved_flags & NO_PAGER)
      strcpy(stack, " You are not recieving paged output.\n");
   else
      strcpy(stack, " You are recieving paged output.\n");
   stack = strchr(stack, 0);

   if (p2->flags & BLOCK_SU && p->residency & PSU)
   {
      strcpy(stack, " You are ignoring sus.\n");
      stack = strchr(stack, 0);
   }

   /* will they be notified when people enter their rooms (ie have they
      room notify set) */
   if (p2->saved_flags & ROOM_ENTER)
       strcpy(stack, " You will be informed when someone enters one of "
	      "your rooms.\n");
   else
       strcpy(stack, " You will not be informed when someone enters "
	      "one of your rooms.\n");
   stack = strchr(stack, 0);

   strcpy(stack, " --\n");
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


/* Catch SEGV's and BUS's on load of players, hopefully... */
void error_on_load(int dummy)
{
  longjmp(jmp_env, 1);
}


/* this bit is simply because write_to_file has an important bit of code
   that both dynamic and normal parts use */
int	write_to_file_common(saved_player *sp)
{
   char *oldstack;
   int length;
   
   oldstack = stack;
   
   stack+=4;
   stack = store_string(stack, sp->lower_name);
   stack = store_int(stack, sp->last_on);
   stack = store_int(stack, sp->saved_flags | COMPRESSED_LIST);
   stack = store_int(stack, sp->residency);
   if((sp->residency!=BANISHD) && (sp->residency != STANDARD_ROOMS)) {
     stack = store_string(stack, sp->last_host);
     stack = store_string(stack, sp->saved_email);
#ifndef DYNAMIC
     stack = store_int(stack, sp->data.length);
     memcpy(stack, sp->data.where, sp->data.length);
     stack+=sp->data.length;
#endif
     construct_room_save(sp);
     construct_list_save(sp);
     construct_mail_save(sp);
   }
#ifdef OSF
    length = (long) stack - (long) oldstack;
#else
    length = (int) stack - (int) oldstack;
#endif
   (void) store_int(oldstack, length);
   return length;
}


void    load_player_common(player *p, file readfrom)
{
   char *r;
#ifdef ANSI_COLS
    int count;
#endif
   
   r = readfrom.where;
   
   /* Make pfile additions/removals at the END of this data! :)
      PLEASE remember to add the appropriate matching lines to:
      construct_save_data.
      PLEASE NOTE!!!  This now uses Oolon's (thanks) get_int_safe
      and get_string_safe functions, which allow you to just add
      data items to the files without having to run file updates */                                 
   
   r = get_string_safe(p->name, r, readfrom);
   r = get_string_safe(p->prompt, r, readfrom);
   r = get_string_safe(p->converse_prompt, r, readfrom);
   r = get_string_safe(p->email, r, readfrom);
   r = get_string_safe(p->password, r, readfrom);
   r = get_string_safe(p->title, r, readfrom);
   r = get_string_safe(p->plan, r, readfrom);
   r = get_string_safe(p->description, r, readfrom);
   r = get_string_safe(p->enter_msg, r, readfrom);
   r = get_string_safe(p->pretitle, r, readfrom);
   r = get_string_safe(p->ignore_msg, r, readfrom);
   r = get_string_safe(p->room_connect, r, readfrom);
   r = get_int_safe(&p->term_width, r, readfrom);
   r = get_int_safe(&p->word_wrap, r, readfrom);
   r = get_int_safe(&p->max_rooms, r, readfrom);
   r = get_int_safe(&p->max_exits, r, readfrom);
   r = get_int_safe(&p->max_autos, r, readfrom);
   r = get_int_safe(&p->max_list, r, readfrom);
   r = get_int_safe(&p->max_mail, r, readfrom);
   r = get_int_safe(&p->gender, r, readfrom);
   r = get_int_safe(&p->no_shout, r, readfrom);
   r = get_int_safe(&p->total_login, r, readfrom);
   r = get_int_safe(&p->term, r, readfrom);
   r = get_int_safe(&p->birthday, r, readfrom);
   r = get_int_safe(&p->age, r, readfrom);
   r = get_int_safe(&p->jetlag, r, readfrom);
   r = get_int_safe(&p->sneezed, r, readfrom);
   r = get_int_safe(&p->sflags, r, readfrom);

#ifdef ANSI_COLS
    for(count=0;count<MAX_COLS;count++)
      r = get_int_safe(&p->colours[count], r, readfrom);
#endif
   
   if (((p->term_width) >> 1) <= (p->word_wrap))
      p->word_wrap = (p->term_width) >> 1;
}



/* thing to dump out all email addresses of users onto a disk file */
int	dump_out_emails_fn(player *p, int type, int verbose)
{
  int i, j, fd=-1, vcount=0, icount=0, scount=0, bcount=0, count=0;
#ifdef ROBOTS
  int rcount=0;
#endif
  saved_player **hash, *scan;
    
  /* type 1 is commas btw */
#if defined( FREEBSD )
  fd = open("files/stuff/emails.file", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open("files/stuff/emails.file", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
  if(fd<0) {
    if(verbose)
      tell_player(p, " Error!  Unable to open file.\n");
    return 0;
  }
  
  /* ok, we know what type of scan they want */
  for(j=0; j<26; j++) {
    hash = saved_hash[j];
    for(i=0; i<HASH_SIZE; i++, hash++) {
      for(scan=*hash; scan; scan=scan->next) {
        switch(scan->residency) {
          case STANDARD_ROOMS:
            scount++;
            break;
          case BANISHD:
            bcount++;
            break;
          default:
#ifdef ROBOTS
	    /* robots */
            if(scan->residency & ROBOT_PRIV) {
              rcount++;
              break;
            }
	    else if(scan->saved_email[0]==' ' && scan->saved_email[1]==0) {
	      vcount++;
	      break;
	    }
#else
	    /* test for validated emails */
	    if(scan->saved_email[0]==' ' && scan->saved_email[1]==0) {
	      vcount++;
	      break;
	    }
#endif
	    /* test for null emails (warn about it) */
	    else if(scan->saved_email[0]==0) {
	      icount++;
	      if(verbose)
	        vtell_player(p, " Player '%s' has a null email address! (wont save)\n", scan->lower_name);
	      break;
	    }
	    else { /* valid, write it out */
	      if(type==1) {
	        if(!count)
	          sprintf(stack, "%s", scan->saved_email);
	        else
	          sprintf(stack, ",%s", scan->saved_email);
	      }
	      else
	        sprintf(stack, "%s\n", scan->saved_email);
	      write(fd, stack, strlen(stack));
	      count++;
	    }
        }        
      }
    }
  }
  if(type==1) /* commas, append newline */
  write(fd, "\n", 1);
  close(fd);
  /* print out stats */
  if(verbose) 
    vtell_player(p, " Stats: %d system room(s) skipped.\n"
    		    "        %d banished name(s) skipped.\n"
  		    "        %d validated address(es) skipped.\n"
  		    "        %d null address(es) skipped and reported.\n", scount, bcount, vcount, icount);
#ifdef ROBOTS
  if(verbose)
    vtell_player(p, "        %d robot(s) skipped.\n", rcount);
#endif
  if(verbose)
    vtell_player(p, " Done:  %d emails dumped to file.\n", count);
  return 1;
}


void	dump_out_emails(player *p, char *str)
{
  int type=0;
  
  if(*str && *str=='c') {
    type=1;
    tell_player(p, " Dumping emails out to file (files/stuff/emails.list) with comma seperation.\n");
  }
  else if(*str && *str=='n') {
    type=2;
    tell_player(p, " Dumping emails out to file (files/stuff/emails.list) with newline seperation.\n");
  }
  else {
    tell_player(p, " Format: dump_emails <c/n>\n         Use c for comma seperation and n for newline.\n");
    return;
  }
  dump_out_emails_fn(p, type, 1);
}
  

/* function to locate an email from a partial string - 3 letters? */
void	find_email(player *p, char *str)
{
  int i, j, count=0, dofull=0;
  saved_player **hash, *scan;
  char *oldstack=stack, *full;
  
  if(!*str) {
    tell_player(p, " Format: find_email <string of 3 chars or more> [full]\n");
    return;
  }
  
  /* check for 'full'
     find space, kill it, step past, check - et viola */
  full = next_space(str);
  if(full && *full) {
    *full++=0;
    if(full && *full)
      dofull=1;
  }
  
  if((int)strlen(str)<3) {
    tell_player(p, " String must be AT LEAST 3 characters long.\n");
    return;
  }
  
  /* make it lower case - easier for scanning */
  lower_case(str);
  
  sprintf(stack, " People whos emails contain '%s':\n ", str);
  stack = strchr(stack, 0);
  
  /* lets go findyfind */
  for(j=0; j<26; j++) {
    hash = saved_hash[j];
    for(i=0; i<HASH_SIZE; i++, hash++) {
      for(scan=*hash; scan; scan=scan->next) {
        switch(scan->residency) {
          case STANDARD_ROOMS:
            break;
          case BANISHD:
            break;
          default:
            if(scan->saved_email[0] && scan->saved_email[0]!=' ') {
              if(strstr(scan->saved_email, str)) {
                count++;
                if(dofull)
                  sprintf(stack, "%-20s[%s]\n ", scan->lower_name, scan->saved_email);
                else
                  sprintf(stack, "%s, ", scan->lower_name);
                stack = strchr(stack, 0);
              }
            }
        }
      }
    }
  }
  if(!count) {
    vtell_player(p, " No emails containing '%s' found.\n", str);
    stack = oldstack;
    return;
  }
  if(dofull)
    sprintf(stack, "%d match%s found.\n", count, numeric_es(count));
  else {
    stack-=2;
    sprintf(stack, "\n %d match%s found.\n", count, numeric_es(count));
  }
  stack=end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


#ifdef DYNAMIC
#include "include/dynamic_plists.c"
#else
#include "include/normal_plists.c"
#endif /* dynamic */


/* robots */
#ifdef ROBOTS
#include "include/robot_plists.c"
#endif
