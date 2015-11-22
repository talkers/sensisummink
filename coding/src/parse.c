/*
 * parse.c
 */

#include "include/config.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#ifndef FREEBSD228
#include <malloc.h>
#endif
#include <stdlib.h>

#ifdef IRIX62
#include <malloc.h>
#endif

#ifdef SUNOS
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"
#include "include/proto.h"
#include "include/clist.h"
#include "include/plugins.h"

/* interns */
struct command *last_com;
char           *stack_check;
int             nsync = 0, synct = 0, sync_counter = 0, note_sync = NOTE_SYNC_TIME;
int     mem_use_log_count = 0;
int             account_wobble = 1;
int             performance_timer = 0;
struct command *help_list = 0;
file            help_file = {0, 0};
int             shutdown_count = -1;
void		execute_command(player *, struct command *, char *);
void            clear_comlist_tags(struct command *);
int             comm_match = 0;



/* what happens if bad stack detected */
void            bad_stack(void)
{
   int             missing;
#ifdef OSF
    missing = (long) stack - (long) stack_check;
#else
    missing = (int) stack - (int) stack_check;
#endif
   if (last_com)
      sprintf(stack_check, "Bad stack in function %s, missing %d bytes",
         last_com->text, missing);
   else
      sprintf(stack_check, "Bad stack somewhere, missing %d bytes", missing);
   stack = end_string(stack_check);
   log("stack", stack_check);
   stack = stack_check;
}

/* function to execute a command */
void              execute_command(player *p, struct command *com, char *str)
{
    last_com = com;
    stack_check = stack;
    p->command_used = com;
    (*com->function) (p, str);
    p->command_used = 0;
    if (stack!=stack_check)
      bad_stack();
    sys_flags &= ~(FAILED_COMMAND | PIPE | ROOM_TAG | FRIEND_TAG | EVERYONE_TAG);
    command_type = 0;
}


/* match commands to the sub and main command lists */
char           *do_match(char *str, struct command *com_entry)
{
  char         *scan;
  
  /* check all of the given string until the first space given */
  for(scan = com_entry->text; *scan && *str && !isspace(*str); scan++, str++)
    if(tolower(*str) != *scan)
      return 0;
  
  /* now there are a few possibilities, if *str exists then it was too long
     if *scan exists, then we have a possible match
     if neither exist, we have a definite total match */
  if(!(com_entry->type & cNO_SPACE) && *str && !isspace(*str))
    return 0;
  /* if we have NEW_PARSER undefined, cripple the below code */
  if(*scan) 
  { /* possible match */
#ifdef NEW_PARSER
    if(com_entry->type & cNO_MATCH)
      return 0; /* some commands we dont want to complete */
    com_entry->type |= cTAGGED;
    comm_match++;
#endif
    return 0;
  }
  
  /* while we are on spaces */
  while(*str && isspace(*str))
    str++;
  return str;
}


/* version of the above without matching */
char           *do_notag_match(char *str, struct command *com_entry)
{
  char         *scan;
  
  /* check all of the given string until the first space */
  for(scan = com_entry->text; *scan && *str && !isspace(*str); scan++, str++)
    if(tolower(*str) != *scan)
      return 0;
  
  /* now there are a few possibilities, if *str exists then it was too long
     if *scan exists, then we have a possible match
     if neither exist, we have a definite total match */
  if((!(com_entry->type & cNO_SPACE) && *str && !isspace(*str)) || *scan)
    return 0;
    
  /* while we are on spaces */
  while(*str && isspace(*str))
    str++;
  return str;
}


/* function to clear TAGs on a comlist */
void               clear_comlist_tags(struct command *comlist)
{
  if(!comm_match)
    return;
  while(comlist->text) {
    comlist->type &= ~cTAGGED;
    comlist++;
  }
  comm_match = 0;
}


void            sub_command(player * p, char *str, struct command * comlist)
{
  struct command *comliststart;
  char           *oldstack, *rol, *space;
  oldstack = stack;

  comliststart = comlist;
  
  for(comliststart=comlist; comlist->text; comlist++)
    if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	((!comlist->andlevel) || ((p->residency) & (comlist->andlevel)))) {
      rol = do_match(str, comlist);
      if (rol) {
	execute_command(p, comlist, rol);
	clear_comlist_tags(comliststart);
        return;
      }
    }

  /* check for taggings */
  if(comm_match) {
    if(comm_match==1) { /* single match */
      for(comlist = comliststart; comlist->text && !(comlist->type & cTAGGED); comlist++);

      for(space=comlist->text,rol=str; *space && *rol && !isspace(*rol); rol++);
      while(*rol && isspace(*rol))
        rol++;
      comlist->type &= ~cTAGGED;
      comm_match = 0;
      execute_command(p, comlist, rol);
    }
    else { /* multis */
      strcpy(oldstack, " Multiple sub command matches: ");
      stack = strchr(stack, 0);
      for(comlist=comliststart; comlist->text; comlist++)
	if(comlist->type & cTAGGED) {
	  comlist->type &= ~cTAGGED;
	  sprintf(stack, "%s ", comlist->text);
	  stack = strchr(stack, 0);
        }
      stack--;
      strcpy(stack, ".\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      comm_match = 0;
    }
    return;
  }
  
  rol = str;
  while (*rol && !isspace(*rol))
    rol++;
  *rol = 0;
#ifdef NEW_PARSER
  vtell_player(p, " Cannot match sub command '%s'.\n", str);
#else
  vtell_player(p, " Cannot find sub command '%s'.\n", str);
#endif
  stack = oldstack;
}


