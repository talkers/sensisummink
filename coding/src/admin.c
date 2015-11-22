/* 
 * admin.c
 */

#include "include/config.h"

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#ifndef FREEBSD228
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <dirent.h>

#if defined( SUNOS )
 #define TIME_DEFINES
#endif

#ifdef IRIX62
 #include <sys/types.h>
 #include <malloc.h>
#endif

#include "include/missing_headers.h"
#include "include/proto.h"

/* interns */

flag_list       permission_list[] = {
   {"residency", BASE | BUILD | LIST | ECHO_PRIV | MAIL | SESSION},
   {"nosync", NO_SYNC},
   {"base", BASE},
   {"echo", ECHO_PRIV},
   {"no_timeout", NO_TIMEOUT},
   {"banished", BANISHD},
   {"mail", MAIL},
   {"list", LIST},
   {"build", BUILD},
#ifdef ROBOTS
   {"robot", ROBOT_PRIV},
#endif
   {"session", SESSION},
   {"su_channel", PSU},
   {"warn", WARN},
   {"script", SCRIPT},
   {"trace", TRACE},
   {"hcadmin", NO_TIMEOUT | PSU | WARN | TRACE | SCRIPT | SU | LOWER_ADMIN | ADMIN | HCADMIN},
   {"lower_admin", PSU | WARN | TRACE | SU | LOWER_ADMIN},
   {"su", PSU | WARN | TRACE | SU},
   {"admin", NO_TIMEOUT | PSU | WARN | TRACE | SCRIPT | SU | LOWER_ADMIN | ADMIN},
{0, 0}};

int count_su(void);
int count_newbies(void);

/* function to evaluate p against p2 in terms of privs! */
int	check_privs(int p1res, int p2res)
{
  /* hmm */
  if(p2res==STANDARD_ROOMS)
    return 0;
    
  /* if they are an hc admin, then do it regardless */
  if(p1res & HCADMIN)
    return 1;
    
#ifdef ROBOTS
  /* if its a robot and they arent admin, say no now.. */
  if(p2res & ROBOT_PRIV && !(p1res & ADMIN))
    return 0;
#endif
    
  /* check the priv levels */
  if(p2res>=p1res) /* if they are greater or equal, refuse */
    return 0;
    
  return 1; /* sorted, right mate, etc */
}


#ifndef BROKEN_MSTATS
/* malloc data */
#ifdef HAS_MALLINFO
void show_malloc(player * p, char *str)
{
   char *oldstack;
   struct mallinfo i;

   oldstack = stack;
   i = mallinfo();

   sprintf(stack, "Total arena space\t%d\n"
                  "Ordinary blocks\t\t%d\n"
                  "Small blocks\t\t%d\n"
                  "Holding blocks\t\t%d\n"
                  "Space in headers\t\t%d\n"
                  "Small block use\t\t%d\n"
                  "Small blocks free\t%d\n"
                  "Ordinary block use\t%d\n"
                  "Ordinary block free\t%d\n"
#if defined ( SOLARIS ) || defined ( OSF ) || defined ( IRIX )
                  "Keep cost\t\t%d\n",
#else
                  "Keep cost\t\t%d\n"
                  "Small block size\t\t%d\n"
                  "Small blocks in holding\t%d\n"
                  "Rounding factor\t\t%d\n"
                  "Ordinary block space\t%d\n"
                  "Ordinary blocks alloc\t%d\n"
                  "Tree overhead\t%d\n",
#endif
                  i.arena, i.ordblks, i.smblks, i.hblks, i.hblkhd, i.usmblks,
#if defined ( SOLARIS ) || defined ( OSF ) || defined ( IRIX )
                  i.fsmblks, i.uordblks, i.fordblks, i.keepcost);
#else
                  i.fsmblks, i.uordblks, i.fordblks, i.keepcost,
                  i.mxfast, i.nlblks, i.grain, i.uordbytes, i.allocated,
                  i.treeoverhead);
#endif
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

#else /* has_mallinfo */

void show_malloc(player *p, char *str)
{
   struct mstats memstats;

   memstats = mstats();

   vtell_player(p, " Memory statistics:-\n"
                  "  Total Heap Size:                %8d\n"
                  "  Chunks Allocated:               %8d\n"
                  "  Byte Total of Chunks:           %8d\n"
                  "  Chunks in Free List:            %8d\n"
                  "  Byte Total of Free List Chunks: %8d\n",
           memstats.bytes_total, memstats.chunks_used, memstats.bytes_used,
           memstats.chunks_free, memstats.bytes_free);
}
#endif /* has_mallinfo */
#endif /* broken_mstats */


/* view logs */

void            view_log(player * p, char *str)
{
    char *oldstack, *ext;
    file logb;
    DIR *file_dir;
    struct dirent *log_name;
    
    oldstack = stack;
    
    switch (*str)
    {
      case 0:
      case '?':
	sprintf(stack, " Log files you can view:\n");
	stack = strchr( stack, 0 );
	file_dir = opendir("logs");
	for ( log_name=readdir(file_dir);
	     log_name != NULL; log_name=readdir(file_dir) )
	{
	  ext = strrchr(log_name->d_name, '.');
	  if ( !ext )
	    continue;
	  if ( !(strcasecmp(ext, ".log")) )
	    {
	      *ext = 0;
	      sprintf(stack, " %s,", log_name->d_name);
	      stack = strchr( stack, 0 );
	    }
	}
	if(file_dir)
	  closedir(file_dir);
	strcpy(stack, "\n");
	stack = end_string( oldstack );
	tell_player(p, oldstack);
	stack = oldstack;
	return;
      case '.':
	tell_player(p, " Uh-uh, you can't do that !\n");
	return;
    }
    sprintf(stack, "logs/%s.log", str);
    stack = end_string(stack);

    logb = load_file_verbose(oldstack, 0);
    if (logb.where)
    {
	if (*(logb.where))
	    pager(p, logb.where, 0);
	else
	    vtell_player(p, " Couldn't find logfile 'logs/%s.log'\n", str);
	free(logb.where);
    }
    stack = oldstack;
}

/* grep command :-) */
void            grep_log(player * p, char *str)
{
    char *oldstack, *scan, *scan2, *search;
    file logb;
    int count = 0;
    
    oldstack = stack;
    
    search = strchr(str, 0); /* get to end of line */
    search--;
    while(*search && *search!=' ')
           search--;
    if (*search)
       *search++=0;    
    if (!*search || (*str && (int)strlen(str)<2) || !*str)
    {
       tell_player(p, " Format: grep <string of 2 chars or more> <log file>\n");
       return;
    }
    
    sprintf(stack, "logs/%s.log", search);
    stack = end_string(stack);
    logb = load_file_verbose(oldstack, 0);
    stack = oldstack;
    
    if (logb.where)
    {
        if (*(logb.where))
        {
            scan=logb.where;
            while(*scan)
            {
                scan2=scan; /* set beginning of line pointer */
                /* search to find a match */
                while(*scan && *scan!='\n')
                {
                    if (!strncasecmp(scan, str, strlen(str)))
                    {
                        /* copy the line to the stack */
                        while(*scan2 && *scan2!='\n')
                            *stack++=*scan2++;
                        *stack++='\n';
                        /* get scan to the end of the line */
                        while(*scan&&*scan!='\n') scan++;
                        count++;
                        break;
                    }
                    scan++;
                }
                scan++; /* get scan to beginning of next line if we can */
            }
            if (count==0)
            {
               tell_player(p, " Sorry, no matches found.\n");
               free(logb.where);
               stack = oldstack;
               return;
            }
            sprintf(stack, " %d match%s found.\n", count, numeric_es(count));
            stack = strchr(stack, 0);
            *stack++=0;
            pager(p, oldstack, 0);
        }
        else
	    vtell_player(p, " Couldn't find logfile 'logs/%s.log'\n", search);
        free(logb.where);
    }
    stack = oldstack;
}


/* net stats */
void netstat(player * p, char *str)
{
   vtell_player(p, "Total bytes:\t\t(I) %d\t(O) %d\n"
                  "Average bytes:\t\t(I) %d\t\t(O) %d\n"
                  "Bytes per second:\t(I) %d\t\t(O) %d\n"
                  "Total packets:\t\t(I) %d\t(O) %d\n"
                  "Average packets:\t(I) %d\t\t(O) %d\n"
                  "Packets per second:\t(I) %d\t\t(O) %d\n",
                  in_total, out_total, in_average, out_average, in_bps, out_bps,
                  in_pack_total, out_pack_total, in_pack_average,
                  out_pack_average, in_pps, out_pps);
}


/* warn someone */

void warn(player * p, char *str)
{
   char *oldstack, *msg, *pstring, *final;
   player **list, **step;
   int i,n;

   oldstack = stack;
   align(stack);
   command_type = PERSONAL | SEE_ERROR | WARNING;

   if (p->saved_flags & BLOCK_TELLS)
   {
      tell_player(p, " You are currently BLOCKING TELLS. It might be an idea to"
                     " unblock so they can reply, eh?\n");
   }
   msg = next_space(str);
   if (*msg)
      *msg++ = 0;
   if (!*msg)
   {
      tell_player(p, " Format: warn <player(s)> <message>\n");
      stack = oldstack;
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Try going on_duty before doing that.\n");
       stack=oldstack;
       return;
     }

   /* no warns to groups */
   if (!strcasecmp(str, "everyone") || !strcasecmp(str, "friends")
       || !strcasecmp(str, "supers") || !strcasecmp(str, "sus")
       || strstr(str, "everyone"))
   {
      tell_player(p, " Now that would be a bit silly wouldn't it?\n");
      stack = oldstack;
      return;
   }
   /* should you require warning, the consequences are somewhat severe */
   if (!strcasecmp(str, "me"))
   {
      tell_player(p, " You Silly Sod\n\nBye!\n");
      stack = oldstack;
      p->flags |= TRIED_QUIT;
      quit(p, "");
      return;
   }
   list = (player **) ALFIX stack; 
   n = global_tag(p, str, 1);
   if (!n)
   {
      stack = oldstack;
      return;
   }
   final = stack;
   if (p->gender==PLURAL)
     sprintf(stack, "-=> %s warn you: %s\n\n", p->name, msg);
   else
     sprintf(stack, "-=> %s warns you: %s\n\n", p->name, msg);
   stack = end_string(stack);
   for (step = list, i = 0; i < n; i++, step++)
   {
      if (*step != p)
      {
         command_type |= HIGHLIGHT;
         tell_player(*step, "\007\n");
         tell_player(*step, final);
         command_type &= ~HIGHLIGHT;
      }
   }
   stack = final;

   command_type = 0;
   pstring = tag_string(p, list, n);
   cleanup_tag(list, n);
   vsu_wall("-=> %s warn%s %s: %s\n", p->name, single_s(p), pstring, msg);
   vlog("warn", "%s warn%s %s: %s", p->name, single_s(p), pstring, msg);
   stack = oldstack;
}





/* the eject command , muhahahahaa */

void sneeze(player * p, char *str)
{
   time_t t;
   int nologin = 0;
   char *oldstack, *num;
   player*e;

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: sneeze <person/s> [<time in minutes>]\n");
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player (p," You need to be on_duty to sneeze people\n");
       return;
     }

   t = time(0);
   if ((num = strrchr(str, ' ')))
      nologin = atoi(num) * 60;
   if (nologin > (60 * 10) && !(p->residency & ADMIN))
   {
      tell_player(p, " That amount of time is too harsh, set to 10 mins now "
                     "...\n");
      nologin = (60 * 10);
   }
   if (!nologin)
      nologin = 60;
   else
      *num = 0;
   while (*str)
   {
      while (*str && *str != ',')
         *stack++ = *str++;
      if (*str)
         str++;
      *stack++ = 0;
      if (*oldstack)
      {
         e = find_player_global(oldstack);
         if (e)
         {
            if (!check_privs(PRIVS(p), PRIVS(e)))
            {
               tell_player(p, " No way pal !!!\n");
               vtell_player(e, "-=> %s tried to sneeze over you.\n", p->name);
               vlog("sneeze", "%s failed to sneeze all over %s", p->name, e->name);
            } 
            else
            {
               tell_player(e, "\n\n One of the Super Users unclogs their nose in"
                             " your direction. You die a horrible green death."
                             "\n\n");
               e->sneezed = t + nologin;
               quit(e, 0);
               vtell_room(e->location, "-=> %s %s sneezed upon, and not at all "
                              "happy about it.\n", e->name, isare(e));
	       if (p->gender==PLURAL)
		 vsu_wall("-=> All of the %s group together, "
			 "handkerchieves at the ready, and as one they all "
			 "sneeze on %s.\n-=> %s was from "
			 "%s\n",p->name, e->name, e->name, e->inet_addr);
	       else
		 vsu_wall("-=> %s sneezes on %s.\n-=> %s was from "
			 "%s\n",p->name, e->name, e->name, e->inet_addr);
               vlog("sneeze", "%s sneeze%s on %s [%s]", p->name, single_s(p), e->name, e->inet_addr);
               sync_to_file(*(e->lower_name), 0);
            }
         }
      }
      stack = oldstack;
   }
}


/*
 * reset person (in case the su over does it (which wouldn't be like an su at
 * all.. nope no no))
 */

void reset_sneeze(player * p, char *str)
{
   char *newtime;
   time_t t=0, nologin=0;
   player dummy;

   if (!*str)
   {
      tell_player(p, " Format: reset_sneeze <person>"
                     " [<new time in minutes>]\n");
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go back on duty first.\n");
       return;
     }

   newtime = next_space(str);
   if (*newtime)
   {
      t = time(0);
      *newtime++ = 0;
      nologin = atoi(newtime) * 60;
      if (nologin > (60 * 10) && !( p->residency & ADMIN))
      {
         tell_player(p, " Now that isn't a very nice amount of time is it?\n"
                        " Reset to 10 minutes...\n");
         nologin = 60 * 10;
      }
      nologin += t;
   } 
   else
      nologin = 0;

   memset(&dummy, 0, sizeof(player));
   strcpy(dummy.lower_name, str);
   lower_case(dummy.lower_name);
   dummy.fd = p->fd;
   if (!load_player(&dummy))
   {
      tell_player(p, " No such person in saved files.\n");
      return;
   }
   switch (dummy.residency)
   {
      case STANDARD_ROOMS:
         tell_player(p, " That's a system room.\n");
         return;
      default:
         if (dummy.residency & BANISHD)
         {
            if (dummy.residency == BANISHD)
               tell_player(p, " That Name is banished.\n");
            else
               tell_player(p, " That Player is banished.\n");
            return;
         }
         break;
   }
   dummy.sneezed = nologin;
   dummy.location = (room *) - 1;
   save_player(&dummy);
   if (!nologin)
      vtell_player(p, " Reset the sneeze time on %s ...\n", dummy.name);
   else
      vtell_player(p, " Changed the Sneeze time on %s to %d seconds.\n",
              dummy.name, (int) (nologin - t));

   /* tell the SUs, too */
   if (!nologin) {
     vsu_wall("-=> %s reset%s the sneeze time on %s ...\n", p->name,single_s(p), dummy.name);
     vlog("sneeze", "%s reset%s the sneeze time on %s", p->name,single_s(p), dummy.name);
   }
   else {
     vsu_wall("-=> %s change%s the Sneeze time on %s to %d seconds.\n", p->name, single_s(p), dummy.name, (int) (nologin - t));
     vlog("sneeze", "%s change%s the Sneeze time on %s to %d seconds", p->name, single_s(p), dummy.name, (int) (nologin - t));
   }
}



/* the eject command (too) , muhahahahaa */
void soft_eject(player * p, char *str)
{
   char *reason;
   player *e;

   reason = next_space(str);
   if (*reason)
      *reason++ = 0;
   if (!*reason)
   {
      tell_player(p, " Format: drag <person> <reason>\n");
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Its best to be on_duty to do that kind of thing.\n");
       return;
     }

   e = find_player_global(str);
   if (e)
   {
      if (!check_privs(PRIVS(p), PRIVS(e)))
      {
         tell_player(p, " Sorry, you can't...\n");
         vlog("drag", "%s tried to drag %s", p->name, e->name);
      } else
      {
         tell_player(e, "\n\n A large wave drags you back into the sea and "
		"dumps you.\n   Divine punishment...\n\n");
         quit(e, 0);
         vtell_room(e->location, "-=> %s has been dragged into the sea by a large"
                        " wave.!!\n", e->name);
	 vsu_wall("-=> %s drag%s %s\n-=> %s was from %s\n", p->name, single_s(p), e->name, e->name, e->inet_addr);
         vlog("drag", "%s - %s : %s", p->name, e->name, reason);
      }
   }
}


/* similar to shout but only goes to super users (eject and higher) */
void su(player * p, char *str)
{
   command_type = 0;

   if (!*str)
   {
      tell_player(p, " Format: su <message>\n");
      return;
   }
   if (p->flags & BLOCK_SU)
   {
      tell_player(p, " You can't do sus when you're ignoring them.\n");
      return;
   }
   if (*str == ';') {
      str++;
      while (*str == ' ')
         str++;
      if ( p->flags & FROGGED )
         vsu_wall("<%s croakily %s>\n", p->name, str);
      else
         vsu_wall("<%s %s>\n", p->name, str);
   } 
   else {
      if ( p->flags & FROGGED )
         vsu_wall("<%s> %s Ribbet!\n", p->name, str);
      else
         vsu_wall("<%s> %s\n", p->name, str);
   }
}


/* su-emote.. it's spannerish, I know, but what the hell */

void suemote(player * p, char *str)
{
   command_type = 0;

   if (!*str)
   {
      tell_player(p, " Format: se <message>\n");
      return;
   }
   if (p->flags & BLOCK_SU)
   {
      tell_player(p, " You can't do su emotes when you're ignoring them.\n");
      return;
   }
   if ( p->flags & FROGGED )
      vsu_wall("<%s croakily %s>\n", p->name, str);
   else
      vsu_wall("<%s %s>\n", p->name, str);
}


/* Su think */
void suthink(player * p, char *str)
{
   command_type = 0;

   if (!*str)
   {
      tell_player(p, " Format: st <message>\n");
      return;
   }
   if (p->flags & BLOCK_SU)
   {
      tell_player(p, " You can't do su thinks when you're ignoring them.\n");
      return;
   }
   if ( p->flags & FROGGED )
      vsu_wall("<%s thinks in a green fashion . o O ( %s )>\n", p->name, str);
   else
      vsu_wall("<%s thinks . o O ( %s )>\n", p->name, str);
}


/* toggle whether the su channel is highlighted or not */
void su_hilited(player * p, char *str)
{
   if (p->saved_flags & SU_HILITED)
   {
      tell_player(p, " You will not get the su channel hilited.\n");
      p->saved_flags &= ~SU_HILITED;
   } else
   {
      tell_player(p, " You will get the su channel hilited.\n");
      p->saved_flags |= SU_HILITED;
   }
}


/* Sync all player files */
void sync_all_by_user(player * p, char *str)
{
   tell_player(p, " Starting to sync ALL players...");
   sync_all();
   tell_player(p, " Completed\n\r");
}


/* toggle whether the program is globally closed to newbies */
void close_to_newbies(player * p, char *str)
{
   int wall = 0;

   if (p->flags & BLOCK_SU )
     {
       tell_player(p," You cant do THAT when off_duty.\n");
       return;
     }

   if ((!strcasecmp("on", str)||!strcasecmp("open",str))
       && sys_flags & CLOSED_TO_NEWBIES)
   {
      sys_flags &= ~CLOSED_TO_NEWBIES;
      /*log the open*/
      vlog("newbies", "Program opened to newbies by %s", p->name);
      wall = 1;
   } 
   else if ((!strcasecmp("off", str)||!strcasecmp("close",str))
	      && !(sys_flags & CLOSED_TO_NEWBIES)) {
      sys_flags |= CLOSED_TO_NEWBIES;
#ifdef ANTISPAM
      newbie_reopen_count = -1;
#endif
      /*log the close*/
      vlog("newbies" ,"Program closed to newbies by %s",p->name);
      wall = 1;
   } else
      wall = 0;

   if (sys_flags & CLOSED_TO_NEWBIES)
   {
      if (!wall)
        tell_player(p, " Program is closed to all newbies.\n");
      else
        vsu_wall("\n   <%s closes the prog to newbies>\n\n", p->name);
   } 
   else
   {
      if (!wall)
        tell_player(p, " Program is open to newbies.\n");
      else
        vsu_wall("\n   <%s opens the prog to newbies>\n\n", p->name);
   }
}


/* command to list lots of info about a person */
void check_info(player * p, char *str)
{
   player dummy, *p2;
   char *oldstack;

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: check info <player>\n");
      return;
   }
   memset(&dummy, 0, sizeof(player));

   p2 = find_player_absolute_quiet(str);
   if (p2)
      memcpy(&dummy, p2, sizeof(player));
   else
   {
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " No such person in saved files.\n");
         return;
      }
   }

   switch (dummy.residency)
   {
      case STANDARD_ROOMS:
         tell_player(p, " Standard rooms file\n");
         return;
      default:
         if (dummy.residency & BANISHD)
         {
            if (dummy.residency == BANISHD)
               tell_player(p, "BANISHED (Name only).\n");
            else
               tell_player(p, "BANISHED.\n");
         }
         sprintf(stack, "            <   Res  >                <SU >\n"
                        "            B ETbsMLBS    OW    F ST  LH SA\n"
                        "Residency   %s\n", bit_string(dummy.residency));
         break;
   }
   stack = strchr(stack, 0);

   sprintf(stack, "%s %s %s\n%s\n%s\n%s %s\nEMAIL:%s\n",
           dummy.pretitle, dummy.name, dummy.title, dummy.description,
           dummy.plan, dummy.name, dummy.enter_msg, dummy.email);
   stack = strchr(stack, 0);
   if(dummy.saved) {
     sprintf(stack, "SAVED:%s\n", dummy.saved->saved_email);
     stack = strchr(stack, 0);
   }
   switch (dummy.gender)
   {
      case MALE:
         strcpy(stack, "Gender set to male.\n");
         break;
      case FEMALE:
         strcpy(stack, "Gender set to female.\n");
         break;
      case PLURAL:
         strcpy(stack, "Gender set to plural.\n");
         break;
      case OTHER:
         strcpy(stack, "Gender set to something.\n");
         break;
      case VOID_GENDER:
         strcpy(stack, "Gender not set.\n");
         break;
   }
   stack = strchr(stack, 0);
   if (dummy.password[0] == 0)
   {
      strcpy(stack, "NO PASSWORD SET\n");
      stack = strchr(stack, 0);
   }
   sprintf(stack, "            CHTSHPQEPRSHENAMNALICNDPSFSJRES-\n"
                  "Saved flags %s\n", bit_string(dummy.saved_flags));
   stack = strchr(stack, 0);
   sprintf(stack, "            PRNREPCPTLCEISDRUBSWAFS---------\n"
                  "flags       %s\n", bit_string(dummy.flags));
   stack = strchr(stack, 0);
   sprintf(stack, "Max: rooms %d, exits %d, autos %d, list %d, mails %d\n",
           dummy.max_rooms, dummy.max_exits, dummy.max_autos,
           dummy.max_list, dummy.max_mail);
   stack = strchr(stack, 0);
   sprintf(stack, "Term: width %d, wrap %d\n",
           dummy.term_width, dummy.word_wrap);
   stack = strchr(stack, 0);
   if (dummy.script)
   {
      sprintf(stack, "Scripting on for another %s.\n",
              word_time(dummy.script));
      stack = strchr(stack, 0);
   }
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* command to view email status about people on the prog */
void view_player_email(player * p, char *str)
{
   player *scan;
   char *oldstack, middle[80];
   int page, pages, count;

   oldstack = stack;
   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;

   pages = (current_players - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;

   if (current_players == 1)
      strcpy(middle, "There is only you on the program at the moment");
   else
      sprintf(middle, "There are %s people on the program",
              number2string(current_players));
   pstack_mid(middle);

   count = page * (TERM_LINES - 2);
   for (scan = flatlist_start; count; scan = scan->flat_next)
   {
      if (!scan)
      {
         tell_player(p, " Bad where listing, abort.\n");
         log("error", "Bad where list");
         stack = oldstack;
         return;
      } else if (scan->name[0])
         count--;
   }

   for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
   {
      if (scan->name[0] && scan->location)
      {
         if (scan->residency == NON_RESIDENT)
            sprintf(stack, "%s is non resident.\n", scan->name);
         else if (scan->email[0])
         {
            if (scan->email[0] == ' ' && scan->email[1] == 0)
               sprintf(stack, "%s has a validated email address.\n",
                       scan->name);
            else
            {
               sprintf(stack, "%s [%s]\n", scan->name, scan->email);
               if (scan->saved_flags & PRIVATE_EMAIL)
               {
                  while (*stack != '\n')
                     stack++;
                  strcpy(stack, " (private)\n");
               }
            }
         } else
         sprintf(stack, "%s has not set an email address.\n", scan->name);
         stack = strchr(stack, 0);
         count++;
      }
   }
   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   pstack_mid(middle);

   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* command to validate lack of email */

void validate_email(player * p, char *str)
{
   player *p2;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," You cant validate emails when off_duty.\n");
       return;
     }

   p2 = find_player_global(str);
   if (!p2)
      return;
   p2->email[0] = ' ';
   p2->email[1] = 0;
   sys_flags |= INVIS_SAVE;
   save_player(p2);
   sys_flags &= ~INVIS_SAVE;
   tell_player(p, " Set player as having no email address.\n");

   vlog("validate_email" ,"%s validated email for %s", p->name, p2->name);
}