void            match_commands(player * p, char *str)
{
  struct command *comlist, *comliststart;
  char           *rol, *oldstack, *space;
  oldstack = stack;
  
  while (*str && *str == ' ')
    str++;
  space = strchr(str, 0);
  space--;
  while (*space == ' ')
    *space-- = 0;
  if (!*str)
    return;
  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];
  else
    comlist = coms[0];
  
  for(comliststart=comlist; comlist->text; comlist++)
    if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	((!comlist->andlevel) || ((p->residency) & (comlist->andlevel)))) {
      rol = do_match(str, comlist);
      if (rol) {
	execute_command(p, comlist, rol);
	clear_comlist_tags(comliststart);
	return;
      }
    }

  /* check for taggings */
  if(comm_match) {
    if(comm_match==1) { /* single match */
      for(comlist = comliststart; comlist->text && !(comlist->type & cTAGGED); comlist++);

      for(space=comlist->text,rol=str; *space && *rol && !isspace(*rol); rol++);
      while(*rol && isspace(*rol))
        rol++;
      comlist->type &= ~cTAGGED;
      comm_match = 0;
      execute_command(p, comlist, rol);
    }
    else { /* multiple matches - list them */
      strcpy(oldstack, " Multiple command matches: ");
      stack = strchr(stack, 0);
      for(comlist=comliststart; comlist->text; comlist++)
	if(comlist->type & cTAGGED) {
	  comlist->type &= ~cTAGGED;
	  sprintf(stack, "%s ", comlist->text);
	  stack = strchr(stack, 0);
        }
      stack--;
      strcpy(stack, ".\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      comm_match = 0;
    }
    return;
  }
  
  rol = str;
  while (*rol && !isspace(*rol))
    rol++;
  *rol = 0;
#ifdef NEW_PARSER
  vtell_player(p, " Cannot match command '%s'.\n", str);
#else
  vtell_player(p, " Cannot find command '%s'.\n", str);
#endif
  stack = oldstack;
}


/* a thing to draw a divider line. boring */
void divider_line(player *p)
{
   int length;
   
   length = p->term_width;
   if (length<=0) length=79;
   
   memset(stack, '-', length);
   stack+=length;
   *stack++='\n';
   *stack = 0;
}


/* a divider line with a title bit in the bar */
void titled_line(player *p, char *string)
{
    int half;
    int length;
    char str[79];

    length = p->term_width;
    
    /* check for excessive string length */
    if (length <= 0)
	length=79;

    memset(str, 0, 79);
    strncpy(str, string, 79);

    if ((int)strlen(str) > (length-2))
       *(str+length-2)=0; /* make str equal to length, with paranoi -3 */
    /* ie 2 for spaces and 1 for 0 bit */
    /* the -1 is for the space either side */
    half = ((length-(int)strlen(str))/2)-1;
    
    /* write dashes on one side */
    memset(stack, '-', half);
    stack += half;
    *stack++ = ' ';
    
    strcpy(stack, str);
    stack = strchr(stack, 0);
    *stack++ = ' ';

    if ((half+half+(int)strlen(str)+2) > length)
        half--;
    if ((half+half+(int)strlen(str)+2) < length)
        half++;
    /* write dashes on other side */
    memset(stack, '-', half);
    stack += half;
    *stack++ = '\n';
    *stack = 0;
}


/* returns the first char of the input buffer */
char           *first_char(player * p)
{
   char           *scan;
   scan = p->ibuffer;
   while (*scan && isspace(*scan))
      scan++;
   return scan;
}



/* flag changing routines */


/* returns the value of a flag from the flag list */

int             get_flag(flag_list * list, char *str)
{
   for (; list->text; list++)
      if (!strcmp(list->text, str))
    return list->change;
   return 0;
}


/* routine to get the next part of an arg */

char           *next_space(char *str)
{
   while (*str && *str != ' ')
      str++;
   if (*str == ' ')
   {
      while (*str == ' ')
    str++;
      str--;
   }
   return str;
}


/* list commands to the stack by type */
void      list_by_type(player *p, int tcheck)
{
  int s;
  struct command *cscan;
  
  for (s = 0; s < 27; s++) {
    for(cscan = coms[s]; cscan->text; cscan++) {
      if((!cscan->level || (p->residency & cscan->level)) && 
	 (!cscan->andlevel || (p->residency & cscan->andlevel)) &&
	  cscan->type & tcheck) {
	sprintf(stack, "%s ", cscan->text);
	stack = strchr(stack, 0);
      }
    }
  }
  stack-=1;
  *stack++='\n';
}