/* New version of blankpass */

void new_blankpass(player *p, char *str)
{
   char *pass;
   player *p2, dummy;

   if(!*str)
   {
       tell_player(p, " Format: blankpass <player> <new password>\n");
       return;
   } 

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," You cant blankpass when off_duty.\n");
       return;
     }

   pass = 0;
   pass = strchr(str, ' ');
   if (pass)
   {
      *pass++ = 0;
      if ((int)strlen(pass) > (MAX_PASSWORD-2) || (int)strlen(pass) < 3)
      {
         tell_player(p, " Try a reasonable length password.\n");
         return;
      }
   }
   else {
     tell_player(p, " Format: blankpass <player> <new password>\n");
     return;
   }
   
   lower_case(str);
   p2 = find_player_absolute_quiet(str);
   if (p2)
   {
/* if player is logged in */
      if (!check_privs(PRIVS(p),PRIVS(p2)) && !(p->residency & HCADMIN) )
      {
         tell_player(p, " You can't blankpass THAT person!\n");
         vsu_wall_but(p, "-=> %s TRIED to blankpass %s!\n", p->name, p2->name);
         return;
      }
      vtell_player(p2, "-=> %s has just changed your password.\n", p->name);
      strcpy(p2->password, do_crypt(pass, p2));
      tell_player(p, " Password changed. They have NOT been informed of what it is.\n");
      vlog("blanks", "%s changed %s's password (logged in)", p->name, p2->name);
      set_update(*str);
      return;
    }
   else
     {
       strcpy(dummy.lower_name, str);
       dummy.fd = p->fd;
       if (load_player(&dummy))
       {
	   if (dummy.residency & BANISHD)
	   {
	       tell_player(p, " By the way, this player is currently BANISHD.");
	       if (dummy.residency == BANISHD)
		   tell_player(p, " (Name Only)\n");
	       else
		   tell_player(p, "\n");
	   }
      if (!check_privs(PRIVS(p), dummy.residency) && !(p->residency & HCADMIN))
      {
         tell_player(p, " You can't blankpass THAT person!\n");
         vsu_wall_but(p, "-=> %s TRIED to blankpass %s!\n", p->name, dummy.name);
         return;
      }
	       strcpy(dummy.password, do_crypt(pass, &dummy));
	       tell_player(p, " Password changed in saved files.\n");
	       vlog("blanks", "%s changed %s's password (logged out)", p->name,
		 dummy.name);
	   dummy.script = 0;
	   dummy.script_file[0] = 0;
	   dummy.flags &= ~SCRIPTING;
	   dummy.location = (room *) -1;
	   save_player(&dummy);
       } else
	 tell_player(p, " Can't find that player in saved files.\n");
     }   
}


/* a test fn to test things 
void test_fn(player * p, char *str)
{
   do_birthdays();
}*/


/* give someone lag ... B-) */
void            add_lag(player * p, char *str)
{
   char           *size;
   int             new_size;
   player         *p2;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Lagging isnt nice at the best of times, the least "
		   "you can do is go on_duty before you torture the poor "
		   "victim {:-)\n");
       return;
     }

   size = next_space(str);
   *size++ = 0;
   new_size = atoi(size);

   /*change minutes to seconds*/
   new_size*=60;

   /* can't check with new_size == 0 as we need that for unlagging */
   /* check for duff command syntax */
   if (strlen(size) == 0)
   {
      tell_player(p, " Format: lag <player> <time in minutes>\n");
      return;
   }
   /* find them and return if they're not on */
   p2 = find_player_global(str);
   if (!p2)
      return;
   /* thou shalt not lag those above you */
   if (!check_privs(PRIVS(p), PRIVS(p2)))
   {
       tell_player(p, " You can't do that !!\n");
       vtell_player(p2, "-=> %s tried to lag you.\n", p->name);
       return;
   }
   
   /* check for silly or nasty amounts of lag */
   if (new_size < 0)
   {
	 tell_player(p, " That's not nice, and anyway you can't lag anyone "
		     "permanently any more. Set to 10 minutes.\n");
	 new_size = 600;
   }
   if (new_size > 600 && !(p->residency & ADMIN))
   {
       tell_player(p, "That's kinda excessive, set to 10 minutes.\n");
       new_size = 600;
   }
   /* lag 'em */
   p2->lagged = new_size;

   /* report success */
   if (new_size == 0)
   {
       vtell_player(p, " %s has been unlagged.\n", p2->name);
       vsu_wall_but(p, "-=> %s unlags %s.\n",p->name,p2->name);
       vlog("lag", "%s unlags %s",p->name,p2->name);
   }
   else
   {
       tell_player(p, " Tis Done ..\n");
       vsu_wall("-=> %s lags %s for %d minutes.\n",p->name,p2->name,new_size/60);
       vlog("lag", "%s lags %s for %d minutes",p->name,p2->name,new_size/60);
   }
}


/* remove shout from someone for a period */
void remove_shout(player * p, char *str)
{
   char *size = 0;
   int new_size = 5;
   player *p2;
   
   if (!*str)
   {
      tell_player(p, " Format: rm_shout <player> [<for how long>]\n");
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," You need to be on_duty for that\n");
       return;
     }

   size = strchr(str, ' ');
   if (size)
   {
      *size++ = 0;
      new_size = atoi(size);
   }
   p2 = find_player_global(str);
   if (!p2)
      return;
   if (!check_privs(PRIVS(p), PRIVS(p2)))
   {
      tell_player(p, " You can't do that !!\n");
      vtell_player(p2, "-=> %s tried to remove shout from you.\n", p->name);
      return;
   }
   p2->saved_flags &= ~SAVENOSHOUT;
   if (new_size)
      tell_player(p2, "-=> You suddenly find yourself with a sore throat.\n");
   else
      tell_player(p2, "-=> Someone hands you a cough sweet.\n");
   if (new_size > 30)
     if (!(p->residency & ADMIN))
       new_size = 5;
   switch (new_size)
   {
      case -1:
         vsu_wall("-=> %s just remove shouted %s. (permanently!)\n",
                 p->name, p2->name);
         p2->saved_flags |= SAVENOSHOUT;
         p2->no_shout = -1;
         break;
      case 0:
         vsu_wall("-=> %s just allowed %s to shout again.\n", p->name,p2->name);
         break;
      case 1:
         vsu_wall("-=> %s just remove shouted %s for 1 minute.\n",p->name, p2->name);;
         break;
      default:
         vsu_wall("-=> %s just remove shouted %s for %d minutes.\n",p->name, p2->name, new_size);
         break;
   }
   new_size *= 60;
   if (new_size >= 0)
      p2->no_shout = new_size;

   if (new_size != 0)
     vlog("rm_shout", "%s removed %s's shout for %d.",p->name,p2->name, new_size);
   else
     vlog("rm_shout", "%s regranted shouts to %s.",p->name,p2->name);
}


/* remove trans movement from someone for a period */

void remove_move(player * p, char *str)
{
   char *size;
   int new_size = 5;
   player         *p2;

   if (!*str)
   {
      tell_player(p, " Format: rm_move <player> [<for how long>]\n");
      return;
   }

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," You need to be on_duty for that.\n");
       return;
     }

   size = strchr(str, ' ');
   if (size)
   {
      *size++ = 0;
      new_size = atoi(size);
   } else
      new_size = 1;
   p2 = find_player_global(str);
   if (!p2)
      return;
   if (!check_privs(PRIVS(p), PRIVS(p2)))
   {
      tell_player(p, " You can't do that !!\n");
      vtell_player(p2, "-=> %s tried to remove move from you.\n", p->name);
      return;
   }
   if (new_size)
      tell_player(p2, "-=> You step on some chewing-gum, and you suddenly "
                      "find it very hard to move ...\n");
   else
      tell_player(p2, "-=> Someone hands you a new pair of shoes ...\n");
   if (new_size > 30)
      new_size = 5;
   new_size *= 60;
   if (new_size >= 0)
      p2->no_move = new_size;
   else 
      p2->no_move = -1;
   if ((new_size/60) == 1)
      vsu_wall("-=> %s remove moves %s for 1 minute.\n", p->name,p2->name);
   else if (new_size == 0)
      vsu_wall("-=> %s allows %s to move again.\n", p->name,p2->name);
   else if (new_size <0 )
      vsu_wall("-=> %s remove moves %s. Permanently!\n", p->name,p2->name);
   else
      vsu_wall("-=> %s remove moves %s for %d minutes.\n", p->name,p2->name, new_size/60);
   
   if (new_size != 0)
     vlog("rm_move", "%s removed %s's move for %d",p->name,p2->name, new_size);
   else
     vlog("rm_move", "%s allowed %s to move again",p->name,p2->name);   
}


/* change someones max list limit */

void change_limit(player * p, char *str)
{
   char *size;
   char *play;
   int new_size, *changeme=0;
   player *p2;
   player dummy;

   lower_case(str);
   play = next_space(str);
   if(*play) *play++=0;
   size = next_space(play);
   if(*size) *size++=0;
   new_size = atoi(size);
   /* syntax check */
   if (!new_size)
   {
      tell_player(p, " Format: chlim <limit> <player> <new size>\n"
      		     " Limits are: auto, exit, list, mail, room\n");
      return;
   }

   /* check valid type to set */
   if(strcmp(str, "auto") && strcmp(str, "exit") && strcmp(str, "list") &&
      strcmp(str, "mail") && strcmp(str, "room")) {
     tell_player(p, " Limits are: auto, exit, list, mail, room\n");
     return;
   }
   /* negative limit trap */
   if (new_size < 0)
     {
       vtell_player(p, " Now try a _positive_ %s limit...\n", str);
       return;
     }

   p2 = find_player_absolute_quiet(play);
   if (!p2)
   {
      memset(&dummy, 0, sizeof(player));
      strcpy(dummy.lower_name, play);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " That player doesn't exist.\n");
         return;
      }
      p2 = &dummy;
   }
   if (!check_privs(PRIVS(p), PRIVS(p2)))
   {
      tell_player(p, " You can't do that !!\n");
      vtell_player(p2, "-=> %s tried to change your %s limit.\n", p->name, str);
      return;
   }
   
   if(!strcmp(str, "auto"))
     changeme = &(p2->max_autos);
   else if(!strcmp(str, "exit"))
     changeme = &(p2->max_exits);
   else if(!strcmp(str, "list"))
     changeme = &(p2->max_list);
   else if(!strcmp(str, "mail"))
     changeme = &(p2->max_mail);
   else if(!strcmp(str, "room"))
     changeme = &(p2->max_rooms);
   /* log it */
   vlog("chlim", "%s changed %s'%s %s limit from %d to %d",
   	p->name, p2->name, trailing_s(p2), str, (*changeme), new_size);
   	
   (*changeme) = new_size;

   if (p2 != &dummy)
      vtell_player(p2, "-=> %s has changed your %s limit to %d.\n", p->name, str, new_size);
   else
      save_player(&dummy);
   tell_player(p, " Tis Done ..\n");
}


void sync_files(player * p, char *str)
{
   if (!isalpha(*str))
   {
      tell_player(p, " Argument must be a letter.\n");
      return;
   }
   sync_to_file(tolower(*str), 1);
   tell_player(p, " Sync succesful.\n");
}


/* manual retrieve from disk */

void restore_files(player * p, char *str)
{
   if (!isalpha(*str))
   {
      tell_player(p, " Argument must be a letter.\n");
      return;
   }
   remove_entire_list(tolower(*str));
   hard_load_one_file(tolower(*str));
   tell_player(p, " Restore succesful.\n");
}


/* shut down the program */

void pulldown(player * p, char *str)
{
   char *oldstack, *reason, *i;

   if (p->flags & BLOCK_SU)
   {
       tell_player(p," You need to be on_duty for that\n");
       return;
   }
   oldstack = stack;
   command_type &= ~HIGHLIGHT;
   
   if (!(p->residency & (LOWER_ADMIN|ADMIN)))
   {
       /* SUs can see a shutdown but not start one */
       if (*str)
       {
	   /* lest they try... */
	   tell_player(p, " NOT bloody likely.\n");
	   return;
       }
       if (shutdown_count > 1)
	   /* if a shutdown is in progress */
       {
	   /* contruct the message to tell them and send it to them */
	   vtell_player(p, "\n %s, in %d seconds.\n",
		   shutdown_reason, shutdown_count);
	   return;
       }
       else
       {
	   /* tell them no joy */
	   tell_player(p, " No shutdown in progress.\n");
	   return;
       }
       
   }
   if (!*str)
   {
       if (shutdown_count > -1)
       {
	   vtell_player(p, "\n %s, in %d seconds.\n  \'shutdown -1\' to abort.\n\n"
		   , shutdown_reason, shutdown_count);
	   return;
       } else
       {
	   tell_player(p, " Format: shutdown <countdown> [<reason>]\n");
	   return;
       }
   }
   reason = strchr(str, ' ');
   if (!reason)
   {
      sprintf(shutdown_reason, "%s is shutting the program down - it is "
	      "probably for a good reason too\n",p->name);
   } else
   {
      *reason++ = 0;
      sprintf(shutdown_reason, "%s is shutting the program down - %s",
              p->name, reason);
   }
   if (!strcmp(str, "-1"))
   {
      shutdown_reason[0] = '\0';
      if (shutdown_count < 300)
      {
         raw_wall("\n\nShutdown aborted "
                  "(If you ever knew one was in progress...)\n\n");
      } else
      {
         tell_player(p, " Shutdown Aborted.\n");
      }
      shutdown_count = -1;
      return;
   }
   i = str;
   while (*i != 0)
   {
      if (!isdigit(*i))
      {
         tell_player(p, " Format: shutdown <countdown> [<reason>]\n");
         return;
      }
      i++;
   }
   shutdown_count = atoi(str);
   vtell_player(p, "-=> Program set to shutdown in %d seconds...\n", shutdown_count);
   command_type &= ~HIGHLIGHT;
   stack = oldstack;
}


/* wall to everyone, non blockable */

void wall(player * p, char *str)
{
   char *oldstack;

   if(p->flags & BLOCK_SU)
     {
       tell_player (p,"Permissions changed...\nOnly kidding {:-) \n"
		    "No, seriously, you cant use wall when off_duty.\n");
       return;
     }

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: wall <arg>\n");
      return;
   }
   sprintf(oldstack, " %s screams -=> %s <=-\007\n", p->name, str);
   stack = end_string(oldstack);
   command_type |= (HIGHLIGHT|PERSONAL|WARNING);
   raw_wall(oldstack);
   command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
   stack = oldstack;
}


/* permission changes routines */

/* the resident command */

void resident(player * p, char *str)
{
   player *p2;
   int ressie = 0;

   if (!*str)
   {
      tell_player(p, " Format: resident <whoever>\n");
      return;
   }

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," Nope, not whilst you are off_duty you wont.\n");
       return;
     }
   
   if (!strcasecmp(str, "me"))
      p2 = p;
   else
      p2 = find_player_global(str);
   if (!p2)
      return;

   if (!strcasecmp(p2->name, "guest"))
   {
      tell_player(p, "\n The name 'Guest' is reserved because people may use "
                     "that when first logging in before using the name they "
                     "REALLY want to use. So get this person to choose another "
                     "name, THEN make them resident.\n\n");
      return;
   }
   if ((p2->residency != NON_RESIDENT) && p2 != p)
   {
      if (p2->saved)
      {
         if (p2->saved->last_host)
         {
            if (p2->saved->last_host[0] != 0)
            {
               tell_player(p, " That player is already resident, and has "
                              "re-logged in\n");
               return;
            }
         }
      }
      ressie = 1;
   }
   if (ressie)
      vtell_player(p2, "\n\n-=> You are now a resident.\n");
   else
   {
     if (p->gender==PLURAL)
       vtell_player(p2, "\n\n-=> %s have made you a resident.\n", p->name);
     else
       vtell_player(p2, "\n\n-=> %s has made you a resident.\n", p->name);
   }

   vtell_player(p2, " For this to take effect, you MUST set an email address"
     " and password NOW.\n"
     " If you don't you will still not be able to save, and next time you"
     " log in, you will be no longer resident.\n"
     " To set an email address, simply type 'email <whatever>' as a command. "
     "(without the quotes or <>'s)\n"
     " You must use your proper system email address for this.\n"
     " To set your password, simply type 'password' as a command, and follow"
     " the prompts.\n"
     " IF you get stuck, read the help, using the command 'help',  ask %s, "
     "or any other Super User (type lsu for a list), for help...\n\n",
	   p->name);

   if (ressie)
   {
      vtell_player(p, " You repeat the message about setting email and "
                     "password to %s\n", p2->name);
      return;
   }
   if (p2 != p)
   {
      p2->residency |= get_flag(permission_list, "residency");
      p2->residency |= NO_SYNC;
      p2->email[0] = 0;
      p2->flags &= ~SCRIPTING;
      p2->flags |= ARGH_DONT_CACHE_ME;
      p2->script = 0;
      p2->script_file[0] = 0;
      strcpy(p2->script_file, "dummy");
      tell_player(p, " Residency granted ...\n");

      if (p->gender==PLURAL)
	vsu_wall("-=> All the %s gang up and grant residency to %s\n", p->name,p2->name);
      else
	vsu_wall("-=> %s grants residency to %s\n", p->name, p2->name);
      p2->saved_residency = p2->residency;
      p2->saved = 0;
      vlog("resident", "%s made %s a resident.", p->name, p2->name);
   }
}


/* the grant command */

void grant(player * p, char *str)
{
   char *permission;
   player *p2;
   saved_player *sp;
   int change;
   char *oldstack;
   int count;

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," Don't think I want to grant that till you go back "
		   "on_duty\n");
       return;
     }
   
   oldstack = stack;
   permission = next_space(str);
   if (!*str || !*permission)
   {
      strcpy(stack, " Format: grant <whoever> <whatever>\n"
                    " Grantable privs are: ");
      stack = strchr(stack, 0);
      for (count=0;permission_list[count].text!=0;count++)
      {
         sprintf(stack, "%s, ", permission_list[count].text);
         stack = strchr(stack, 0);
      }
      while (*stack != ',')
         stack--;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   *permission++ = 0;

   change = get_flag(permission_list, permission);
   if (!change)
   {
      tell_player(p, " Can't find that permission.\n");
      return;
   }
   if (!(p->residency & change))
   {
     if ( !(p->residency & HCADMIN))
      {
         tell_player(p, " You can't give out permissions you haven't got "
           "yourself.\n");
         return;
      }
   }
   p2 = find_player_global(str);
   if (!p2)
   {
      lower_case(str);
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Couldn't find player.\n");
         stack = oldstack;
         return;
      }
      if (sp->residency == BANISHD || sp->residency == STANDARD_ROOMS)
      {
         tell_player(p, " That is a banished NAME, or System Room.\n");
         stack = oldstack;
         return;
      }
      if ((change == STANDARD_ROOMS) && !(sp->residency == 0))
      {
         tell_player(p, " You can't grant sysroom to anything but a blank"
                        "playerfile.\n");
         stack = oldstack;
         return;
      }
      if(!check_privs(PRIVS(p), sp->residency))
      {
         tell_player(p, " You can't alter that save file\n");
         vlog("grant", "%s failed to grant %s to %s\n", p->name,
                 permission, str);
         return;
      }
      tell_player(p, " Permission changed in player files.\n");
      stack = oldstack;
      vlog("grant", "%s granted %s to %s", p->name, permission, 
	      sp->lower_name);
      sp->residency |= change;
      set_update(*str);
      return;
   } else
   {
      if (p2->residency == NON_RESIDENT)
      {
         tell_player(p, " That player is non-resident!\n");
         stack = oldstack;
         return;
      }
      if (p2->residency == BANISHD || p2->residency == STANDARD_ROOMS)
      {
         tell_player(p, " That is a banished NAME, or System Room.\n");
         stack = oldstack;
         return;
      }
      if (!check_privs(PRIVS(p), PRIVS(p2)))
      {
         tell_player(p, " No Way Pal !!\n");
         vtell_player(p2, "-=> %s tried to grant your permissions.\n", p->name);
         stack = oldstack;
         return;
      }
      sprintf(oldstack, "\n%s has changed your permissions.\n", p->name);
      p2->saved_residency |= change;
      p2->residency = p2->saved_residency;
      stack = strchr(stack, 0);
      if (p2->residency & SU)
      {
         strcpy(stack, "Read the appropriate files please ( shelp "
                       "basic and shelp advanced )\n\n");
      }
      stack = end_string(oldstack);
      tell_player(p2, oldstack);
      stack = oldstack;
      vlog("grant", "%s granted %s to %s", p->name, permission, p2->name);
      save_player(p2);
      tell_player(p, " Permissions changed ...\n");
   }
   stack = oldstack;
}


/* the remove command */