void           view_commands_format_string(player *p)
{
  char *oldstack;
  
  oldstack = stack;
  strcpy(stack, " Format: commands [a-z|comms|cust|sys|info|list|move|misc");
  stack = strchr(stack, 0);
#ifdef SOCIALS
   strcpy(stack, "|soc");
   stack = strchr(stack, 0);
#endif
  if(p->residency & PSU) {
    strcpy(stack, "|su");
    stack = strchr(stack, 0);
  }
  if(p->residency & LOWER_ADMIN) {
    strcpy(stack, "|adm");
    stack = strchr(stack, 0);
  }
  strcpy(stack, "|all]\n");
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* get a listing of your commands */
void           view_commands(player *p, char *str)
{
  char *oldstack, buffer[80];
  struct command *cscan;
  int l, tcheck = 0, count = 0;
  
  if(!*str) {
    view_commands_format_string(p);
    return;
  }
  
  if(strlen(str)==1) { /* all commands beginning with ... */
    lower_case(str);
    if(!isalpha(*str)) {
      view_commands_format_string(p);
      return;
    }
    l = *str - 'a';
    if(l<0 || l>25) {
      view_commands_format_string(p);
      return;
    }
    l++; /* get into right sort of number for comlist */
    oldstack = stack;
    memset(buffer, 0, 80);
    sprintf(buffer, "Your commands beginning with %c", *str);
    titled_line(p, buffer);
    for(cscan = coms[l]; cscan->text; cscan++) 
      if((!cscan->level || (p->residency & cscan->level)) &&
	 (!cscan->andlevel || (p->residency & cscan->andlevel))) {
        sprintf(stack, "%s ", cscan->text);
        stack = strchr(stack, 0);
        count++;
      }
    if(count) {
      stack--;
      *stack++='\n';
    }
    divider_line(p);
    *stack++=0;
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }   
  else { /* must want a commandtype listing */
    if(!strcasecmp(str, "comms"))
      tcheck |= cCOMMS;
    else if(!strcasecmp(str, "cust"))
      tcheck |= cCUST;
    else if(!strcasecmp(str, "sys"))
      tcheck |= cSYS;
    else if(!strcasecmp(str, "info"))
      tcheck |= cINFO;
    else if(!strcasecmp(str, "list"))
      tcheck |= cLIST;
    else if(!strcasecmp(str, "move"))
      tcheck |= cMOVE;
    else if(!strcasecmp(str, "room"))
      tcheck |= cROOM;
    else if(!strcasecmp(str, "misc"))
      tcheck |= cMISC;
#ifdef SOCIALS
    else if(!strcasecmp(str, "soc"))
      tcheck |= cSOC;
#endif
    else if(p->residency & PSU && !strcasecmp(str, "su"))
      tcheck |= cSU;
    else if(p->residency & LOWER_ADMIN && !strcasecmp(str, "adm"))
      tcheck |= cADM;
    else if(!strcasecmp(str, "all")) {
      tcheck |= (cCOMMS|cCUST|cSYS|cINFO|cLIST|cMOVE|cROOM|cMISC);
#ifdef SOCIALS
       tcheck |= cSOC;
#endif
      if(p->residency & PSU)
        tcheck |= cSU;
      if(p->residency & LOWER_ADMIN)
        tcheck |= cADM;
    }
    else {
      view_commands_format_string(p);
      return;
    }
  }
    
  /* we have the command type to check in 'check', so lets do things with it */
  oldstack = stack;
  titled_line(p, "Your Commands");
  if(tcheck & cCOMMS) {
    strcpy(stack, "Communications: ");
    stack = strchr(stack, 0);
    list_by_type(p, cCOMMS);
  }
  if(tcheck & cCUST) {
    strcpy(stack, "Customisation: ");
    stack = strchr(stack, 0);
    list_by_type(p, cCUST);
  }
  if(tcheck & cSYS) {
    strcpy(stack, "Sys Settings: ");
    stack = strchr(stack, 0);
    list_by_type(p, cSYS);
  }
  if(tcheck & cINFO) {
    strcpy(stack, "Information: ");
    stack = strchr(stack, 0);
    list_by_type(p, cINFO);
  }
  if(tcheck & cMOVE) {
    strcpy(stack, "Movement: ");
    stack = strchr(stack, 0);
    list_by_type(p, cMOVE);
  }
  if(tcheck & cLIST) {
    strcpy(stack, "Lists: ");
    stack = strchr(stack, 0);
    list_by_type(p, cLIST);
  }
  if(tcheck & cROOM) {
    strcpy(stack, "Rooms: ");
    stack = strchr(stack, 0);
    list_by_type(p, cROOM);
  }
  if(tcheck & cMISC) {
    strcpy(stack, "Miscellaneous: ");
    stack = strchr(stack, 0);
    list_by_type(p, cMISC);
  }
#ifdef SOCIALS
   if(tcheck & cSOC) {
     strcpy(stack, "Socials: ");
     stack = strchr(stack, 0);
     list_by_type(p, cSOC);
   }
#endif
  if(tcheck & cSU) {
    strcpy(stack, "Superuser: ");
    stack = strchr(stack, 0);
    list_by_type(p, cSU);
  }
  if(tcheck & cADM) {
    strcpy(stack, "Admin: ");
    stack = strchr(stack, 0);
    list_by_type(p, cADM);
  }
  divider_line(p);
  *stack++=0;
  tell_player(p, oldstack);
  stack = oldstack; 
}


void            view_sub_commands(player * p, struct command * comlist)
{
   char           *oldstack;
   oldstack = stack;

   titled_line(p, "Your sub commands");
   
   /* this bit will make it look prettier :) */
   if(comlist==check_list)
     strcpy(stack, "Check: ");
   else if(comlist==editor_list)
     strcpy(stack, "Editor: ");
   else if(comlist==keyroom_list)
     strcpy(stack, "Room(key): ");
   else if(comlist==mail_list)
     strcpy(stack, "Mail: ");
   else if(comlist==news_list)
     strcpy(stack, "News: ");
   else if(comlist==editor_list)
     strcpy(stack, "Room: ");
#ifdef INTERCOM
   else if(comlist==intercom_list)
     strcpy(stack, "Intercom: ");
#endif
   else
     strcpy(stack, "Unknown: ");
   
   stack = strchr(stack, 0);

   for (; comlist->text; comlist++)
      if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	  ((!comlist->andlevel) || ((p->residency) & (comlist->andlevel))))
      {
    sprintf(stack, "%s, ", comlist->text);
    stack = strchr(stack, 0);
      }
   stack -= 2;
   *stack++ = '\n';
   divider_line(p);
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}