void remove_priv(player * p, char *str)
{
   char *permission;
   player *p2;
   saved_player *sp;
   int change, count;
   char *oldstack;

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," Go back on duty first.\n");
       return;
     }

   oldstack = stack;
   permission = next_space(str);
   if (!*str || !*permission)
   {
      strcpy(stack, " Format: remove <whoever> <whatever>\n"
                    " Remove-able privs are: ");
      stack = strchr(stack, 0);
      for (count=0;permission_list[count].text!=0;count++)
      {
         sprintf(stack, "%s, ", permission_list[count].text);
         stack = strchr(stack, 0);
      }
      while (*stack != ',')
         stack--;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   *permission++ = 0;

   if (!(strcasecmp("everyone", str))
         && !(strcasecmp("everything", permission))
         && (p->residency & (1 << 27)))
   {
      tell_player(p, "\n You can sod off and die if you think I'm going to "
        "let you do that ...\n\n");
      su_wall("\n-=>Someone just tried to rm -rf * my entire sodding "
         "directory!\n\n");
      return;
   }
   change = get_flag(permission_list, permission);
   if (!change)
   {
      tell_player(p, " Can't find that permission.\n");
      return;
   }
   if (!(p->residency & change))
   {
      if ( !(p->residency & HCADMIN) )
      {
         tell_player(p, " You can't remove permissions you haven't got "
                        "yourself.\n");
         return;
      }
   }

   p2 = find_player_global(str);
   if (!p2)
   {
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Couldn't find player.\n");
         return;
      }
      if (!check_privs(PRIVS(p), sp->residency))
      {
         tell_player(p, " You cant change that save file !!!\n");
         vlog("grant", "%s failed to remove %s from %s", p->name,
                 permission, str);
         return;
      }
      sp->residency &= ~change;
      if (sp->residency == NON_RESIDENT)
         remove_player_file(sp->lower_name);
      set_update(*str);
      tell_player(p, " Permissions changed in save files.\n");
      stack = oldstack;
      vlog("grant", "%s removes %s from %s", p->name,
           permission, str);
      return;
   } else
   {
      if (!check_privs(PRIVS(p), PRIVS(p2)))
      {
         tell_player(p, " No Way Pal !!\n");
         vtell_player(p2, "-=> %s tried to remove your permissions.\n", p->name);
         stack = oldstack;
         return;
      }
      p2->residency &= ~change;
      p2->saved_residency = p2->residency;
      vtell_player(p2, "-=> %s has changed your permissions.\n", p->name);
      if (p2->residency != NON_RESIDENT)
         save_player(p2);
      else
         remove_player_file(p2->lower_name);
      tell_player(p, " Permissions changed ...\n");
   }
   stack = oldstack;
}


/* remove player completely from the player files */

void nuke_player(player * p, char *str)
{
   char *reason;
   player *p2, dummy, *fscan, *prev=0;
   saved_player *sp;
   char nuked[MAX_NAME] = "";
   char nukee[MAX_NAME] = "";
   char naddr[MAX_INET_ADDR] = "";
   int mcount = 0, sscan, mesg_done = 0;
   recmail *scan;
   note *smail, *snext;
#ifdef DYNAMIC
   int cached;
#endif

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," I'm sorry Dave, I cannot allow you to do that\n"
		   " Luv - Hal\n"
		   " OK, not really, it was cos you were off_duty. {:-)\n");
       return;
     }
   
   
   if(!*str) {
     tell_player(p, " Format: nuke <player> <reason>\n");
     return;
   }
   if(*str) reason = next_space(str);
   if(*reason) *reason++=0;
   if(!*reason) {
     tell_player(p, " Format: nuke <player> <reason>\n");
     return;
   }
   p2 = find_player_absolute_quiet(str);
   if (!p2)
      tell_player(p, "No such person on the program.\n");
   if (p2)
   {
      if (!check_privs(PRIVS(p), PRIVS(p2)))
      {
         tell_player(p, " You can't nuke them !\n");
         vtell_player(p2, "-=> %s tried to nuke you.\n", p->name);
         return;
      }
      if (p2->saved)
         all_players_out(p2->saved);
      tell_player(p2, 
             "\n"
             "-=> There are times that you wish you'd developed      <=-\n"
             "-=> some way of stopping stray nukes from dropping on  <=-\n"
             "-=> your head and killing you.                         <=-\n"
             "-=>                                                    <=-\n"
             "-=> This is one of those times, mostly because that's  <=-\n"
             "-=> what just happened.                                <=-\n"
             "-=>                                                    <=-\n"
             "-=> See you in hell :-)                                <=-\n"
             "-=>                                                    <=-\n"
             "-=> (If you hadn't guessed, you've just been nuked...) <=-\n");
      p2->saved = 0;
      p2->residency = 0;
      p2->flags |= TRIED_QUIT;
      quit(p2, "");
      strcpy(nuked, p2->name);
      strcpy(naddr, p2->inet_addr);
      vsu_wall("-=> %s nuke%s %s to a crisp, toast time!\n-=> %s "
		"was from %s\n",p->name, single_s(p), nuked, nuked, naddr);
      mesg_done = 1;
#ifdef DYNAMIC
      cached = find_cached_player(p2->lower_name);
      if(cached>=0) {
        sync_cache_item_to_disk(cached);
        empty_cache_item(cached);
      }
#endif
   }
   strcpy(nukee, str);
   lower_case(nukee);
   sp = find_saved_player(nukee);
   if (!sp)
   {
      vtell_player(p, " Couldn't find saved player '%s'.\n", str);
      return;
   }
   
   if (!check_privs(PRIVS(p), sp->residency))
   {
      tell_player(p, " You can't nuke that save file !\n");
      return;
   }
/* TRY to clean up notes */
   memset(&dummy, 0, sizeof(dummy));
   strcpy(dummy.lower_name, sp->lower_name);
   dummy.fd = p->fd;
   load_player(&dummy);
   if (!*nuked)
   {
      strcpy(nuked, dummy.name);
      strcpy(naddr, sp->last_host);
   }
   scan = dummy.saved->received_list;
   if (scan) {
      for (mcount=0; scan; scan=scan->next, mcount++);
      for (;mcount;mcount--)
         delete_received_fn(&dummy, 1);
   }
   mcount = 1;
   sscan = dummy.saved->mail_sent;
   smail = find_note(sscan);
   if (smail)
   {
      while (smail)
      {
         mcount++;
         sscan = smail->next_item;
         snext = find_note(sscan);
         if (!snext && sscan)
         {
            smail->next_item = 0;
            smail = 0;
         } 
         else
            smail = snext;
      }
      for(;mcount;mcount--)
         delete_sent_fn(&dummy, 1);
   }
   save_player(&dummy);
/* END clean up notes */
   all_players_out(sp);
   tell_player(p, " Files succesfully nuked.\n");
   if(!mesg_done)
      vsu_wall("-=> %s nuke%s \'%s\' to a crisp, toast time!\n",p->name, single_s(p), sp->lower_name);
   vlog("nuke", "%s nuked %s [%s] with reason: %s", p->name, nuked, naddr, reason);
   remove_player_file(nukee);
   
   /* make sure they weren't sitting at the login screen.. */
   fscan = flatlist_start;
   while(fscan) {
     prev = fscan;
     fscan = fscan->flat_next;
     if(!strcasecmp(prev->name, nukee))
       quit(prev, 0);
   }
}


/* banish a player from the program */
void banish_player(player * p, char *str)
{
   char *reason, *i, ban_name[MAX_NAME + 1] = "";
   player *p2;
   saved_player *sp;
   int newbie=0;

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," Nope, go back on duty, doing it when off_duty is "
		   "cheating.\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: banish <player> <reason>\n");
      return;
   }
   reason = next_space(str);
   if(*reason) *reason++=0;
   if(!*reason) {
     tell_player(p, " Format: banish <player> <reason>\n");
     return;
   }
   ban_name[0] = 0;
   vlog("banish", "%s %s trying to banish %s with reason: %s", p->name, isare(p), str, reason);
   lower_case(str);
   p2 = find_player_absolute_quiet(str);
   if (!p2)
      tell_player(p, " No such person on the program.\n");
   if (p2)
   {
      if (!check_privs(PRIVS(p), PRIVS(p2)))
      {
         tell_player(p, " You can't banish them !\n");
         vtell_player(p2, "-=> %s tried to banish you.\n", p->name);
         return;
      }
      if ( p2->residency == NON_RESIDENT )
         newbie=1;
      tell_player(p2, "\n\n-=> You have been banished !!!.\n\n\n");
      p2->saved_residency |= BANISHD;
      p2->residency = p2->saved_residency;
      p2->flags |= TRIED_QUIT;
      quit(p2, "");
      strcpy(ban_name, p2->name);
   }
   if (!newbie)
   {
      sp = find_saved_player(str);
      if (sp)
      {
         if (sp->residency & BANISHD)
         {
            tell_player(p," Already banished!\n");
            return;
         }
         if (!check_privs(PRIVS(p), sp->residency))
         {
            tell_player(p, " You can't banish that save file !\n");
            return;
         }
         sp->residency |= BANISHD;
         strcpy(ban_name, sp->lower_name);
         set_update(*str);
         tell_player(p, " Player successfully banished.\n");
      } 
      else
      {
      /* Create a new file with the BANISHD flag set */
         i = str;
         while (*i)
         {
            if (!isalpha(*i++))
            {
               tell_player(p, " Banished names must only contain letters!\n");
               return;
            }
         }
         create_banish_file(str);
         tell_player(p, " Name successfully banished.\n");
      }
   }
   else { /* it is a newbie */
     /* create a banish file */
     i = str;
     while(*i) {
       if(!isalpha(*i++)) {
         tell_player(p, " Banished names must only contain letters!\n");
         return;
       }
     }
     create_banish_file(str);
     tell_player(p, " Name successfully banished.\n");
   }
   if (ban_name[0] != 0 && !newbie)
      vsu_wall("-=> %s banish%s %s.\n", p->name, plural_es(p), ban_name);
   else 
      vsu_wall("-=> %s banish%s the name %s.\n", p->name, plural_es(p), str);
}



/* Unbanish a player or name */

void unbanish(player *p, char *str)
{
   saved_player *sp;
   char *reason=0;

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," Ahem. Look at your flags. You are off_duty. Get back "
		   "on_duty if you wanna do that.\n");
       return;
     }
     
   if(str && *str) {
     reason = next_space(str);
     if(reason && *reason)
       *reason++=0;
   }
   
   if(!*str || !*reason) {
     tell_player(p, " Format: unbanish <player> <reason>\n");
     return;
   }

   lower_case(str);
   sp = find_saved_player(str);
   if (!sp)
   {
      tell_player(p, " Can't find saved player file for that name.\n");
      return;
   }
   if ( !(sp->residency & BANISHD) )
   {
      tell_player(p, " That player isn't banished!\n");
      return;
   }
   if ( sp->residency == BANISHD )
   {
      remove_player_file(str);
      vsu_wall("-=> %s unbanish%s the Name \'%s\'\n", p->name, plural_es(p), str);
      return;
   }
   sp->residency &= ~BANISHD;
   set_update(*str);
   sync_to_file(str[0], 0);
   vsu_wall("-=> %s unbanish%s the Player '%s' with reason: %s\n", p->name, plural_es(p), str, reason);
   vlog("banish", "%s unbanished player '%s' with reason: %s", p->name, str, reason);
}


/* create a new character */

void make_new_character(player * p, char *str)
{
   char *oldstack, *cpy, *email, *password=0;
   player *np;
   int length = 0;

   if (p->flags &BLOCK_SU)
     {
       tell_player(p," on_duty first please.\n");
       return;
     }

   oldstack = stack;
   /* chop the argument into "name\000email\000password\000" with ptrs as
   appropriate */
   email = next_space(str);
   if(email && *email) {
     *email++=0;
     password = next_space(email);
     if(password && *password)
       *password++=0;
   }

   if (!*str || !*email || !*password)
   {
      tell_player(p, " Format: make <character name> <email addr> "
                     "<password>\n");
      return;
   }

   for (cpy = str; *cpy; cpy++)
   {
      if (isalpha(*cpy))
      {
         *stack++ = *cpy;
         length++;
      }
   }
   *stack++ = 0;
   if (length > (MAX_NAME - 2))
   {
      tell_player(p, " Name too long.\n");
      stack = oldstack;
      return;
   }
   if((int)strlen(password)<3 || (int)strlen(password)>(MAX_PASSWORD-2)) {
      tell_player(p, " Invalid password (must be >3 character and not too long).\n");
      stack = oldstack;
      return;
   }
   if (find_saved_player(oldstack))
   {
      tell_player(p, " That player already exists.\n");
      stack = oldstack;
      return;
   }
   np = create_player();
   np->flags &= ~SCRIPTING;
   strcpy(np->script_file, "dummy");
   np->fd = p->fd;
   np->location = (room *) -1;

   restore_player(np, oldstack);
   np->flags &= ~SCRIPTING;
   strcpy(np->script_file, "dummy");
   strcpy (np->inet_addr, "NOT YET LOGGED ON");
   np->residency = get_flag(permission_list, "residency");
   np->saved_residency = np->residency;

   /* Crypt that password, why don't you */

   strcpy(np->password, do_crypt(password, np));

   /* strncpy(np->password,oldstack,(MAX_PASSWORD-2)); */

   strncpy(np->email, email, (MAX_EMAIL - 2));
   lower_case(np->email);
#ifdef DYNAMIC
   np->flags |= ARGH_DONT_CACHE_ME;
   save_player(np);
   np->flags &= ~ARGH_DONT_CACHE_ME;
#else
   save_player(np);
#endif
   np->fd = -1;
   np->location = 0;
   destroy_player(np);
   vlog("make", "%s creates %s.", p->name, oldstack);
   tell_player(p, " Player created.\n");
   stack = oldstack;
}


/* List the Super Users who're on */

void lsu(player * p, char *str)
{
   int count = 0;
   char *oldstack, *prestack, buffer[100];
   player *scan;

#ifdef INTERCOM
   if (str && *str =='@')
   {
     do_intercom_lsu(p, str);
     return;
   }
#endif
   
   oldstack = stack;
   titled_line(p, "Supers on");
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      prestack = stack;
      if (scan->residency & PSU && scan->location)
      {
         if ( (scan->flags & BLOCK_SU) && !(p->residency & PSU) )
            continue;
         count++;
         *stack = ' ';
         stack++;
         sprintf(stack, "%-20s", scan->name);
         stack = strchr(stack, 0);
         
         if (scan->saved_residency & ADMIN)
            strcpy(stack, "< Admin >       ");
         else if (scan->saved_residency & LOWER_ADMIN)
            strcpy(stack, "< Lower Admin > ");
         else if (scan->saved_residency & SU)
            strcpy(stack, "< Super User >  ");
/* It's a horrible kludge I know... */
         else if (!(p->residency & PSU))
         {
            count--;
            stack = prestack;
            continue;
         } else if (scan->saved_residency & PSU)
            strcpy(stack, "< Pseudo SU >   ");
         stack = strchr(stack, 0);
   
         if (scan->flags & BLOCK_SU)
            strcpy(stack, "                [  Off Duty   ]");
         else if ((scan->idle) < 300)
            strcpy(stack, "                [   Active    ]"); 
         else
            sprintf(stack, "                [%2d:%2d:%2d idle]", 
            ((scan->idle)/3600),(((scan->idle)/60)%60),(scan->idle%60));
         stack = strchr(stack, 0);
 
         *stack++ = '\n';
      }
   }
   if (count > 1) {
      sprintf(buffer, "There are %d Super Users connected", count);
      titled_line(p, buffer);
   }
   else if (count == 1)
      titled_line(p, "There is one Super User connected");
   else
      titled_line(p, "There are no Super Users connected");
   *stack++=0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* List the Newbies that're on */

void lnew(player * p, char *str)
{
   char *oldstack, buffer[100];
   int count = 0;
   player *scan;

   oldstack = stack;
   titled_line(p, "Newbies on");
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->residency == NON_RESIDENT && scan->location)
      {
         count++;
         sprintf(stack, "%-20s ", scan->name);
         stack = strchr(stack, 0);
         sprintf(stack, "%-40s ", scan->inet_addr);
         stack = strchr(stack, 0);
         if (scan->assisted_by[0] != '\0')
         {
            sprintf(stack, "[%s]", scan->assisted_by);
            stack = strchr(stack, 0);
         }
         *stack++ = '\n';
      }  
   }

   if (count > 1) {
      sprintf(buffer, "There are %d Newbies connected", count);
      titled_line(p, buffer);
   }
   else if (count == 1)
      titled_line(p, "There is one Newbie connected");
   else
      titled_line(p, "There are no Newbies connected");
   *stack++=0;
   
   if (count == 0)
      tell_player(p, " No Newbies on at the moment.\n");
   else
      tell_player(p, oldstack);
   stack = oldstack;
}


/*
 * rename a person (yeah, right, like this is going to work .... )
 * 
 */

void do_rename(player * p, char *str, int verbose)
{
   char *oldstack, *firspace, name[MAX_NAME + 2], *letter;
   char oldname[MAX_NAME+2];
   int hash;
   player *oldp, *scan, *previous;
   saved_player *sp;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," It would be in the best interests of humanity in "
		   "general, if you were to go on_duty to this mightily "
		   "important task.\n");
	 return;
     }

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: rename <person> <new-name>\n");
      return;
   }
   if (!(firspace = strchr(str, ' ')))
      return;
   *firspace = 0;
   firspace++;
   letter = firspace;
   if (!(oldp = find_player_global(str)))
      return;
   if (oldp->residency & BASE)
   {
      vtell_player(p, " But you cannot rename %s. They are a resident.\n", oldp->name);
      return;
   }
   strcpy(oldname,oldp->lower_name);
   scan = find_player_global_quiet(firspace);
   if (scan)
   {
      vtell_player(p, " There is already a person with the name '%s' "
                     "logged on.\n", scan->name);
      return;
   }
   strcpy(name, firspace);
   lower_case(name);
   sp = find_saved_player(name);
   if (sp)
   {
      vtell_player(p, " There is already a person with the name '%s' "
                     "in the player files.\n", sp->lower_name);
      return;
   }
   /* Test for a nice inputted name */

   if ((int)strlen(letter) > MAX_NAME - 2 || (int)strlen(letter) < 2)
   {
      tell_player(p, " Try picking a name of a decent length.\n");
      stack = oldstack;
      return;
   }
   while (*letter)
   {
      if (!isalpha(*letter))
      {
         tell_player(p, " Letters in names only, please ...\n");
         stack = oldstack;
         return;
      }
      letter++;
   }

   /* right, newname doesn't exist then, safe to make a change (I hope) */
   /* Remove oldp from hash list */

   scan = hashlist[oldp->hash_top];
   previous = 0;
   while (scan && scan != oldp)
   {
      previous = scan;
      scan = scan->hash_next;
   }
   if (!scan)
      log("error", "Bad hash list (rename)");
   else if (!previous)
      hashlist[oldp->hash_top] = oldp->hash_next;
   else
      previous->hash_next = oldp->hash_next;

   strcpy(name, oldp->lower_name);
   strncpy(oldp->name, firspace, MAX_NAME - 2);
   lower_case(firspace);
   strncpy(oldp->lower_name, firspace, MAX_NAME - 2);

   /* now place oldp back into named hashed lists */

   hash = ((int) (oldp->lower_name[0]) - (int) 'a' + 1);
   oldp->hash_next = hashlist[hash];
   hashlist[hash] = oldp;
   oldp->hash_top = hash;

   stack = oldstack;
   if (verbose)
   {
      sprintf(stack, " %s dissolves in front of your eyes, and "
                     "rematerialises as %s ...\n", name, oldp->name);
      stack = end_string(stack);

      /* tell room */
      scan = oldp->location->players_top;
      while (scan)
      {
        if (scan != oldp && scan != p)
           tell_player(scan, oldstack);
        scan = scan->room_next;
      }
      stack = oldstack;
      vtell_player(oldp, "\n-=> %s %s just changed your name to be '%s' ...\n\n",
         p->name, havehas(p), oldp->name);
   }
   tell_player(p, " Tis done ...\n");
   stack = oldstack;

   /* log it */
   vlog("rename", "Rename by %s - %s to %s", p->name, name, oldp->name);
   vsu_wall("-=> %s rename%s %s to %s.\n", p->name, single_s(p), name, oldp->name);
}


/* User interface to renaming a newbie */
void rename_player(player * p, char *str)
{
   do_rename(p, str, 1);
}


/* For an SU to go back on duty */
void on_duty(player * p, char *str)
{
   if ((p->flags & BLOCK_SU) != 0)
   {
      p->flags &= ~BLOCK_SU;
      tell_player(p, " You return to duty.\n");
      p->residency = p->saved_residency;
      vsu_wall_but(p, "-=> %s return%s to duty.\n", p->name, single_s(p));
      vlog("duty", "%s returned to duty", p->name);
   } 
   else
   {
      tell_player(p, " Are you asleep or something? You are ALREADY On Duty!"
                     " <smirk>\n");
   }
}


/* For an SU to go off duty */
void block_su(player * p, char *str)
{
   if ((p->flags & BLOCK_SU) == 0)
   {
      p->flags |= BLOCK_SU;
      tell_player(p, " You're now off duty ... Now everyone else can"
            " bitch about you on the SU channel ;)\n");
      /*I mean, why though,"
	" what the hell is the point of being a superuser if"
	" you're going to go off duty the whole time?\n");*/

      p->saved_residency = p->residency;
      if (p->gender==PLURAL)
	vsu_wall_but(p, "-=> The %s all go off duty.\n", p->name);
      else
	vsu_wall_but(p, "-=> %s goes off duty.\n", p->name);
      vlog("duty", "%s goes off duty", p->name);
   } 
   else
   {
      tell_player(p, " But you are ALREADY Off Duty! . o O ( D'Oh )\n");
   }
}


/* help for superusers */
void super_help(player * p, char *str)
{
   char *oldstack;
   file help;

   oldstack = stack;
   if (!*str || (!strcasecmp(str, "admin") && !(p->residency & ADMIN)))
   {
      tell_player(p, " SuperUser help files that you can read are: basic, "
                     "advanced.\n");
      return;
   }
   if (*str == '.')
   {
      tell_player(p, " Uh-uh, cant do that ...\n");
      return;
   }
   sprintf(stack, "files/doc/%s.doc", str);
   stack = end_string(stack);
   help = load_file_verbose(oldstack, 0);
   if (help.where)
   {
      if (*(help.where))
      {
         if (p->saved_flags & NO_PAGER)
            tell_player(p, help.where);
         else
            pager(p, help.where, 1);
      } else
      {
         tell_player(p, " Couldn't find that help file ...\n");
      }
      FREE(help.where);
   }
   stack = oldstack;
}


/* assist command */
void assist_player(player * p, char *str)
{
   player *p2, *p3;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first.\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: assist <person>\n");
      return;
   }
   if (!strcasecmp(str, "me"))
   {
      p2 = p;
   } else
   {
      p2 = find_player_global(str);
      if (!p2)
         return;
   }
   if (p != p2)
   {
      if (p2->residency != NON_RESIDENT)
      {
         tell_player(p, " That person isn't a newbie though ...\n");
         return;
      }
   }
   if (p2->flags & ASSISTED)
   {
      p3 = find_player_absolute_quiet(p2->assisted_by);
      if (p3)
      {
         if (p != p3)
            vtell_player(p, " That person is already assisted by %s.\n",
                    p2->assisted_by);
         else
            vtell_player(p, " That person has already been assisted by %s."
                           " Oh! That's you that is! *smirk*\n", p2->assisted_by);
         return;
      }
   }
   if (p!=p2)
   {
      p2->flags |= ASSISTED;
      strcpy(p2->assisted_by, p->name);
   }
   if (p->gender == PLURAL)
     vtell_player(p2, "\n-=> %s are superusers, and would be more than "
	     "happy to assist you in any problems you may have (including "
	     "gaining residency, type 'help residency' to find out more "
	     "about that).  To talk to %s, type 'tell %s <whatever>\', "
	     "short forms of names usually work as well.\n\n", p->name,
	     get_gender_string(p), p->lower_name);
   else
     vtell_player(p2, "\n-=> %s is a superuser, and would be more than "
	     "happy to assist you in any problems you may have (including "
	     "gaining residency, type 'help residency' to find out more "
	     "about that).  To talk to %s, type 'tell %s <whatever>\', "
	     "short forms of names usually work as well.\n\n", p->name,
	     get_gender_string(p), p->lower_name);
   if (p!=p2)
   {
      vsu_wall_but(p, "-=> %s assists %s.\n", p->name, p2->name);
      vtell_player(p, " You assist %s.\n", p2->name);
      vlog("resident", "%s assists %s", p->name, p2->name);
   }
}