/* initialise the hash array */

void            init_parser(void)
{
   int             i;
   struct command *scan;
   
   log("boot", "Initialising parser...");
   
   scan = complete_list;
   for (i = 0; i < 27; i++)
   {
      coms[i] = scan;
      while (scan->text) 
         scan++;
      scan++;
      clear_comlist_tags(coms[i]);
   }
}



/* handle input from one player */

void            input_for_one(player * p)
{
   char           *pick;

#ifdef ANTISPAM
#ifdef ROBOTS
    if(!(p->flags & ROBOT) && p->ibuffer[0]) {
#else
    if(p->ibuffer[0]) {
#endif
      if(!strcmp(p->ibuffer_copy, p->ibuffer)) {
        p->matched_ibuffer_count++;
        if(p->matched_ibuffer_count==MAX_ANTISPAM) {
          p->flags |= (SPAMMER | CHUCKOUT);
          if(p->residency==NON_RESIDENT) {
            sys_flags |= CLOSED_TO_NEWBIES;
            newbie_reopen_count = 300;
          }
          else if(!(p->residency & ADMIN))
            p->sneezed = time(0)+300;
        }
      }
      else {
        strncpy(p->ibuffer_copy, p->ibuffer, IBUFFER_LENGTH); 
        p->matched_ibuffer_count = 0;
      }
    }
#endif
   
   if (p->input_to_fn && !(current_room == prison && !(p->residency & (ADMIN | SU))))
   {
      p->idle = 0;
      p->idle_msg[0] = 0;
      last_com = &input_to;
      stack_check = stack;
	(*p->input_to_fn) (p, p->ibuffer);
      if (stack != stack_check)
         bad_stack();
      sys_flags &= ~(FAILED_COMMAND | PIPE | ROOM_TAG | FRIEND_TAG
                     | EVERYONE_TAG);
      command_type = 0;
      return;
   }
   if (!p->ibuffer[0])
      return;
   p->idle = 0;
   p->idle_msg[0] = 0;
   action = "doing command";
   if (p->ibuffer[0] != '#')
   {
      if (p->saved_flags & CONVERSE)
      {
         pick = p->ibuffer;
         while (*pick && isspace(*pick))
            pick++;
         if (*pick)
            if (*pick == '/')
               if (current_room == prison &&
                   !(p->residency & (ADMIN | SU)))
                  sub_command(p, pick + 1, restricted_list);
               else
                  match_commands(p, pick + 1);
            else
               say(p, pick);
      } else if (current_room == prison && !(p->residency & (ADMIN | SU)))
         sub_command(p, p->ibuffer, restricted_list);
      else
         match_commands(p, p->ibuffer);
   }
}


/* scan through the players and see if anything needs doing */

void            process_players(void)
{
   player         *scan, *scan2;
   char           *oldstack;

   for (scan = flatlist_start; scan; scan = scan2)
   {
     if (scan->flat_next)
       if (((player *)scan->flat_next)->flat_previous != scan)
	 {
	   raw_wall("\n\n   -=> Non-terminated flatlist <=-\n\n");
	   raw_wall("\n\n   -=> Dumping end off of list <=-\n\n");
	   scan->flat_next=NULL;
	 }
	 scan2=scan->flat_next; /* ? */
	 
#ifdef ROBOTS
      if ((scan->fd < 0 && !(scan->flags & ROBOT)) || (scan->flags & PANIC) || (scan->flags & CHUCKOUT))
#else
      if ((scan->fd < 0) || (scan->flags & PANIC) || (scan->flags & CHUCKOUT))
#endif
      {

         oldstack = stack;
         current_player = scan;

         if (scan->location && scan->name[0] && !(scan->flags & RECONNECTION))
         {
            vtell_room(scan->location, " %s suddenly dissolve%s into a squintillion dots"
                           " that quickly disperse.\n",
                    scan->name, single_s(scan));
            save_player(scan);
         }
         if (!(scan->flags & RECONNECTION))
         {
            command_type = 0;
	    if (scan->gender==PLURAL)
	      do_inform(scan, "[%s have disconnected] %s");
	    else
	      do_inform(scan, "[%s has disconnected] %s");
            if (scan->saved && !(scan->flags & NO_SAVE_LAST_ON))
               scan->saved->last_on = time(0);
         }
#ifdef ANTISPAM
         if(scan->flags & SPAMMER) {
#ifdef IDENT
            vlog("spammers", "%s - %s@%s (%s)", scan->name, scan->userID, scan->inet_addr, scan->ibuffer_copy);
#else
            vlog("spammers", "%s - %s (%s)", scan->name, scan->inet_addr, scan->ibuffer_copy);
#endif
           command_type |= (HIGHLIGHT|PERSONAL|WARNING);
           tell_player(scan, "\n\n");
           tell_player(scan, "-=> You have been disconnected as a SPAMMER.  Please don't do this because\n");
           tell_player(scan, "-=> it tends to piss off the Admin lots, who now have a log of your site.\n\n");
           command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
           su_wall("-=> Program closed to newbies due to spammer.\n");
         }
#endif
         destroy_player(scan);
         current_player = 0;
         stack = oldstack;
      }
      else if (scan->flags & INPUT_READY)
      {
/* there used to be this here...
            if (!(scan->lagged) && !(scan->flags & PERM_LAG))
   for reference... */

         if (!(scan->lagged))
         {
            current_player = scan;
            current_room = scan->location;
            input_for_one(scan);
            current_player = 0;
            current_room = 0;

            if (scan->flags & PROMPT)
            {
               if (scan->saved_flags & CONVERSE)
                  do_prompt(scan, scan->converse_prompt);
               else
                  do_prompt(scan, scan->prompt);
            }
         }
         memset(scan->ibuffer, 0, IBUFFER_LENGTH);
         scan->flags &= ~INPUT_READY;
      }
   }
}




/* timer things */


/* automessages */

void            do_automessage(room * r)
{
   int             count = 0, type;
   char           *scan, *oldstack;
   oldstack = stack;
   scan = r->automessage.where;
   if (!scan)
   {
      r->flags &= ~AUTO_MESSAGE;
      return;
   }
   for (; *scan; scan++)
      if (*scan == '\n')
    count++;
   if (!count)
   {
      FREE(r->automessage.where);
      r->automessage.where = 0;
      r->automessage.length = 0;
      r->flags &= ~AUTO_MESSAGE;
      stack = oldstack;
      return;
   }
   count = rand() % count;
   for (scan = r->automessage.where; count; count--, scan++)
      while (*scan != '\n')
    scan++;
   while (*scan != '\n')
      *stack++ = *scan++;
   *stack++ = '\n';
   *stack++ = 0;
   type = command_type;
   command_type = AUTO;
   tell_room(r, oldstack);
   command_type = type;
   r->auto_count = r->auto_base + (rand() & 63);
   stack = oldstack;
}


/* file syncing */

void            do_sync(void)
{
   int             origin;
   action = "doing sync";
   sync_counter = SYNC_TIME;
   origin = synct;
   while (!update[synct])
   {
      synct = (synct + 1) % 26;
      if (synct == origin)
    break;
   }
   if (update[synct])
   {
      sync_to_file(synct + 'a', 0);
#ifdef DYNAMIC
      sync_cache_letter(synct + 'a');
#endif
      synct = (synct + 1) % 26;
   }
}

/* this is the actual timer pling */

void            actual_timer(int sponge)
{
   static int      pling = TIMER_CLICK;
   player         *scan;
   time_t t;

   if (sys_flags & PANIC)
      return;

#ifndef USE_SIGACTION
    if ((int) signal(SIGALRM, actual_timer) < 0)
      handle_error("Can't set timer signal.");
#endif /* use_sigaction */

   t = time(0);
   if ((splat_timeout - t) <= 0)
      splat1 = splat2 = splat3 = splat4 = 255;
   pling--;
   if (pling)
      return;

   pling = TIMER_CLICK;

   sys_flags |= DO_TIMER;

   if (mem_use_log_count > 0)
      mem_use_log_count--;
   if (shutdown_count > 0)
      shutdown_count--;
#ifdef ANTISPAM
    if(newbie_reopen_count>0)
      newbie_reopen_count--;
#endif

#ifdef ROBOTS
   process_robot_counters();
#endif

   for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (!(scan->flags & PANIC))
      {
    	scan->idle++;
    	scan->total_login++;
    	if (scan->script && scan->script > 1)
       	  scan->script--;
    	if (scan->timer_fn && scan->timer_count > 0)
       	  scan->timer_count--;
    	if (scan->no_shout > 0)
       	  scan->no_shout--;
    	if (scan->no_move > 0)
          scan->no_move--;
    	if (scan->lagged > 0)
       	  scan->lagged--;
    	if (scan->shout_index > 0)
       	  scan->shout_index--;
    	if (scan->jail_timeout > 0)
	  scan->jail_timeout--;
      }
   net_count--;
   if (!net_count)
   {
      net_count = 10;
      in_total += in_current;
      out_total += out_current;
      in_pack_total += in_pack_current;
      out_pack_total += out_pack_current;
      in_bps = in_current / 10;
      out_bps = out_current / 10;
      in_pps = in_pack_current / 10;
      out_pps = out_pack_current / 10;
      in_average = (in_average + in_bps) >> 1;
      out_average = (out_average + out_bps) >> 1;
      in_pack_average = (in_pack_average + in_pps) >> 1;
      out_pack_average = (out_pack_average + out_pps) >> 1;
      in_current = 0;
      out_current = 0;
      in_pack_current = 0;
      out_pack_current = 0;
   }
}


/* the timer function */

void            timer_function(void)
{
   player         *scan, *old_current;
   room           *r, **list;
   char           *oldstack, *text;
   int             count = 0, pcount = 0;
   char           *action_cpy;
   struct tm      *ts;
   time_t          t;

#ifndef BROKEN_MSTATS /* linux with broken mstats */
#ifdef HAS_MALLINFO
   struct mallinfo minfo;
#else /* mstats? */
   struct mstats memstats;
#endif /* has_mallinfo */
#endif /* broken_mstats */

   if (!(sys_flags & DO_TIMER))
      return;
   sys_flags &= ~DO_TIMER;

   waitpid((pid_t) - 1, (int *) 0, WNOHANG);
   /* wait3(0,WNOHANG,0); */

   old_current = current_player;
   action_cpy = action;

   oldstack = stack;

#ifndef BROKEN_MSTATS
   if (mem_use_log_count == 0)
   {
#ifdef HAS_MALLINFO
      minfo = mallinfo();
      vlog("mem", "Total arena space - %d", minfo.arena);
#else /* has_mallinfo */
      memstats = mstats();
      vlog("mem", "Total heap size - %d", memstats.bytes_total);
#endif /* has_mallinfo */
      mem_use_log_count = 60;
   }
#endif /* broken_mstats */
   
   if (shutdown_count > -1)
   {
      command_type |= (HIGHLIGHT|PERSONAL|WARNING);
      switch (shutdown_count)
      {
      case 31536000:
	raw_wall("\n\n-=> We'll be shutting down this time next year <=-\n"
		 "-=> Anyone who is still on at this time wins a "
		 "prize <=-\n\n");
	break;
      case 86400:
	raw_wall("\n\n-=> We'll be shutting down this time tomorrow - "
		 "Helpful arent we! <=-\n\n");
	break;
      case 3600:
	 raw_wall("\n\n-=> We'll be rebooting in 1 hour, "
		  "pity the admin that sets it to such a "
		  "stupid time. <=-\n\n");
	break;
      case 900:
	raw_wall("\n\n-=> We'll be rebooting in 15 mins, "
		 "You can 5 eggs in that time! <=-\n\n");
	break;
      case 300:
	raw_wall("\n\n-=> We'll be rebooting in 5 mins, "
		 "go put the kettle on now <=-\n\n");
	break;
      case 180:
	raw_wall("\n\n-=> 3 minutes to go until the reboot, "
		 "time to make the tea <=-\n\n");
	break;
      case 60:
	raw_wall("\n\n-=> 1 minute left to the reboot, "
		 "time to say bye to people <=-\n\n");
	break;
      case 30:
	raw_wall("\n\n-=> 30 seconds until the reboot, "
		 "hold on to your drinks <=-\n\n");
	break;
      case 15:
	raw_wall("\n\n-=> 15 seconds to go to reboot, "
		 "see you in a little while! <=-\n\n");
	break;
      case 10:
	raw_wall("\n\n-=> 10 seconds to go to reboot, "
		 "The end is nigh! <=-\n\n");
	break;
      case 5:
	raw_wall("\n\n-=> 5 seconds to go to reboot, "
		 "The end is nigh(er)! <=-\n\n");
	break;
      case 1:
	raw_wall("\n\n-=> 1 second to go to reboot, "
		 "The end is REALLY nigh! <=-\n\n");
	break;
      case 0:
	log("shutdown", shutdown_reason);
	sys_flags |= SHUTDOWN;
	stack = oldstack;
	return;
      }
      command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
    }

   if (sync_counter)
      sync_counter--;
   else
      do_sync();

   if (note_sync)
      note_sync--;
   else
   {
      note_sync = NOTE_SYNC_TIME;
      sync_notes(1);
   }

#ifdef ANTISPAM
    if(newbie_reopen_count==0) {
      if(sys_flags & CLOSED_TO_NEWBIES) {
        sys_flags &= ~CLOSED_TO_NEWBIES;
        su_wall("-=> Program reopened to newbies after temporary close due to spammer.\n");
      }
      newbie_reopen_count=-1;
    }
#endif
   
   align(stack);
   list = (room **) ALFIX stack; 

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (!(scan->flags & PANIC))
      {
         if (scan->script && scan->script == 1)
         {
            text = stack;
            sprintf(text, " Time is now %s.\n"
                          " Scripting stopped ...\n", convert_time(time(0)));
            stack = end_string(text);
            tell_player(scan, text);
            stack = text;
            scan->script = 0;
         }
         if (scan->timer_fn && !scan->timer_count)
         {
            current_player = scan;
            (*scan->timer_fn) (scan);
            scan->timer_fn = 0;
            scan->timer_count = -1;
         }
         current_player = old_current;
         action = "processing autos";
         r = scan->location;
         if (r)
         {
            pcount++;
            if (r->flags & AUTO_MESSAGE && !(r->flags & AUTOS_TAG))
            {
               if (!r->auto_count)
                  do_automessage(r);
               else
                  r->auto_count--;
               *(room **) ALFIX stack = r;
               stack += sizeof(room *);
               count++;
               r->flags |= AUTOS_TAG;
            }
         }
/* Jail timeout thang */
         if (scan->jail_timeout == 0 && scan->location == prison)
         {
            command_type |= (HIGHLIGHT|PERSONAL|WARNING);
            tell_player(scan, " After serving your sentence you are flung out"
                              " to society again.\n");
            command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
            move_to(scan, ENTRANCE_ROOM, 0);
         }
      }
   }
   for (; count; count--, list++)
      (*list)->flags &= ~AUTOS_TAG;
   stack = oldstack;
   action = action_cpy;
   current_players = pcount;

   t = time(0);
   ts = localtime(&t);

   /*
    * if (!account_wobble && (ts->tm_hour)==0) { account_wobble=1;
    * do_birthdays(); }
    */
   if (account_wobble == 1 && ((ts->tm_hour) == 3))
   {
      account_wobble = 2;
      /* system("bin/account &"); */
   }
   if (account_wobble == 2 && ((ts->tm_hour) > 3))
      account_wobble = 1;
}