/* Confirm if password and email are set on a resident */
void confirm_password(player * p, char *str)
{
   char *oldstack;
   player *p2;

   if (!*str)
   {
      tell_player(p, " Format: confirm <name>\n");
      return;
   }
   p2 = find_player_global(str);
   if (!p2)
      return;

   if (p2->residency == NON_RESIDENT)
   {
      tell_player(p, " That person is not a resident.\n");
      return;
   }
   oldstack = stack;

   p2->residency |= NO_SYNC;
   /* check email */
   if (p2->email[0]==0)
   {
      strcpy(stack, " Email has not been set.");
   } 
   else if (p2->email[0] == ' ' && p2->email[1]==0)
   {
      strcpy(stack, " Email validated set.");
      if(p2->password[0]!=0)
        p2->residency &= ~NO_SYNC;
   } 
   else if (!strstr(p2->email, "@") || !strstr(p2->email, "."))
   {
      strcpy(stack, " Probably not a correct email.");
      if(p2->password[0]!=0)
        p2->residency &= ~ NO_SYNC;
   } 
   else
   {
      strcpy(stack, " Email set.");
      if(p2->password[0]!=0)
        p2->residency &= ~ NO_SYNC;
   }
   stack = strchr(stack, 0);

   if (p2->email[0] && p2->email[0] != ' ')
   {
      if (p->residency & ADMIN || !(p2->saved_flags & PRIVATE_EMAIL))
      {
         sprintf(stack, " - %s", p2->email);
         stack = strchr(stack, 0);
         if (p2->saved_flags & PRIVATE_EMAIL)
         {
            strcpy(stack, " (private)\n");
         } else
         {
            strcpy(stack, "\n");
         }
      } else 
      {
         strcpy(stack, "\n");
      }
   } else
   {
      strcpy(stack, "\n");
   }
   stack = strchr(stack, 0);

   /* password */
   if (p2->password[0]!=0)
   {
      strcpy(stack, " Password set.\n");
   } 
   else
   {
      strcpy(stack, " Password NOT-set (won't save).\n");
      p2->residency |= NO_SYNC;
   }
   stack = strchr(stack, 0);

   if (p2->residency & NO_SYNC)
      sprintf(stack, " Character '%s' won't be saved.\n", p2->name);
   else
      sprintf(stack, " Character '%s' will be saved.\n", p2->name);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


/* reset email of a player */
/* this version even manages to check if they are logged in at the time :-/ */
/* leave the old one in a little while until we are sure this works */
void blank_email(player * p, char *str)
{
   player dummy;
   player *p2;
   char *space;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first.\n");
       return;
     }

   /* spot incorrect syntax */
   if (!*str)
   {
      tell_player(p, " Format: blank_email <player> <email>\n");
      return;
   }

   /* spot lack of sensible email address */
   space = 0;
   space = strchr(str, ' ');
   if (space != NULL)
   {
      *space++ = 0;
      if ((int)strlen(space) < 7)
      {
         tell_player(p, " Try a reasonable email address.\n");
         return;
      }
   }
   else 
   {
      tell_player(p, " Format: blank_email <player> <email>\n");
      return;
   }

   /* look for them on the prog */
   lower_case(str);
   p2 = find_player_absolute_quiet(str);

   /* if player logged in */
   if (p2)
   {
       /* no blanking the emails of superiors... */
       if (!check_privs(PRIVS(p), PRIVS(p2)) && !(p->residency & HCADMIN))
	   /* naughty, naughty, so tell the person, the target, and the
	      su channel */
       {
	   tell_player(p, " You cannot blank that person's email address.\n");
	   vtell_player(p2, "-=> %s tried to blank your email address, but "
		   "failed.\n", p->name);
	   vsu_wall_but(p, "-=> %s failed in an attempt to blank the email "
		   "address of %s.\n", p->name, p2->name);
	   return;
       }
       else
	   /* p is allowed to do things to p2 */
       {
	   /* tell the target and the SUs all about it */
	   if (space == NULL)
	       vtell_player(p2, "-=> Your email address has been blanked "
		       "by %s.\n", p->name);
	   else
	       vtell_player(p2, "-=> Your email address has been changed "
		       "by %s.\n", p->name);	       
	   if (space == NULL)
	       vsu_wall_but(p, "-=> %s %s their email blanked by %s.\n",
		       p2->name, havehas(p2), p->name);
	   else
	       vsu_wall_but(p, "-=> %s %s their email changed by %s.\n",
		       p2->name, havehas(p2), p->name);

	   /* actually blank it, and flag the player for update */
	   /* and avoid strcpy from NULL since it's very dodgy */
	   strcpy(p2->email, space);
	   lower_case(p2->email);
	   /* save, forcing copy in of saved_email */
	   sys_flags |= INVIS_SAVE;
	   save_player(p2);
	   sys_flags &= ~INVIS_SAVE;
	   set_update(*str);

	   /* report success to the player */
	   vtell_player(p, "-=> You successfully change %s's email.\n", p2->name);
	   /* log the change */
	   vlog("blanks", "%s changed %s's email address (logged in)", p->name, p2->name);
	   return;
       }
   }
   else
       /* they are not logged in, so load them */
       /* set up the name and port first */
   {
       strcpy(dummy.lower_name, str);
       dummy.fd = p->fd;
       if (load_player(&dummy))
       {
	   /* might as well point this out if it is so */
 	   if (dummy.residency & BANISHD)
	   {
	       tell_player(p, " By the way, this player is currently BANISHD.");
	       if (dummy.residency == BANISHD)
		   tell_player(p, " (Name Only)\n");
	       else
		   tell_player(p, "\n");
	   }
	   /* announce to the SU channel */
	   vsu_wall_but(p, "-=> %s changes the email of %s, who is logged out at the moment.\n", p->name, dummy.name);
	   /* change or blank the email address */
	   strcpy(dummy.email, space);
	   lower_case(dummy.email);

	   /* report success to player */
	   vtell_player(p, "-=> Successfully changed the email of %s, not logged in atm.\n", dummy.name);

	   /* and log it */
	   vlog("blanks", "%s changed %s's email address (logged out)", p->name, dummy.name);

	   /* save char LAST thing so maybe we won't blancmange the files */
	   dummy.script = 0;
	   dummy.script_file[0] = 0;
	   dummy.flags &= ~SCRIPTING;
	   dummy.location = (room *) -1;
	   save_player(&dummy);
	   return;
       }
       else
	   /* name does not exist, tell the person so and return */
       {
	   vtell_player(p, "-=> The name '%s' was not found in saved files.\n",
		   dummy.name);
	   return;
       }
   }
}


/* The almighty frog command!!!! */

void frog(player *p, char *str)
{
   player *d;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first. Next time you do this when off_duty "
		   "it will affect YOU instead!!\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: frog <player>\n");
      return;
   }
   if (!strcasecmp(str, "me"))
   {
      tell_player(p, " Do you REALLY want to frog yourself?\n");
      return;
   }
   d = find_player_global(str);
   if (d)
   {
      if (d == p)
      {
         tell_player(p, " Do you REALLY want to frog yourself?\n");
         return;
      }
      if (d->flags & FROGGED)
      {
         tell_player(p, " That player is ALREADY frogged!\n");
         return;
      }

      if (!check_privs(PRIVS(p), PRIVS(d)))
      {
         tell_player(p, " You can't do that!\n");
         vtell_player(d, "-=> %s tried to frog you!\n", p->name);
         return;
      }
      d->flags |= FROGGED;
      d->saved_flags |= SAVEDFROGGED;
      vtell_player(p, " You frog %s!\n", d->name);
      vtell_player(d, "-=> %s turn%s you into a frog!\n", p->name,single_s(p));
      vsu_wall_but(p, "-=> %s turn%s %s into a frog!\n", p->name,single_s(p), d->name);
      vlog("frog", "%s frogs %s", p->name, d->name);
   }
}


/* Well, I s'pose we'd better have this too */

void unfrog(player *p, char *str)
{
   player *d;
   saved_player *sp;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first. {:-P\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: unfrog <player>\n");
      return;
   }
   d = find_player_global(str);
   if (d)
   {
      if (d == p)
      {
         if (p->flags & FROGGED)
         {
            tell_player(p, " You can't, you spoon!\n");
	    if (p->gender==PLURAL)
	      vsu_wall_but(p, "-=> %s try to unfrog %s...\n", p->name,self_string(p));
	    else
	      vsu_wall_but(p, "-=> %s tries to unfrog %s...\n", p->name,self_string(p));
         } else
            tell_player(p, " But you're not frogged...\n");
         return;
      }
      if (!(d->flags & FROGGED))
      {
          tell_player(p, " That person isn't a frog...\n");
          return;
      }
      d->flags &= ~FROGGED;
      d->saved_flags &= ~SAVEDFROGGED;
      if (p->gender==PLURAL)
	vtell_player(d, "-=> The %s all kiss you and you are no longer a "
		"frog.\n",p->name);
      else
	vtell_player(d, "-=> %s kisses you and you are no longer a frog.\n",
		p->name);
      vtell_player(p, " You kiss %s and %s %s no longer a frog.\n", d->name,
              gstring(d),isare(d));
      if (p->gender==PLURAL)
        vsu_wall_but(p, "-=> The %s all kiss %s, and %s %s no longer a "
		"frog.\n",p->name, d->name, gstring(d), isare(d));
      else
	vsu_wall_but(p, "-=> %s kisses %s, and %s %s no longer a frog.\n",
		p->name, d->name, gstring(d), isare(d));
      vlog("frog", "%s unfrogged %s", p->name, d->name);
   } 
   else
   {
      tell_player(p, " Checking saved files...\n");
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Not found.\n");
         return;
      }
      if (!(sp->saved_flags & SAVEDFROGGED))
      {
         tell_player(p, " But that person isn't a frog...\n");
         return;
      }
      sp->saved_flags &= ~SAVEDFROGGED;
      vtell_player(p, " Ok, %s is no longer a frog.\n", sp->lower_name);
      vsu_wall_but(p, "-=> %s unfrog%s %s.\n", p->name,single_s(p), sp->lower_name);
   }
}


/* unconverse, get idiots out of converse mode */
void unconverse(player *p, char *str)
{
   player *p2;
   saved_player *sp;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first... Please..... Thanx.\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: unconverse <player>\n");
      return;
   }
   lower_case(str);
   p2 = find_player_global_quiet(str);
   if (!p2)
   {
      tell_player(p, " Player not logged on, checking saved player files...\n");
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Can't find saved player file.\n");
         return;
      }
      if (!(sp->residency & SU) && !(sp->residency & ADMIN))
      {
         if (!(sp->saved_flags & CONVERSE))
         {
            tell_player(p, " They aren't IN converse mode!!!\n");
            return;
         }
         sp->saved_flags &= ~CONVERSE;
         set_update(*str);
         vtell_player(p, " You take \'%s' out of converse mode.\n", sp->lower_name);
      } 
      else
         tell_player(p, " You can't do that to them!\n");
      return;
   }
   if (!(p2->saved_flags & CONVERSE))
   {
      tell_player(p, " But they're not in converse mode!!!\n");
      return;
   }
   if (!(p2->residency & SU) && !(p2->residency & ADMIN))
   {
      p2->saved_flags &= ~CONVERSE;
      p2->mode &= ~CONV;
      if (p->gender == PLURAL)
	vtell_player(p2, "-=> %s have taken you out of converse mode.\n",
		p->name);
      else
	vtell_player(p2, "-=> %s has taken you out of converse mode.\n",
		p->name);
      do_prompt(p2, p2->prompt);
      vtell_player(p, " You take %s out of converse mode.\n", p2->name);
   } 
   else
   {
      tell_player(p, " You can't do that to them!\n");
      vtell_player(p2, "-=> %s tried to unconverse you!\n", p->name);
   }
}


void unjail(player *p, char *str)
{
   player *p2, dummy;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first.\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: unjail <player>\n");
      return;
   }

   if (!strcasecmp(str, "me"))
      p2 = p;
   else
      p2 = find_player_global(str);
   if (!p2)
   {
      tell_player(p, " Checking saved files... ");
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " Not found.\n");
         return;
      } else
      {
         tell_player(p, "\n");
         p2 = &dummy;
         p2->location = (room *) -1;
      }
   }
   if (p2 == p)
   {
      if (p->location == prison)
      {
         tell_player(p, " You struggle to open the door, but to no avail.\n");
	 if (p->gender == PLURAL)
	   vsu_wall_but(p, "-=> %s try to unjail %s. *grin*\n", p->name,
		   self_string(p));
	 else
	   vsu_wall_but(p, "-=> %s tries to unjail %s. *grin*\n", p->name,
		   self_string(p));
      } 
      else
         tell_player(p, " But you're not in jail!\n");
      return;
   }

   if (p2 == &dummy)
   {
      if (!(p2->saved_flags & SAVEDJAIL))
      {
         tell_player(p, " Erm, how can I say this? They're not in jail...\n");
         return;
      }
   } else if (p2->jail_timeout == 0 || p2->location != prison)
   {
      tell_player(p, " Erm, how can I say this? They're not in jail...\n");
      return;
   }

   p2->jail_timeout = 0;
   p2->saved_flags &= ~SAVEDJAIL;
   if (p2 != &dummy)
   {
     command_type |= (HIGHLIGHT|PERSONAL|WARNING);
     if (p->gender== PLURAL)
       vtell_player(p2, "-=> The %s release you from prison.\n", p->name);
     else
       vtell_player(p2, "-=> %s releases you from prison.\n", p->name);
     command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
      move_to(p2, ENTRANCE_ROOM, 0);
   }
   
   vsu_wall("-=> The %s release%s %s from jail.\n", p->name, single_s(p), p2->name);
   vlog("jail","%s released %s", p->name, p2->name);
   if (p2 == &dummy)
      save_player(&dummy);
}


/* continuous scripting of a connection */
void script(player *p, char *str)
{
   char *oldstack;
   time_t t;

   if (p->flags & SCRIPTING)
   {
      if (!*str)
      {
         tell_player(p, " You are ALREADY scripting! ('script off' to turn"
                        " current scripting off)\n");
      }
      if (!strcasecmp(str, "off"))
      {
         p->flags &= ~SCRIPTING;
         vtell_player(p, "-=> Scripting stopped at %s\n", convert_time(time(0)));
         *(p->script_file)=0;
         vsu_wall("-=> %s has stopped continuous scripting.\n", p->name);
      }
      return;
   }

   if (!*str)
   {
      tell_player(p, " You must give a reason for starting scripting.\n");
      return;
   }
   p->flags |= SCRIPTING;
   vtell_player(p, "-=> Scripting started at %s, for reason \'%s\'\n",
           convert_time(time(0)), str);
   vsu_wall("-=> %s has started continuous scripting with reason "
                  "\'%s\'\n", p->name, str);
   t = time(0);
   oldstack = stack;
   strftime(stack, 16, "%d%m%y%H%M%S", localtime(&t));
   stack = end_string(stack);
   sprintf(p->script_file, "%s%s", p->name, oldstack);
   stack = oldstack;
   sprintf(stack, "logs/scripts/%s.log", p->script_file);
   stack = end_string(stack);
   unlink(oldstack);
   stack = oldstack;
}


/* cut down version of lsu() to just return number of SUs on */
int count_su(void)
{
  int count=0;
  player *scan;
 
  for (scan=flatlist_start;scan;scan=scan->flat_next)
    if (scan->residency&PSU && scan->location)
      count++;
 
  return count;
}
 

/* cut down version of lnew() to just return number of newbies on */
int count_newbies(void)
{
  int count=0;
  player *scan;

  for (scan=flatlist_start;scan;scan=scan->flat_next)
    if (scan->residency==NON_RESIDENT && scan->location)
      count++;
 
  return count;
}
 
/*
   Now that we know:
   the number of SUs,
   the number of newbies,
   and the number of ppl on the prog (current_players),
   we can output some stats
   */
 
void player_stats(player *p, char *str)
{
  tell_player(p,"Current Program/Player stats:\n");
  vtell_player(p," Players on program: %3d\n"
          "      Newbies on   : %3d\n"
          "      Supers on    : %3d\n"
          "      Normal res.  : %3d\n\n",
          current_players,
          count_newbies(),
          count_su(),
          (current_players-(count_su()+count_newbies())));
}


/* Go to the SUs study */
void go_comfy(player *p, char *str)
{
   command_type |= ADMIN_BARGE;
   if (p->location == comfy)
   {
      tell_player(p, " You're already in the study!\n");
      return;
   }
   if (p->no_move)
   {
      tell_player(p, " You seem to be stuck to the ground.\n");
      return;
   }
   move_to(p, "system.comfy", 0);
}


/* Tell you what mode someone is in */
void mode(player *p, char *str)
{
   player *p2;

   if (!*str)
   {
      tell_player(p, " Format: mode <player>\n");
      return;
   }

   p2 = find_player_global(str);
   if (!p2)
      return;

   if (p2->mode == NONE)
   {
      vtell_player(p, " %s is in no particular mode.\n", p2->name);
   } else if (p2->mode & PASSWORD)
   {
      vtell_player(p, " %s is in Password Mode.\n", p2->name);
   } else if (p2->mode & ROOMEDIT)
   {
      vtell_player(p, " %s is in Room Mode.\n", p2->name);
   } else if (p2->mode & MAILEDIT)
   {
      vtell_player(p, " %s is in Mail Mode.\n", p2->name);
   } else if (p2->mode & NEWSEDIT)
   {
      vtell_player(p, " %s is in News Mode.\n", p2->name);
   } else if (p2->mode & SNEWSEDIT)
   {
      vtell_player(p, " %s is in SuNews Mode.\n", p2->name);
   } else if (p2->mode & CONV)
   {
      vtell_player(p, " %s is in Converse Mode.\n", p2->name);
   } else
   {
      vtell_player(p, " Ermmm, %s doesn't appear to be in any mode at all.\n",
                 p2->name);
   }
}


/* the yoyo command! *grin* How'd I let myself get talked into this? */
void yoyo(player *p, char *str)
{
   player *p2;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," You attempt to use your yoyo, but the string gets "
		   "badly tangled and you accomplish nothing. Try again when "
		   "you are on_duty.\n");
       return;
     }

   if (!*str)
   {
      tell_player(p, " Format: yoyo <player>\n");
      return;
   }

   p2 = find_player_global(str);
   if (!p2)
   {
      vtell_player(p, " No-one of the name '%s' on at the moment.\n", str);
      return;
   }
   if (!check_privs(PRIVS(p), PRIVS(p2)))
   {
      vtell_player(p, " You try your best but end up nearly breaking your"
                     " finger trying to play yoyo with %s!\n", p2->name);
      vtell_player(p2, "-=> %s tried to play yoyo with you!\n", p->name);
      return;
   }
   if (p->gender==PLURAL)
     vsu_wall("-=> The %s all gang up and play yoyo with %s.\n", p->name, p2->name);
   else
     vsu_wall("-=> %s plays yoyo with %s.\n", p->name, p2->name);
   vlog("yoyo", "%s played yoyo with %s.", p->name, p2->name);
   command_type |= ADMIN_BARGE;
   current_player = p2;
   vtell_room(p2->location, " %s %s swung out of the room on a yoyo string,"
                  " by some superuser!\n", p2->name, isare(p2));
   trans_to(p2, "system.void");
   if (p->gender==PLURAL)
     vtell_room(p2->location, " The %s all swing through the room on a yoyo string!\n",
	     p2->name);
   else
     vtell_room(p2->location, " %s swings through the room on a yoyo string!\n",
	     p2->name);
   trans_to(p2, "system.prison");
   if (p->gender==PLURAL)
     vtell_room(p2->location, " The %s all swing through the room on a yoyo string!\n",
	     p2->name);
   else
     vtell_room(p2->location, " %s swings through the room on a yoyo string!\n",
	     p2->name);
   trans_to(p2, ENTRANCE_ROOM);
   if (p->gender==PLURAL)
     vtell_room(p2->location, " The %s all land back on the sand with a <THUMP> after an"
	   " exhausting trip.\n",p2->name);
   else
     vtell_room(p2->location, " %s lands back on the sand with a <THUMP> after an"
	   " exhausting trip.\n",p2->name);
   current_player = p;
   command_type |= (HIGHLIGHT|WARNING|PERSONAL);
   tell_player(p2, "-=> You just found out what it feels like to be a yoyo!\n");
   tell_player(p2, "-=> If you'd rather not have it happen again, behave.\n");
   command_type &= ~(HIGHLIGHT|WARNING|PERSONAL|ADMIN_BARGE);
}


void blank_prefix(player *p, char *str)
{
   player         *p2, dummy;

   if (p->flags & BLOCK_SU)
     {
       tell_player(p," Go on_duty first.\n");
       return;
     }

   if (*str)
     {
       p2 = find_player_absolute_quiet(str);
       if (p2)
	 {
           if (!check_privs(PRIVS(p), PRIVS(p2)))
	     {
               tell_player(p, " You can't do that to THAT person.\n");
               vsu_wall_but(p, "-=> %s tried to blank %s\'s prefix!\n",
                       p->name, p2->name);
               return;
	     }
           p2->pretitle[0] = 0;
           vtell_player(p2, "-=> %s has blanked your prefix.\n", p->name);
           vlog("blanks", "%s blanked %s's prefix.", p->name, p2->name);
           tell_player(p, " Blanked.\n");
           return;
	 }
       strcpy(dummy.lower_name, str);
       dummy.fd = p->fd;
       if (load_player(&dummy))
	 {
           if (!check_privs(PRIVS(p), dummy.residency))
	     {
               tell_player(p, " You can't do that to THAT person.\n");
               vsu_wall_but(p, "-=> %s tried to blank %s\'s prefix!\n",
                       p->name, dummy.name);
               return;
	     }
           dummy.pretitle[0] = 0;
           vlog("blanks", "%s blanked %s's prefix.", p->name, dummy.name);
           dummy.location = (room *) -1;
           save_player(&dummy);
           tell_player(p, " Blanked in saved file.\n");
           return;
	 }
       else
           tell_player(p, " Can't find that person on the program or in the "
                       "files.\n");
     }
   else
       tell_player(p, " Format: blank_prefix <player>\n");
}


void abort_shutdown(player *p,char *str)
{
  pulldown(p,"-1");
  return;
}


void echoall(player *p,char *str)
{
  char *oldstack;

  oldstack=stack;

  if ((int)strlen(str)<1)
    {
      tell_player(p,"Usage: echoall <message>\n");
      return;
    }

  sprintf(oldstack,"%s\n",str);
  stack=end_string(oldstack);
  raw_wall(oldstack);
  stack=oldstack;

  return;
}


void crash(player *p, char *str)
{
   char *flop = 0;
   
   *flop = -1;
}