/* the help system (aargh argh argh) */


/* look through all possible places to find a bit of help */

struct command *find_help(char *str)
{
  struct command *comlist;
  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];
  else
    comlist = coms[0];
  
  for (; comlist->text; comlist++)
    if (do_notag_match(str, comlist))
      return comlist;
  comlist = help_list;
  if (!comlist)
    return 0;
  for (; comlist->text; comlist++)
    if (do_notag_match(str, comlist))
      return comlist;
  
  return 0;
}


void            next_line(file * hf)
{
   while (hf->length > 0 && *(hf->where) != '\n')
   {
      hf->where++;
      hf->length--;
   }
   if (hf->length > 0)
   {
      hf->where++;
      hf->length--;
   }
}

void            init_help(void)
{
   file            hf;
   struct command *found, *hstart;
   char           *oldstack, *start, *scan;
   int             length;
   oldstack = stack;


   log("boot", "Initialising help system...");

   if (help_list)
      FREE(help_list);
   help_list = 0;

   if (help_file.where)
      FREE(help_file.where);
   help_file = load_file("files/doc/help");
   hf = help_file;

   align(stack);
   hstart = (struct command *) ALFIX stack; 

   while (hf.length > 0)
   {
      while (hf.length > 0 && *(hf.where) != ':')
    next_line(&hf);
      if (hf.length > 0)
      {
    scan = hf.where;
    next_line(&hf);
    *scan++ = 0;
    while (scan != hf.where)
    {
       start = scan;
       while (*scan != ',' && *scan != '\n')
          scan++;
       *scan++ = 0;
       found = find_help(start);
       if (!found)
       {
          found = (struct command *) ALFIX stack; 
          stack += sizeof(struct command);
          found->text = start;
          found->function = 0;
          found->level = 0;
          found->andlevel = 0;
          found->type = 0;
       }
       found->help = hf.where;
    }
      }
   }
   *(hf.where - 1) = 0;
   found = (struct command *) ALFIX stack; 
   stack += sizeof(struct command);
   found->text = 0;
   found->function = 0;
   found->level = 0;
   found->andlevel = 0;
   found->help = 0;
   found->type = 0;
#ifdef OSF
    length = (long) stack - (long) hstart;
#else
    length = (int) stack - (int) hstart;
#endif
   help_list = (struct command *) MALLOC(length);
   memcpy(help_list, hstart, length);
   stack = oldstack;
}


/* load that help file in  */

int             get_help(player * p, char *str)
{
   int             fail = 0;
   file            text;
   char           *oldstack;

   oldstack = stack;
   if (*str == '.')
      return 0;

   sprintf(stack, "files/doc/%s.help", str);
   stack = end_string(stack);
   text = load_file_verbose(oldstack, 0);
   if (text.where)
   {
      if (*(text.where))
      {
    stack = oldstack;
    sprintf(stack, "-- Help ----------------------------------------"
       "---------------------\n%s-----------------------------"
       "----------------------------------------\n", text.where);
    stack = end_string(stack);
    pager(p, oldstack, 1);
    fail = 1;
      } else
    fail = 0;
      free(text.where);
   }
   stack = oldstack;
   return fail;
}





/* the help command */

void help(player * p, char *str)
{
   char *oldstack, *output;
   char buffer[50];
   struct command *fn;
   oldstack = stack;

   memset(buffer, 0, 50);
   
   if (!*str)
   {
      if (p->residency)
         str = "general";
      else
         str = "newbie";
   }
   strncpy(buffer, str, 49);
   fn = find_help(buffer);
   if (!fn || !(fn->help))
   {
      if (get_help(p, buffer))
         return;
      vtell_player(p, " Cannot find any help on the subject of '%s'\n", buffer);
      return;
   }
   
   sprintf(oldstack, "Help: %s", buffer);
   stack = end_string(oldstack);
   output = stack;
   titled_line(p, oldstack);
   strcpy(stack, fn->help);
   stack = strchr(stack, 0);
   divider_line(p);
   *stack++=0;
   if (p->saved_flags & NO_PAGER)
      tell_player(p, output);
   else
      pager(p, output, 0);
   stack = oldstack;
}


/* ok - time for a repeat command in v2.3 */
void		repeat(player *p, char *str)
{
   char *oldstack;
   
   if(!*str) { /* user wants to check what they will remote */
      tell_player(p, " Format: repeat <user_list>\n");
      if(p->repeat_string[0]!=0)
         vtell_player(p, " Current repeatable message: %s\n", p->repeat_string);
      return;
   }      
   /* check there is something to repeat */
   if(p->repeat_string[0]==0) {
      tell_player(p, " You have no repeatable message.\n");
      return;
   }
   /* else things are fine, except for paranoia.. */
   if(!p->repeat_command) {
      tell_player(p, " Wibble!  You have no repeatable command.\n");
      log("error", "Repeatable message but no repeatable command");
      return;
   }
   /* do it */
   oldstack = stack;
   sprintf(stack, "%s %s", str, p->repeat_string);
   stack = end_string(stack);
   (p->repeat_command)(p, oldstack);
   stack = oldstack;
}


/* Do NOT remove this, or you will be violating the terms of the license
 *
 * This is just so, I, Athanasius, know which version you ftp'd for use in
 * your talker.
 */

void summink_version(player *p, char *str)
{
   char *oldstack;
   
   oldstack = stack;
   sprintf(stack, " This talker is based on SensiSummink V%s, a stable, bugfixed, and improved\n"
	         " version of Summink V1.0, with clean compiling.\n", VERSION);
   stack = strchr(stack, 0);
   strcpy(stack, " Summink was made by Athanasius, and was itself based on EW-too by Simon\n"
                 " Marsh (aka Burble).\n");
   stack = strchr(stack, 0);
   
#ifdef LINUX

#ifdef GLIBC
     strcpy(stack, " -=> Running on Glibc Linux.\n");
#else
     strcpy(stack, " -=> Running on Linux.\n");
#endif /* glibc */

#elif SOLARIS

#ifdef SOLARIS25
     strcpy(stack, " -=> Running on Solaris 2.5.\n");
#elif SOLARIS24
     strcpy(stack, " -=> Running on Solaris 2.4.\n");
#else
     strcpy(stack, " -=> Running on Solaris (unknown).\n");
#endif /* solaris25 */

#elif SUNOS

#ifdef SUNOS431
     strcpy(stack, " -=> Running on SunOS 4.3.1.\n");
#else
     strcpy(stack, " -=> Running on SunOS (unknown).\n");
#endif /* sunos431 */

#elif OSF

#ifdef OSF40
     strcpy(stack, " -=> Running on OSF1/Digital Unix 4.0x.\n");
#else
     strcpy(stack, " -=> Running on OSF1/Digital Unix (unknown).\n");
#endif /* osf40 */

#elif IRIX

#ifdef IRIX63
     strcpy(stack, " -=> Running on Irix 6.3.\n");
#elif IRIX62
     strcpy(stack, " -=> Running on Irix 6.2.\n");
#elif IRIX52
     strcpy(stack, " -=> Running on Irix 5.2.\n");
#else
     strcpy(stack, " -=> Running on Irix (unknown).\n");
#endif

#elif FREEBSD

#ifdef FREEBSD221
     strcpy(stack, " -=> Running on FreeBSD 2.2.1.\n");
#else
     strcpy(stack, " -=> Running on FreeBSD (unknown).\n");
#endif /* freebsd221 */

#else
     strcpy(stack, " -=> Unknown operating system!  Fear wildly!.\n");
#endif
   stack = strchr(stack, 0);
   
#ifdef BROKEN_MSTATS
     strcpy(stack, " -=> Broken Mstats bugfix enabled.\n");
     stack = strchr(stack, 0);
#endif /* broken_mstats */
#ifdef IRIX_BUG
     strcpy(stack, " -=> Broken nonecho on Irix bugfix enabled.\n");
     stack = strchr(stack, 0);
#endif
#ifdef NEW_PARSER
     strcpy(stack, " -=> SensiSummink advanced parser enabled.\n");
     stack = strchr(stack, 0);
#endif /* new parser */
#ifdef ANTISPAM
     strcpy(stack, " -=> SensiSummink antispam code enabled.\n");
     stack = strchr(stack, 0);
#endif
#ifdef ANTIPIPE
     strcpy(stack, " -=> SensiSummink antipipe code enabled.\n");
     stack = strchr(stack, 0);
#endif
#ifdef DYNAMIC
     dynamic_version();
#endif
#ifdef ANSI_COLS
     ansi_cols_version();
#endif
#ifdef SOCIALS
     socials_version();
#endif
#ifdef ROBOTS
     robot_version();
#endif
#ifdef CRASH_RECOVER
     crashrec_version();   
#endif
#ifdef IDENT
     ident_version();
#endif
#ifdef INTERCOM
     ver_intercom_version();
#endif
     
   *stack++=0;
   tell_player(p, oldstack);
   stack = oldstack;
}


