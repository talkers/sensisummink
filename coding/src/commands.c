/*
 * commands.c
 */
 
#include "include/config.h" 
 
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#if defined( LINUX ) && defined( GLIBC )
#define _XOPEN_SOURCE
#endif
#include <unistd.h>
#ifdef IRIX
#include <crypt.h>
#endif

#ifdef SUNOS
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"
#include "include/proto.h"

#ifdef ANSI_COLS
#include "include/colours.h"
#endif

/* birthday and age stuff */

void            set_age(player * p, char *str)
{
   int             new_age;

   if (!*str)
   {
      tell_player(p, " Format: age <number>\n");
      return;
   }
   new_age = atoi(str);
   if (new_age < 0)
   {
      tell_player(p, " You can't be of a negative age !\n");
      return;
   }
   p->age = new_age;
   if (p->age)
      vtell_player(p, " Your age is now set to %d years old.\n", p->age);
   else
      tell_player(p, " You have turned off your age so no-one can see it.\n");
}


/* set birthday */

void            set_birthday(player * p, char *str)
{
   struct tm       bday;
   struct tm      *tm_time;
   time_t          the_time;
   int             t;

   the_time = time(0);
   tm_time = localtime(&the_time);

   if (!*str)
   {
      tell_player(p, " Format: birthday <day>/<month>(/<year>)\n");
      return;
   }
   memset((char *) &bday, 0, sizeof(struct tm));
   bday.tm_year = tm_time->tm_year;
   bday.tm_mday = atoi(str);

   if (!bday.tm_mday)
   {
      tell_player(p, " Birthday cleared.\n");
      p->birthday = 0;
      return;
   }
   if (bday.tm_mday < 0 || bday.tm_mday > 31)
   {
      tell_player(p, " Not a valid day of the month.\n");
      return;
   }
   while (isdigit(*str))
      str++;
   str++;
   bday.tm_mon = atoi(str);
   if (bday.tm_mon <= 0 || bday.tm_mon > 12)
   {
      tell_player(p, " Not a valid month.\n");
      return;
   }
   bday.tm_mon--;

   while (isdigit(*str))
      str++;
   str++;
   while ((int)strlen(str) > 2)
      str++;
   bday.tm_year = atoi(str);
   if (bday.tm_year == 0)
   {
      bday.tm_year = tm_time->tm_year;
      p->birthday = TIMELOCAL(&bday);
   } else
   {
      p->birthday = TIMELOCAL(&bday);
      t = time(0) - (p->birthday);
      if (t > 0)
    p->age = t / 31536000;
   }

   vtell_player(p, " Your birthday is set to the %s.\n", birthday_string(p->birthday));
}


/* show what somone can do */

void            privs(player * p, char *str)
{
    char           *oldstack, name[MAX_NAME + 2];
    int             priv, who = 0;
    player         *p2=0;
    player          dummy;
    
    oldstack = stack;
    /* assume you are looking at your privs */
    strcpy(name, " You");

    /* convert possible name to lower case */
    lower_case(str);
    /* check if the person executing it is an SU */    
    if (*str && p->residency & (SU | ADMIN))
    {
	/* look for person on program - this will report if they
	   are logged out at the time (ie if it fails) */
	p2 = find_player_global(str);
	if (!p2)
	    /* if player is logged out, try and load them into dummy */
	{
	    /* setup name */
	    strcpy(dummy.lower_name, str);
	    lower_case(dummy.lower_name);
	    /* and an fd for messages */
	    dummy.fd = p->fd;
	    /* actually try loading them */
	    if (!load_player(&dummy))
		/* if we can't load them, report abject failure and exit */
	    {
		tell_player(p, " Couldn't find player in saved files.\n");
		return;
	    }
		/* otherwise set p2 so the gender stuff below works */
	    p2 = &dummy;
	}
	    /* setup name */
	    strcpy(name, p2->lower_name);
	    /* and privs */
	    priv = p2->residency;
	    /* flag it as another person's privs */
	    who = 1;
	    /* print the title to the stack */
	    sprintf(stack, " Permissions for %s.\n", name);
	    /* set the stack pointer to the end of the title */
	    stack = strchr(stack, 0);
    }
    else
	/* the person wants their own privs */
	/* so get person's own privs :-) */
	priv = p->residency;

	/* capitalise name again */
    name[0] = toupper(name[0]);
    
    if (priv == NON_RESIDENT)
    {
        stack = oldstack;
        vtell_player(p, "%s will not be saved... not a resident, you see..\n",
		name);
	return;
    }
    if (priv == STANDARD_ROOMS)
    {
        stack = oldstack;
	vtell_player(p, "%s is a System Room\n", name);
	return;
    }
    if (priv & BANISHD)
    {
        stack = oldstack;
	if (priv == BANISHD)
	    vtell_player(p, "%s has been banished ... (Name Only)\n", name);
	else 
	    vtell_player(p, "%s has been banished ...\n", name);
	return;
    }
    
    if (who == 0)
	/* privs for yourself */
    {
	if (priv & BASE)
	    strcpy(stack, " You are a resident.\n");
	else
	    strcpy(stack," You aren't a resident! EEK! Talk to a superuser!\n");
	stack=strchr(stack,0);
	
	if (priv & LIST)
	    strcpy(stack, " You have a list.\n");
	else
	    strcpy(stack, " You do not have a list.\n");
	stack = strchr(stack, 0);
	
	if (priv & ECHO_PRIV)
	    strcpy(stack, " You can echo.\n");
	else
	    strcpy(stack, " You cannot echo.\n");
	stack = strchr(stack, 0);
	
	if (priv & BUILD)
	    strcpy(stack, " You can use room commands.\n");
	else
	    strcpy(stack, " You can't use room commands.\n");
	stack = strchr(stack, 0);
	
	if (priv & MAIL)
	    strcpy(stack, " You can send mail.\n");
	else
	    strcpy(stack, " You cannot send any mail.\n");
	stack = strchr(stack, 0);
	
	if (priv & SESSION)
	    strcpy(stack, " You can set sessions.\n");
	else
	    strcpy(stack, " You cannot set sessions.\n");
	stack = strchr(stack, 0);
	
	if (priv & NO_TIMEOUT)
	{
	    strcpy(stack," You will never time-out.\n");
	    stack=strchr(stack,0);
	}

#ifdef ROBOTS
	if(priv & ROBOT_PRIV)
	{
	    strcpy(stack, " You are a robot.\n"); /* should never happen! */
	    stack = strchr(stack, 0);
	}
#endif
	
	if (priv & PSU)
	{
	    strcpy(stack," You can see the SU channel.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & WARN)
	{
	    strcpy(stack," You can warn people.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & SCRIPT)
	{
	    strcpy(stack," You can use extended scripting.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & TRACE)
	{
	    strcpy(stack," You can see login sites.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & SU && !(priv & (LOWER_ADMIN | ADMIN | HCADMIN)))
	{
	    strcpy(stack," You are a superuser.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & LOWER_ADMIN && !(priv & (ADMIN | HCADMIN)))
	{
	    strcpy(stack," You are a basic administrator.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & ADMIN && !(priv & HCADMIN))
	{
	    strcpy(stack," You are an administrator.\n");
	    stack=strchr(stack,0);
	}
	
	if (priv & HCADMIN)
	{
	    strcpy(stack," You are a hard-coded administrator.\n");
	    stack=strchr(stack,0);
	}
    }
    if (who == 1)
	/* privs for someone else */
    {
	if (priv & BASE)
	    sprintf(stack, "%s %s resident.\n",name, isare(p2));
	else
	    sprintf(stack,"%s %s not resident! EEK!\n",name, isare(p2));
	stack=strchr(stack,0);
	
	if (priv & LIST)
	    sprintf(stack, "%s %s a list.\n", name, havehas(p2));
	else
	    sprintf(stack, "%s %s no list.\n", name, havehas(p2));
	stack = strchr(stack, 0);
	
	if (priv & ECHO_PRIV)
	    sprintf(stack, "%s can echo.\n", name);
	else
	    sprintf(stack, "%s cannot echo.\n", name);
	stack = strchr(stack, 0);
	
	if (priv & BUILD)
	    sprintf(stack, "%s can use room commands.\n", name);
	else
	    sprintf(stack, "%s can't use room commands.\n", name);
	stack = strchr(stack, 0);
	
	if (priv & MAIL)
	    sprintf(stack, "%s can send mail.\n", name);
	else
	    sprintf(stack, "%s cannot send any mail.\n", name);
	stack = strchr(stack, 0);
	
	if (priv & SESSION)
	    sprintf(stack, "%s can set sessions.\n", name);
	else
	    sprintf(stack, "%s cannot set sessions.\n", name);
	stack = strchr(stack, 0);
	
	if (priv & NO_TIMEOUT)
	{
	    sprintf(stack,"%s will never time-out.\n",name);
	    stack=strchr(stack,0);
	}
	
#ifdef ROBOTS
	if(priv & ROBOT_PRIV) 
	{
	    switch(p2->gender)
	    {
	       case PLURAL:
	          sprintf(stack, "%s are robots.\n", name);
	          break;
	       default:
	          sprintf(stack, "%s is a robot.\n", name);
	    }
	    stack = strchr(stack, 0);
	}
#endif

	if (priv & PSU)
	{
	    sprintf(stack,"%s can see the SU channel.\n",name);
	    stack=strchr(stack,0);
	}
	
	if (priv & WARN)
	{
	    sprintf(stack,"%s can warn people.\n",name);
	    stack=strchr(stack,0);
	}
	
	if (priv & SCRIPT)
	{
	    sprintf(stack,"%s can use extended scripting.\n",name);
	    stack=strchr(stack,0);
	}
	
	if (priv & TRACE)
	{
	    sprintf(stack,"%s can see login sites.\n",name);
	    stack=strchr(stack,0);
	}
	
	if (priv & SU)
	{
	    switch (p2->gender)
	    {
	      case PLURAL:
		sprintf(stack,"%s are superusers.\n",name);
		stack=strchr(stack,0);
		break;
	      default:
		sprintf(stack,"%s is a superuser.\n",name);
		stack=strchr(stack,0);
		break;
	    }
	}
	if (priv & LOWER_ADMIN)
	{
	    switch (p2->gender)
	    {
	      case PLURAL:
		sprintf(stack,"%s are basic administrators.\n",name);
		stack=strchr(stack,0);
		break;
	      default:
		sprintf(stack,"%s is a basic administrator.\n",name);
		stack=strchr(stack,0);
		break;
	    }
	}
	
	if (priv & ADMIN)
	{
	    switch (p2->gender)
	    {
	      case PLURAL:
		sprintf(stack,"%s are administrators.\n",name);
		stack=strchr(stack,0);
		break;
	      default:
		sprintf(stack,"%s is an administrator.\n",name);
		stack=strchr(stack,0);
		break;
	    }
	}
	
	if (priv & HCADMIN)
	{
	    switch (p2->gender)
	    {
	      case PLURAL:
		sprintf(stack,"%s are hard-coded administrators.\n",name);
		stack=strchr(stack,0);
		break;
	      default:
		sprintf(stack,"%s is a hard-coded administrator.\n",name);
		stack=strchr(stack,0);
		break;
	    }
	}
    }
	
	/* finish off the end of the chunk of data */
    *stack++ = 0;
	/* and tell the player */
    tell_player(p, oldstack);
	/* reset the stack and exit */
    stack = oldstack;
}


/* recap someones name */

void recap(player * p, char *str)
{
   char *n;
   int found_lower;
   player *p2 = p;

   if (!*str)
   {
      tell_player(p, " Format: recap <name>\n");
      return;
   }
   if (strcasecmp(str, p->lower_name))
   {
      if (!(p->residency & SU))
      {
         tell_player(p, " But that isn't your name !!\n");
         return;
      }
      p2 = find_player_absolute_quiet(str);
      if (!p2)
      {
         tell_player(p, " No-one of that name on the program.\n");
         return;
      }
      if (!check_privs(PRIVS(p), PRIVS(p2)))
      {
         tell_player(p, " Nope, sorry, you can't!\n");
         return;
      }
   }
   found_lower = 0;
   n = str;
   while (*n) {
      if (*n >= 'a' && *n <= 'z')
         found_lower = 1;
      n++;
   }
   if (!found_lower) {
      n = str;
      n++;
      while (*n) {
         *n = *n - ('A' - 'a');
         n++;
      }
   }
   strcpy(p2->name, str);
   vtell_player(p, " Name changed to '%s'\n", p2->name);
}


void            friend_finger(player * p)
{
   char           *oldstack, *temp;
   list_ent       *l;
   saved_player   *sp;
   player          dummy, *p2;
   int             jettime, friend = 0;

   memset(&dummy, 0, sizeof(player));
   oldstack = stack;
   if (!p->saved)
   {
      tell_player(p, " You have no save information, and therefore no "
        "friends ...\n");
      return;
   }
   sp = p->saved;
   l = sp->list_top;
   if (!l)
   {
      tell_player(p, " You have no list ...\n");
      return;
   }
   strcpy(stack, "\n Your friends were last seen...\n");
   stack = strchr(stack, 0);
   do
   {
      if (l->flags & FRIEND && strcasecmp(l->name, "everyone"))
      {
         p2 = find_player_absolute_quiet(l->name);
         friend = 1;
         if (p2)
         {
            sprintf(stack, " %s is logged on.\n", p2->name);
            stack = strchr(stack, 0);
         } else
         {
            temp = stack;
            strcpy(temp, l->name);
            lower_case(temp);
            strcpy(dummy.lower_name, temp);
            dummy.fd = p->fd;
            if (load_player(&dummy))
            {
               switch (dummy.residency)
               {
                  case STANDARD_ROOMS:
                     sprintf(stack, " %s is a system room ...\n", dummy.name);
                     stack = strchr(stack, 0);
                     break;
                  default:
                     if (dummy.residency == BANISHD)
                     {
                        sprintf(stack, " %s is banished. (Name Only)\n",
                                dummy.lower_name);
                        stack = strchr(stack, 0);
                     } else if ( dummy.residency & BANISHD)
                     {
                        sprintf(stack, " %s is banished.\n", dummy.lower_name);
                        stack = strchr(stack, 0);
                     } else
                     {
                        if (dummy.saved)
                           jettime = dummy.saved->last_on + (p->jetlag * 3600);
                        else
                           jettime = dummy.saved->last_on;
                        sprintf(stack, " %s was last seen at %s.\n", dummy.name,
                                convert_time(jettime));
                        stack = strchr(stack, 0);
                    }
                    break;
               }
            } else
            {
               sprintf(stack, " %s doesn't exist.\n", l->name);
               stack = strchr(stack, 0);
            }
         }
      }
      l = l->next;
   } while (l);
   if (!friend)
   {
      tell_player(p, " But you have no friends !!\n");
      stack = oldstack;
      return;
   }
   stack = strchr(stack, 0);
   *stack++ = '\n';
   *stack++ = 0;
   pager(p, oldstack, 0);
   stack = oldstack;
   return;
}


/* finger command */
void finger(player * p, char *str)
{
   player dummy, *p2;
   char *oldstack;
   int jettime;

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: finger <player>\n");
      return;
   }

   if (!strcasecmp(str, "friends"))
   {
      friend_finger(p);
      return;
   }
   
#ifdef INTERCOM
   if(strchr(str, '@'))
   {
     do_intercom_finger(p, str);
     return;
   }
#endif

   if (!strcasecmp(str, "me"))
      p2 = p;
   else
   {
      p2 = find_player_absolute_quiet(str);
      if (!p2)
      {
         strcpy(dummy.lower_name, str);
         lower_case(dummy.lower_name);
         dummy.fd = p->fd;
         if (!load_player(&dummy))
         {
            tell_player(p, " No such person in saved files.\n");
            return;
         }
         p2 = &dummy;
      }
   }
   switch (p2->residency)
   {
      case STANDARD_ROOMS:
         tell_player(p, " That is where some of the standard rooms are stored."
                        "\n");
         return;
      default:
         if (p2->residency == BANISHD)
         {
            tell_player(p, " That name has been banished from this program.\n");
            return;
         } else if (p2->residency & BANISHD)
         {
            tell_player(p, " That player has been banished from"
                           " this program.\n");
            return;
         }
   }

   divider_line(p);
   sprintf(stack, "%s %s\n", p2->name, p2->title);
   stack = strchr(stack, 0);
   divider_line(p);

   if (p2->saved)
   {
      jettime = p2->saved->last_on + (p->jetlag * 3600);
   } else
   {
      jettime = 0;
   }
   if (p2 != &dummy)
   {
      sprintf(stack, "%s %s been logged in for %s since\n%s.\n",
              full_name(p2), havehas(p2), word_time(time(0) - (p2->on_since)),
              convert_time(p2->on_since));
   } else if (p2->saved)
   {
      if (p->jetlag)
      {
         sprintf(stack, "%s %s last seen at %s. (Your time)\n",
                 p2->name, waswere(p2), convert_time(jettime));
      } else
      {
         sprintf(stack, "%s %s last seen at %s.\n", p2->name,
                 waswere(p2), convert_time(p2->saved->last_on));
      }
   }
   stack = strchr(stack, 0);

   sprintf(stack, "%s total login time is %s.\n", caps(gstring_possessive(p2)),
      word_time(p2->total_login));
   stack = strchr(stack, 0);

   if (p2->age && p2->birthday)
   {
      sprintf(stack, "%s %s %d years old and %s birthday %s on the %s.\n",
         p2->name, isare(p2), p2->age, gstring_possessive(p2),isare(p2),
         birthday_string(p2->birthday));
   }
   if (p2->age && !(p2->birthday))
   {
      sprintf(stack, "%s %s %d years old.\n", p2->name, isare(p2), p2->age);
   }
   if (!(p2->age) && p2->birthday)
   {
      sprintf(stack, "%s birthday is on the %s.\n",
              caps(gstring_possessive(p2)), birthday_string(p2->birthday));
   }
   stack = strchr(stack, 0);

   if (new_mail_check(p2))
   {
      sprintf(stack, "%s %s new mail.\n", caps(gstring(p2)),havehas(p2));
      stack = strchr(stack, 0);
   }
   if (((p->residency & (ADMIN|LOWER_ADMIN) && p2->saved_flags & PRIVATE_EMAIL)
          || !(p2->saved_flags & PRIVATE_EMAIL)) && p2->email[0] != ' ')
   {
      if(p2->email[0]==0)
        sprintf(stack, "%s email address is ... Unset\n", caps(gstring_possessive(p2)));
      else
        sprintf(stack, "%s email address is ... %s\n", caps(gstring_possessive(p2)), p2->email);
      if (p2->saved_flags & PRIVATE_EMAIL)
      {
         while ( *stack != '\n' )
            stack++;
         strcpy(stack, " (private)\n");
      }
      stack = strchr(stack, 0);
   } 
   else if (p->residency & (ADMIN|LOWER_ADMIN) && p2->email[0] == ' ')
   {
      sprintf(stack, "Email validated set.\n");
      stack = strchr(stack, 0);
   }
   if (p2->plan[0])
   {
      pstack_mid("plan");
      sprintf(stack, "%s\n", p2->plan);
      stack = strchr(stack, 0);
   }
   divider_line(p);
   *stack++=0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* emergency command */
void            emergency(player * p, char *str)
{
   char           *oldstack;
   oldstack = stack;

   if (p->script)
   {
      if (!strcasecmp(str, "off") || !strcasecmp(str, "stop"))
      {
         vtell_player(p, " Time is now %s.\n Scripting stopped at your request.\n",
                convert_time(time(0)));
         p->script = 0;
         return;
      }
      tell_player(p, " You are already scripting ... use 'emergency stop' to "
        "stop.\n");
      return;
   }
   if (!*str)
   {
      tell_player(p, " You must give a reason for starting emergency scripting"
        " as an argument.\n"
        " (And the reason better be good ...)\n");
      return;
   } else if (!strcasecmp(str, "stop"))
   {
      tell_player(p, " OK, stop being silly, you hadn't yet STARTED emergency"
                      " scripting. Read 'help emergency' to learn how to use"
                      " 'emergency' properly.\n");
      return;
   }
   command_type = PERSONAL | EMERGENCY;
   sprintf(stack, "logs/emergency/%s_emergency.log", p->lower_name);
   unlink(stack);
   stack = oldstack;
   p->script = 60;
   vtell_player(p, " Emergency scripting started for 60 seconds.\n"
      " Remember, any previous scripts will be deleted\n"
      " Reason given : %s\n"
      " Time is now %s.\n", str, convert_time(time(0)));
   vsu_wall("-=> %s has started emergency scripting with reason "
                     "\'%s\'.\n", p->name, str);
}


/* see the time */
void            view_time(player * p, char *str)
{
   if (p->jetlag)
      vtell_player(p, " Your local time is %s.\n"
         " %s time is %s.\n"
         " The program has been up for %s.\n That is from %s.\n"
         " Total number of logins in that time is %s.\n",
         time_diff(p->jetlag), TALKER_NAME, sys_time(), word_time(time(0) - up_date),
         convert_time(up_date), number2string(logins));
   else
      vtell_player(p, " %s time is %s.\n"
         " The program has been up for %s.\n That is from %s.\n"
         " Total number of logins in that time is %s.\n",
         TALKER_NAME, sys_time(), word_time(time(0) - up_date),
         convert_time(up_date), number2string(logins));
}


/* go quiet */
void            go_quiet(player * p, char *str)
{
   earmuffs(p, str);
   blocktells(p, str);
}


/* the examine command */
void            examine(player * p, char *str)
{
   player         *p2;
   char           *oldstack;

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: examine <player>\n");
      return;
   }
   
#ifdef INTERCOM
   if (strchr(str, '@'))
   {
     do_intercom_examine(p, str);
     return;
   }
#endif
   
   if (!strcasecmp("me", str))
      p2 = p;
   else
      p2 = find_player_global(str);
   if (!p2)
      return;
   divider_line(p);
   sprintf(stack, "%s %s\n", p2->name, p2->title);
   stack = strchr(stack, 0);
   divider_line(p);
   if (p2->description[0])
   {
      sprintf(stack, "%s\n", p2->description);
      stack = strchr(stack, 0);
      divider_line(p);
   }
   if (p->jetlag)
      sprintf(stack, "%s %s been logged in for %s\n"
                     "That is from %s. (Your time)\n",
              full_name(p2), havehas(p2), word_time(time(0) - (p2->on_since)),
              convert_time((p2->on_since + (p->jetlag * 3600))));
   else
      sprintf(stack, "%s %s been logged in for %s\nThat is from %s.\n",
         full_name(p2), havehas(p2), word_time(time(0) - (p2->on_since)),
         convert_time(p2->on_since));

   stack = strchr(stack, 0);
   sprintf(stack, "%s total login time is %s.\n", caps(gstring_possessive(p2)),
      word_time(p2->total_login));
   stack = strchr(stack, 0);
   if (p2->saved_flags & BLOCK_TELLS && p2->saved_flags & BLOCK_SHOUT)
      sprintf(stack, "%s %s ignoring shouts and tells.\n", caps(gstring(p2)),
	      isare(p2));
   else
   {
      if (p2->saved_flags & BLOCK_TELLS)
         sprintf(stack, "%s %s ignoring tells.\n", caps(gstring(p2)),
		 isare(p2));
      if (p2->saved_flags & BLOCK_SHOUT)
         sprintf(stack, "%s %s ignoring shouts.\n", caps(gstring(p2)),
		 isare(p2));
   }
   stack = strchr(stack, 0);

   if (p2->age && p2->birthday)
      sprintf(stack, "%s %s %d years old and %s birthday is on the %s.\n",
        p2->name, isare(p2), p2->age, gstring_possessive(p2),
         birthday_string(p2->birthday));
   if (p2->age && !(p2->birthday))
      sprintf(stack, "%s %s %d years old.\n", p2->name, isare(p2), p2->age);
   if (!(p2->age) && p2->birthday)
      sprintf(stack, "%s birthday %s on the %s.\n",
         caps(gstring_possessive(p2)), isare(p2),
	      birthday_string(p2->birthday));
   stack = strchr(stack, 0);

   if (p2->residency != NON_RESIDENT && (!(p2->saved_flags & PRIVATE_EMAIL)
                || p->residency & (ADMIN|LOWER_ADMIN) || p == p2))
   {
      if (p2->email[0] == 0)
         strcpy(stack, "No email address set.\n");
      else if (p2->email[0] == ' ')
         sprintf(stack, "Email validated set.\n");
      else
         sprintf(stack, "Email: %s\n", p2->email);
      if (p2->saved_flags & PRIVATE_EMAIL)
      {
         while ( *stack != '\n' )
            stack++;
         strcpy(stack, " (private)\n");
      }
      stack = strchr(stack, 0);
   }
   divider_line(p);
   if (p2 == p || p->residency & ADMIN)
   {
      sprintf(stack, "Your entermsg is set to ...\n%s %s\n",
         p2->name, p2->enter_msg);
      stack = strchr(stack, 0);
      divider_line(p);
   }
   if ((p2 == p || p->residency & ADMIN) && p2->residency & BASE)
   {
      if ((int)strlen(p2->ignore_msg) > 0)
         sprintf(stack, "Your ignoremsg is set to ...\n%s\n", p2->ignore_msg);
      else
         strcpy(stack, "You have set no ignoremsg.\n");
      stack = strchr(stack, 0);
      divider_line(p);
   }
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* the idle command */
void check_idle(player * p, char *str)
{
   player *scan, **list, **step;
   int i,n;
   char *oldstack, middle[80], namestring[40], *id;
   file *is_scan;
   int page, pages, count, not_idle = 0;

#ifdef INTERCOM
   if (str && *str && strchr(str, '@'))
   {
     do_intercom_idle(p, str);
     return;
   }
#endif

   oldstack = stack;
   command_type |= SEE_ERROR;
   if (isalpha(*str) && !strstr(str, "everyone"))
   {
      align(stack);
      list = (player **) ALFIX stack;
      n = global_tag(p, str, 1);
      if (!n)
      {
         stack = oldstack;
         return;
      }
      id = stack;
      for (step = list, i = 0; i < n; i++, step++)
      {
         if (p->saved_flags & NOPREFIX)
         {
            strcpy(namestring, (*step)->name);
         } else if (*((*step)->pretitle))
         {
            sprintf(namestring, "%s %s", (*step)->pretitle, (*step)->name);
         } else
         {
            strcpy(namestring, (*step)->name);
         }
         if (!(*step)->idle)
         {
            sprintf(stack, "%s has just hit return.\n", namestring);
         } else
         {
            if ((*step)->idle_msg[0])
            {
               sprintf(stack, "%s %s\n%s %s %s idle\n", namestring,
                       (*step)->idle_msg, caps(gstring((*step))),
		       isare(*step),
                       word_time((*step)->idle));
            } else
            {
               sprintf(stack, "%s %s %s idle.\n", namestring,isare(*step),
                       word_time((*step)->idle));
            }
         }
         stack = end_string(stack);
         tell_player(p, id);
         stack = id;
      }
      cleanup_tag(list, n);
      stack = oldstack;
      return;
   }
   if (strstr(str, "everyone"))
   {
      id = str;
      str = strchr(str, ' ');
      if (!str)
      {
         str = id;
         *str++ = '1';
         *str = 0;
      }
   }

   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;

   pages = (current_players - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan->name[0] && scan->location && (scan->idle) < 300)
         not_idle++;

   if (current_players == 1)
   {
      strcpy(middle, "There is only you on the program at the moment");
   } else if (not_idle == 1)
   {
      sprintf(middle, "There are %d people here, only one of whom "
                      "appears to be awake", current_players);
   } else
   {
      sprintf(middle, "There are %d people here, %d of which appear "
                      "to be awake", current_players, not_idle);
   }
   pstack_mid(middle);

   count = page * (TERM_LINES - 2);
   for (scan = flatlist_start; count; scan = scan->flat_next)
   {
      if (!scan)
      {
         tell_player(p, " Bad idle listing, abort.\n");
         log("error", "Bad idle list");
         stack = oldstack;
         return;
      } else if (scan->name[0])
      {
         count--;
      }
   }

   for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
   {
      if (scan->name[0] && scan->location)
      {
         if (p->saved_flags & NOPREFIX)
         {
            strcpy(namestring, scan->name);
         } else if ((*scan->pretitle))
         {
            sprintf(namestring, "%s %s", scan->pretitle, scan->name);
         } else
         {
            sprintf(namestring, "%s", scan->name);
         }
      } else
         continue;
      if (scan->idle_msg[0])
      {
         sprintf(stack, "%s %s\n", namestring, scan->idle_msg);
      } else
      {
         for (is_scan = idle_string_list; is_scan->where; is_scan++)
         {
            if (is_scan->length >= scan->idle)
               break;
         }
         if (!is_scan->where)
            is_scan--;
         sprintf(stack, "%s %s", namestring, is_scan->where);
      }
      stack = strchr(stack, 0);
      count++;
   }
   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   pstack_mid(middle);

   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}



/* converse mode on and off */
void            converse_mode_on(player * p, char *str)
{
   if (p->saved_flags & CONVERSE)
   {
      tell_player(p, " But you are already in converse mode!\n");
      return;
   }
   p->saved_flags |= CONVERSE;
   tell_player(p, " Entering 'converse' mode. Everything you type will get"
                  " said.\n"
                  " Start the line with '/' to use normal commands, and /end"
                  " to\n"
                  " leave this mode.\n");
   p->mode |= CONV;
   return;
}


void            converse_mode_off(player * p, char *str)
{
   if (!(p->saved_flags & CONVERSE))
   {
      tell_player(p, " But you are not in converse mode !\n");
      return;
   }
   p->saved_flags &= ~CONVERSE;
   p->mode &= ~CONV;
   tell_player(p, " Ending converse mode.\n");
   return;
}




/* set things */
void            set_idle_msg(player * p, char *str)
{
   if (*str)
   {
      strncpy(p->idle_msg, str, MAX_TITLE - 2);
      if (p->saved_flags & NOPREFIX)
         vtell_player(p, " Idle message set to ....\n%s %s\nuntil you type a"
            " new command.\n", p->name, p->idle_msg);
      else
         vtell_player(p, " Idle message set to ....\n%s %s\nuntil you type a"
            " new command.\n", full_name(p), p->idle_msg);
   }
   else
      tell_player(p, " Please set an idlemsg of a greater than zero length.\n");
}


void            set_enter_msg(player * p, char *str)
{
   strncpy(p->enter_msg, str, MAX_ENTER_MSG - 2);
   vtell_player(p, " This is what people will see when you enter the "
      "room.\n%s %s\n", p->name, p->enter_msg);
}


void            set_title(player * p, char *str)
{
   strncpy(p->title, str, MAX_TITLE - 2);
   vtell_player(p, " You change your title so that now you are known as "
      "...\n%s %s\n", p->name, p->title);
}


void            set_pretitle(player * p, char *str)
{
   char *scan;
   
   if (strstr(str, "+[") != NULL)
   {
      tell_player(p, " You may not have \"+[\" in your prefix.\n");
      return;
   }

   for (scan = str; *scan; scan++)
   {
      switch (*scan)
      {
         case ' ':
            tell_player(p, " You may not have spaces in your prefix.\n");
            return;
            break;
         case ',':
            tell_player(p, " You may not have commas in your prefix.\n");
            return;
            break;
      }
   }

   if (find_saved_player(str) || find_player_absolute_quiet(str))
   {
      tell_player(p, " That is the name of a player, so you can't use that "
                     "for a prefix.\n");
      return;
   }

   strncpy(p->pretitle, str, MAX_PRETITLE - 2);
   vtell_player(p, " You change your pretitle so you become: %s %s\n",
           p->pretitle, p->name);
}


void            set_description(player * p, char *str)
{
   strncpy(p->description, str, MAX_DESC - 2);
   vtell_player(p, " You change your description to read...\n%s\n",
           p->description);
}


void            set_plan(player * p, char *str)
{
   strncpy(p->plan, str, MAX_PLAN - 2);
   vtell_player(p, " You set your plan to read ...\n%s\n", p->plan);
}


void            set_prompt(player * p, char *str)
{
   strncpy(p->prompt, str, MAX_PROMPT - 2);
   vtell_player(p, " You change your prompt to %s\n", p->prompt);
}


void            set_converse_prompt(player * p, char *str)
{
   strncpy(p->converse_prompt, str, MAX_PROMPT - 2);
   vtell_player(p, " You change your converse prompt to %s\n",
           p->converse_prompt);
}


void            set_term_width(player * p, char *str)
{
   int             n;

   if (!strcasecmp("off", str))
   {
      tell_player(p, " Linewrap turned off.\n");
      p->term_width = 0;
      return;
   }
   n = atoi(str);
   if (!n)
   {
      tell_player(p, " Format: linewrap off/<terminal_width>\n");
      return;
   }
   if (n <= ((p->word_wrap) << 1))
   {
      tell_player(p, " Can't set terminal width that small compared to "
                     "word wrap.\n");
      return;
   }
   if (n < 10)
   {
      tell_player(p, " Nah, you haven't got a terminal so small !!\n");
      return;
   }
   if (n>255) {
     tell_player(p, " Linewrap must not exceed 255 characters.\n");
     return;
   }
   p->term_width = n;
   vtell_player(p, " Linewrap set on, with terminal width %d.\n", p->term_width);
}


void            set_word_wrap(player * p, char *str)
{
   int             n;

   if (!strcasecmp("off", str))
   {
      tell_player(p, " Wordwrap turned off.\n");
      p->word_wrap = 0;
      return;
   }
   n = atoi(str);
   if (!n)
   {
      tell_player(p, " Format: wordwrap off/<max_word_size>\n");
      return;
   }
   if (n >= ((p->term_width) >> 1))
   {
      tell_player(p, " Can't set max word length that big compared to term "
                     "width.\n");
      return;
   }
   p->word_wrap = n;
   vtell_player(p, " Wordwrap set on, with max word size set to %d.\n",
           p->word_wrap);
}


/* quit from the program */
void            quit(player * p, char *str)
{
   p->flags |= CHUCKOUT;

   if (!str) 
      p->flags |= PANIC;
   else if(p->residency & BASE && (p->email[0]==0 || p->password[0]==0) && !(p->flags & TRIED_QUIT)) {
      /* if they are resident, with no email address or password... */
      command_type |= (HIGHLIGHT|PERSONAL|WARN);
      tell_player(p, "\007\n");
      tell_player(p,   "-=> Meep!  You have residency, but:      <=-\n");
      if(p->email[0]==0)
        tell_player(p, "-=> You have not set an email address.   <=-\n");
      if(p->password[0]==0)
        tell_player(p, "-=> You have not set a password.         <=-\n");
      tell_player(p, "-=> Until this is fixed, you won't save! <=-\n");
      tell_player(p, "-=>                                      <=-\n");
      tell_player(p, "-=> Simply type 'quit' again to abandon. <=-\n\n");
      command_type &= ~(HIGHLIGHT|PERSONAL|WARN);
      p->flags &= ~CHUCKOUT;
      p->flags |= TRIED_QUIT;
   }
   else
#ifdef ANSI_COLS
      vtell_player(p, "%s\n", NOR);
#else
      tell_player(p, "\n");
#endif

   /* Clean up lists */

   if (p->residency == NON_RESIDENT)
      check_list_newbie(p->lower_name);
   else
      check_list_resident(p);

   if ((p->logged_in == 1) && (sess_name[0] != '\0'))
      if (!strcasecmp(p->name, sess_name))
         session_reset = 0;
}


/* command to change gender */
void            gender(player * p, char *str)
{
   *str = tolower(*str);
   switch (*str)
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
         tell_player(p, " Gender set to Plural.\n");
         break;
      case 'n':
         p->gender = OTHER;
         tell_player(p, " Gender set to well, erm, something.\n");
         break;
      default:
         tell_player(p, " No gender set, Format : gender m/f/p/n\n");
         break;
   }
}


/* say command */
void            say(player * p, char *str)
{
   char           *oldstack, *mid, *scan, *piped, *prepipe;
   player         *s;
   char *temp, msg[]="Ribbet";

   oldstack = stack;
   command_type = ROOM;
   temp = str;

   if (!*str)
   {
      tell_player(p, " Format: say <msg>\n");
      return;
   }

   if ((p->flags & FROGGED) && (p->location != prison))
      str = (char *)msg;

#ifdef INTERCOM
#ifdef INTERCOM_EXT
   if(p->location && p->location==intercom_room)
   {
     do_intercom_say(p, str);
     return;
   }
#endif
#endif

   extract_pipe_local(str);
   if (sys_flags & FAILED_COMMAND)
   {
      sys_flags &= ~FAILED_COMMAND;
      str = temp;
      return;
   }
   for (scan = str; *scan; scan++);
   if (p->gender!=PLURAL)
     {
       switch (*(--scan))
	 {
	 case '?':
	   mid = "asks";
	   break;
	 case '!':
	   mid = "exclaims";
	   break;
	 default:
	   mid = "says";
	   break;
	 }
     }
   else
     {
       switch (*(--scan))
	 {
	 case '?':
	   mid = "ask";
	   break;
	 case '!':
	   mid = "exclaim";
	   break;
	 default:
	   mid = "say";
	   break;
	 }
     }
     
   

   for (s = p->location->players_top; s; s = s->room_next)
   {
      if (s != current_player)
      {
         prepipe = stack;
         piped = do_pipe(s, str);
         if (!piped)
         {
            cleanup_tag(pipe_list, pipe_matches);
            stack = oldstack;
            str = temp;
            return;
         }
         if (s->saved_flags & NOPREFIX)
            vtell_player(s, "%s %s '%s'\n", p->name, mid, piped);
         else
            vtell_player(s, "%s %s '%s'\n", full_name(p), mid, piped);
         stack = prepipe;
      }
   }
   piped = do_pipe(p, str);
   if (!piped)
   {
      cleanup_tag(pipe_list, pipe_matches);
      stack = oldstack;
      str = temp;
      return;
   }
   switch (*scan)
   {
      case '?':
         mid = "ask";
         break;
      case '!':
         mid = "exclaim";
         break;
      default:
         mid = "say";
         break;
   }
   vtell_player(p, " You %s '%s'\n", mid, piped);
   cleanup_tag(pipe_list, pipe_matches);
   stack = oldstack;
   sys_flags &= ~PIPE;
   str = temp;
}


/* Count  !!s and caps in a str */
int             count_caps(char *str)
{
   int             count = -2;
   char           *mark;

   for (mark = str; *mark; mark++)
      if (isupper(*mark) || *mark == '!')
         count++;
   return (count > 0 ? count : 0);
}


/* shout command */
void shout(player * p, char *str)
{
   char *oldstack, *mid, *scan, *piped, *prepipe;
   player *s;
   char *temp, msg[] = "RIBBET";

   oldstack = stack;
   temp = str;
   command_type = EVERYONE;

   if (!*str)
   {
      tell_player(p, " Format: shout <msg>\n");
      return;
   }
   if ( p->flags & FROGGED )
   {
      str = (char *) msg;
   }
   if (p->saved_flags & BLOCK_SHOUT)
   {
      tell_player(p, " You can't shout whilst ignoring shouts yourself.\n");
      str = temp;
      return;
   }
   if (p->location == colony)
   {
      tell_player(p, " You cannot shout whilst in the nudist's colony.\n");
      return;
   }

   if (p->no_shout || p->saved_flags & SAVENOSHOUT)
   {
      if (p->no_shout > 0)
         vtell_player(p, " You have been prevented from shouting for the "
                        "next %s.\n", word_time(p->no_shout));
      else if (p->no_shout == -1 || p->saved_flags & SAVENOSHOUT)
         tell_player(p, " You have been prevented from shouting for the "
                       "forseeable future.\n");
      str = temp;
      return;
   }
   if (p->shout_index > 60 || strstr(str, "SFSU") || strstr(str, "S F S U"))
   {
      tell_player(p, " You seem to have gotten a sore throat by shouting too"
                     " much!\n");
      str = temp;
      return;
   }

   if (!(p->residency & PSU))
      p->shout_index += (count_caps(str) * 2) + 20;

   extract_pipe_global(str);
   if (sys_flags & FAILED_COMMAND)
   {
      sys_flags &= ~FAILED_COMMAND;
      stack = oldstack;
      str = temp;
      return;
   }
   for (scan = str; *scan; scan++);
   switch (*(--scan))
   {
      case '?':
         mid = "shouts, asking";
         break;
      case '!':
         mid = "yells";
         break;
      default:
         mid = "shouts";
         break;
   }

   for (s = flatlist_start; s; s = s->flat_next)
   {
      if (s != current_player)
      {
         prepipe = stack;
         piped = do_pipe(s, str);
         if (!piped)
         {
            cleanup_tag(pipe_list, pipe_matches);
            stack = oldstack;
            str = temp;
            return;
         }
         if (s->saved_flags & NOPREFIX)
            vtell_player(s, "%s %s '%s'\n", p->name, mid, piped);
         else
            vtell_player(s, "%s %s '%s'\n", full_name(p), mid, piped);
         stack = prepipe;
      }
   }
   piped = do_pipe(p, str);
   if (!piped)
   {
      cleanup_tag(pipe_list, pipe_matches);
      stack = oldstack;
      str = temp;
      return;
   }
   switch (*scan)
   {
      case '?':
         mid = "shout, asking";
         break;
      case '!':
         mid = "yell";
         break;
      default:
         mid = "shout";
         break;
   }
   vtell_player(p, " You %s '%s'\n", mid, piped);
   cleanup_tag(pipe_list, pipe_matches);
   stack = oldstack;
   sys_flags &= ~PIPE;
   str = temp;
}


/* emote command */
void            emote(player * p, char *str)
{
  char           *oldstack, *piped, *prepipe;
  player         *s;
  char            tname[MAX_PRETITLE + MAX_NAME + 3];
  char *temp, msg[]="croaks";
  
  temp = str;
  oldstack = stack;
  
  command_type = ROOM;
  
  if (!*str)
    {
      tell_player(p, " Format: emote <msg>\n");
      return;
    }
  if ((p->flags & FROGGED) && (p->location != prison))
    str = (char *)msg;
    
#ifdef INTERCOM
#ifdef INTERCOM_EXT
  if (p->location && p->location==intercom_room)
  {
    do_intercom_emote(p, str);
    return;
  }
#endif
#endif

  extract_pipe_local(str);
  if (sys_flags & FAILED_COMMAND)
    {
      sys_flags &= ~FAILED_COMMAND;
      str = temp;
      return;
    }
  for (s = p->location->players_top; s; s = s->room_next)
    if (s != current_player)
      {
	prepipe = stack;
	piped = do_pipe(s, str);
	if (!piped)
	  {
	    cleanup_tag(pipe_list, pipe_matches);
	    stack = oldstack;
	    str = temp;
	    return;
	  }
	if (s->saved_flags & (NOPREFIX | NOEPREFIX))
	  strcpy(tname, p->name);
	else
	  strcpy(tname, full_name(p));
	
	if (*str == 39)
	  vtell_player(s, "%s%s\n", tname, piped);
	else
	  vtell_player(s, "%s %s\n", tname, piped);
	stack = prepipe;
      }
  piped = do_pipe(p, str);
  if (!piped)
    {
      cleanup_tag(pipe_list, pipe_matches);
      stack = oldstack;
      str = temp;
      return;
    }
  if (p->saved_flags & (NOPREFIX | NOEPREFIX))
    strcpy(tname, p->name);
  else
    strcpy(tname, full_name(p));
  if (*str == 39)
    vtell_player(p, " You emote : %s%s\n", tname, piped);
  else
    vtell_player(p, " You emote : %s %s\n", tname, piped);
  cleanup_tag(pipe_list, pipe_matches);
  stack = oldstack;
  sys_flags &= ~PIPE;
  str = temp;
}


/* echo command */
void            echo(player * p, char *str)
{
   char           *oldstack, *piped, *prepipe;
   player         *s;
   char *temp, msg[] = "Ribbet";

   oldstack = stack;
   temp = str;
   command_type = ROOM | ECHO_COM;

   if (!*str)
   {
      tell_player(p, " Format: echo <msg>\n");
      return;
   }
   extract_pipe_local(str);
   if (p->flags & FROGGED)
      str = (char *) msg;
   if (sys_flags & FAILED_COMMAND)
   {
      sys_flags &= ~FAILED_COMMAND;
      return;
   }
   for (s = p->location->players_top; s; s = s->room_next)
      if (s != current_player)
      {
         prepipe = stack;
         piped = do_pipe(s, str);
         if (!piped)
         {
            cleanup_tag(pipe_list, pipe_matches);
            stack = oldstack;
            return;
         }
         vtell_player(s, "%s\n", piped);
         stack = prepipe;
      }
   piped = do_pipe(p, str);
   if (!piped)
   {
      cleanup_tag(pipe_list, pipe_matches);
      stack = oldstack;
      return;
   }
   vtell_player(p, " You echo : %s\n", piped);
   cleanup_tag(pipe_list, pipe_matches);
   stack = oldstack;
   sys_flags &= ~PIPE;
}


/* the tell command */
void            tell_fn(player * p, char *str)
{
   char           *msg, *pstring, *final, *mid, *scan;
   char           *oldstack;
   player        **list, **step;
   int             i, n;
   char mesg[] = "Ribbet";
#ifdef INTERCOM
   char *intercom_pointer;
#endif

   command_type = PERSONAL | SEE_ERROR;
   
   if (p->saved_flags & BLOCK_TELLS)
   {
      tell_player(p, " You can't tell other people when you yourself "
                     "are blocking tells.\n");
      return;
   }

   oldstack = stack;
   align(stack);
   list = (player **) ALFIX stack;

   msg = next_space(str);
   if (!*msg)
   {
      tell_player(p, " Format : tell <player(s)> <msg>\n");
      return;
   }
   
#ifdef INTERCOM
   intercom_pointer=strchr(str, '@');
   if (intercom_pointer && intercom_pointer < msg)
   {
     do_intercom_tell(p, str);
     stack = oldstack;
     return;
   }
#endif
   
   if (p->flags & FROGGED)
   {
      *msg = 0;
      msg = (char *) mesg;
   } else
   {
      *msg++ = 0;
   }
   
   /* repeat */
   p->repeat_command = tell_fn;
   strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
   
   /* blockfriend */
   if(!strcasecmp(str, "friends")) {
     if(p->sflags & BLOCK_FRIEND) {
       tell_player(p, " You can't tell to friends while blocking friends.\n");
       stack = oldstack;
       return;
     }
     command_type |= FRIEND_COM;
     p->tfreplytime = time(0)+REPLY_TIMEOUT;
   }   

   if (p->location == colony && !(strcasecmp(str, "everyone"))
       && !(p->residency & ADMIN))
   {
      tell_player(p, " You can't do that whilst in the nudist's colony.\n");
      stack = oldstack;
      return;
   }
   
   if (p->saved_flags & BLOCK_SHOUT && !(strcasecmp(str,"everyone")))
   {
      tell_player(p, " You can't tell everyone whilst ignoring "
		  "shouts yourself.\n");
      stack=oldstack;
      return;
   }
   
   /* noshout check - missing in summink */
   if((p->no_shout || p->saved_flags & SAVENOSHOUT) && !strcasecmp(str, "everyone")) {
     tell_player(p, " You have been prevented from shouting.\n");
     stack = oldstack;
     return;
   }


   for (scan = msg; *scan; scan++);
   
   if (p->gender!=PLURAL)
     {
       switch (*(--scan))
	 {
	 case '?':
	   mid = "asks of";
	   break;
	 case '!':
	   mid = "exclaims to";
	   break;
	 default:
	   mid = "tells";
	   break;
	 }
     }
   else
     {
       switch (*(--scan))
	 {
	 case '?':
	   mid = "ask of";
	   break;
	 case '!':
	   mid = "exclaim to";
	   break;
	 default:
	   mid = "tell";
	   break;
	 }
     }  
   
   if (!(p->residency & PSU) && (!strcasecmp(str, "sus") ||
                                  !strcasecmp(str, "supers")))
   {
      tell_player(p, " Do you REALLY want to go pissing off all the SUs "
                     "like that? I think not...\n");
      return;
   }
   command_type |= SORE;
   n = global_tag(p, str, 1);

   if (!n)
   {
      stack = oldstack;
      return;
   }
   /* for reply */

   if (strcasecmp(str, "everyone") && strcasecmp(str, "room") &&
       strcasecmp(str, "friends"))
   {
      make_reply_list(p, list, n);
   } else
   {
      p->shout_index += count_caps(str) + 17;
   }
   if(n>1) command_type |= MULTITELL;
   
   for (step = list, i = 0; i < n; i++, step++)
   {
      if (*step != p)
      {
         pstring = tag_string(*step, list, n);
         if ((*step)->saved_flags & NOPREFIX)
            vtell_player(*step, "%s %s %s '%s'\n", p->name, mid, pstring, msg);
         else
            vtell_player(*step, "%s %s %s '%s'\n", full_name(p), mid, pstring, msg);
         stack = pstring;
      }
   }
   if (sys_flags & EVERYONE_TAG || sys_flags & FRIEND_TAG
       || sys_flags & ROOM_TAG || !(sys_flags & FAILED_COMMAND))
   {
      switch (*scan)
      {
         case '?':
            mid = "ask of";
            break;
         case '!':
            mid = "exclaim to";
            break;
         default:
            mid = "tell";
            break;
      }
      pstring = tag_string(p, list, n);
      final = stack;
      sprintf(stack, " You %s %s '%s'\n", mid, pstring, msg);
      stack = strchr(stack, 0);
      if (idlingstring(p, list, n))
         strcpy(stack, idlingstring(p, list, n));
      stack = end_string(stack);
      tell_player(p, final);
   }
   cleanup_tag(list, n);
   stack = oldstack;
}


/* the wake command */
void            wake(player * p, char *str)
{
   char           *oldstack;
   player         *p2;

   command_type = PERSONAL | SEE_ERROR;
   oldstack = stack;

   if (p->saved_flags & BLOCK_TELLS)
   {
      tell_player(p, " You can't wake other people when you yourself are "
                     "blocking tells.\n");
      return;
   }
   if (!*str)
   {
      tell_player(p, " Format : wake <player>\n");
      return;
   }
   p2 = find_player_global(str);
   if (!p2)
      return;

   if (p2->saved_flags & NOPREFIX)
      vtell_player(p2, "!!!!!!!!!! OI !!!!!!!!!!! WAKE UP, %s wants to speak "
                     "to you.\007\n", p->name);
   else
      vtell_player(p2, "!!!!!!!!!! OI !!!!!!!!!!! WAKE UP, %s wants to speak "
                     "to you.\007\n", full_name(p));

   if (sys_flags & FAILED_COMMAND)
   {
      stack = oldstack;
      return;
   }
   stack = oldstack;
   sprintf(stack, " You scream loudly at %s in an attempt to wake %s up.\n",
      full_name(p2), get_gender_string(p2));
   stack = strchr(stack, 0);
   if (p2->idle_msg[0] != 0)
      sprintf(stack, " Idling> %s %s\n", p2->name, p2->idle_msg);
   stack = end_string(stack);
   tell_player(p, oldstack);

   stack = oldstack;
}

/* remote command */

void remote(player * p, char *str)
{
   char *msg, *pstring, *final;
   char *oldstack;
   player **list, **step;
   int i, n;
   char tname[MAX_NAME + MAX_PRETITLE + 3];
   char mesg[] = "croaks";
#ifdef INTERCOM
   char *intercom_pointer;
#endif

   command_type = PERSONAL | SEE_ERROR;

   if (p->saved_flags & BLOCK_TELLS)
   {
      tell_player(p, " You can't remote to other people when you yourself are"
                     " blocking tells.\n");
      return;
   }
   if(!strncasecmp(str, "everyone", 8)) {
     command_type |= EVERYONE;
     if(p->saved_flags & BLOCK_SHOUT) {
       tell_player(p, " You may not remote to everyone while blocking shouts.\n");
       return;
     }
     if(p->location==colony && !(p->residency & ADMIN)) {
       tell_player(p, " You can't do that whilst in the nudist's colony.\n");
       return;
     }
     if(p->no_shout || p->saved_flags & SAVENOSHOUT) {
       tell_player(p, " You have been prevented from shouting.\n");
       return;
     }
   }
   oldstack = stack;
   align(stack);
   list = (player **) ALFIX stack; 

   msg = next_space(str);
   if (!*msg)
   {
      tell_player(p, " Format : remote <player(s)> <msg>\n");
      stack = oldstack;
      return;
   }
   
#ifdef INTERCOM
   intercom_pointer = strchr(str, '@');
   if (intercom_pointer && intercom_pointer < msg)
   {
     do_intercom_remote(p, str);
     stack = oldstack;
     return;
   }
#endif
   
   if ( p->flags & FROGGED )
   {
      *msg = 0;
      msg = (char *)mesg;
   } else
   {
      *msg++ = 0;
   }
   
   /* repeat */
   p->repeat_command = remote;
   strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
   
   /* block friend check */
   if(!strcasecmp(str, "friends")) {
     if(p->sflags & BLOCK_FRIEND) {
       tell_player(p, " You can't remote to friends while blocking them.\n");
       stack = oldstack;
       return;
     }
     command_type |= FRIEND_COM;
     p->tfreplytime = time(0)+REPLY_TIMEOUT;
   }
   
   command_type |= SORE;
   if (!(p->residency & PSU) && (!strcasecmp(str, "sus") ||
                                  !strcasecmp(str, "supers")))
   {
      tell_player(p, " Do you REALLY want to go annoying all the SUs "
                     "like that? I think not...\n");
      return;
   }
   n = global_tag(p, str, 1);
   if (!n)
   {
      stack = oldstack;
      return;
   }
   if(n>1) command_type |= MULTITELL;
   for (step = list, i = 0; i < n; i++, step++)
   {
      if ((*step)->saved_flags & (NOPREFIX | NOEPREFIX))
         strcpy(tname, p->name);
      else
         strcpy(tname, full_name(p));
      if (*step != p)
      {
         if (*msg == '\'')
            vtell_player(*step, "%s%s\n", tname, msg);
         else
            vtell_player(*step, "%s %s\n", tname, msg);
      }
   }

   if (sys_flags & EVERYONE_TAG || sys_flags & FRIEND_TAG
       || sys_flags & ROOM_TAG || !(sys_flags & FAILED_COMMAND))
   {
      pstring = tag_string(p, list, n);
      final = stack;
      if (p->saved_flags & (NOPREFIX | NOEPREFIX))
         strcpy(tname, p->name);
      else
         strcpy(tname, full_name(p));
      if (*msg == 39)
         sprintf(stack, " You emote '%s%s' to %s.\n", tname, msg, pstring);
      else
         sprintf(stack, " You emote '%s %s' to %s.\n", tname, msg, pstring);
      stack = strchr(stack, 0);
      if (idlingstring(p, list, n))
         strcpy(stack, idlingstring(p, list, n));
      stack = end_string(stack);
      tell_player(p, final);
   }
   cleanup_tag(list, n);
   stack = oldstack;
}


/* recho command */
void            recho(player * p, char *str)
{
   char           *msg, *pstring, *final;
   char           *oldstack;
   player        **list, **step;
   int             i, n;
   char mesg[] = "Ribbet";

   command_type = PERSONAL | ECHO_COM | SEE_ERROR;

   if (p->saved_flags & BLOCK_TELLS)
   {
      tell_player(p, " You can't recho to other people when you yourself are "
                     "blocking tells.\n");
      return;
   }
   oldstack = stack;
   align(stack);
   list = (player **) ALFIX stack; 

   msg = next_space(str);
   if (!*msg)
   {
      tell_player(p, " Format : recho <player(s)> <msg>\n");
      stack = oldstack;
      return;
   }
   if ( p->flags & FROGGED )
   {
      *msg = 0;
      msg = (char *) mesg;
   } else
      *msg++ = 0;
   
   /* repeat */
   p->repeat_command = recho;
   strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
   
   command_type |= SORE;
   
   /* friend check */
   if(!strcasecmp(str, "friends")) {
     if(p->sflags & BLOCK_FRIEND) {
       tell_player(p, " You can't recho to friends while blocking friends.\n");
       stack = oldstack;
       return;
     }
     command_type |= FRIEND_COM;
   }
   if (!(p->residency & PSU) && (!strcasecmp(str, "sus") ||
                                  !strcasecmp(str, "supers")))
   {
      tell_player(p, " Do you REALLY want to go pissing off all the SUs "
                     "like that? I think not...\n");
      return;
   }
   n = global_tag(p, str, 1);
   if (!n)
   {
      stack = oldstack;
      return;
   }
   if(n>1) command_type |= MULTITELL;
   for (step = list, i = 0; i < n; i++, step++)
      if (*step != p)
         vtell_player(*step, "%s\n", msg);

   if (sys_flags & EVERYONE_TAG || !(sys_flags & FAILED_COMMAND))
   {
      pstring = tag_string(p, list, n);
      final = stack;
      sprintf(stack, " You echo '%s' to %s\n", msg, pstring);
      while (*stack)
         stack++;
      if (idlingstring(p, list, n))
         strcpy(stack, idlingstring(p, list, n));
      stack = end_string(stack);
      tell_player(p, final);
   }
   cleanup_tag(list, n);
   stack = oldstack;
}


/* whisper command */
void            whisper(player * p, char *str)
{
  char           *oldstack, *msg, *everyone, *text, *pstring, *mid, *s;
  player        **list, *scan;
  int             n;
  char mesg[] = "Ribbet";
  
  if (p->saved_flags & BLOCK_TELLS)
    {
      tell_player(p, " You can't whisper to other people when you yourself "
		  "are blocking tells.\n");
      return;
    }
  command_type = ROOM | SEE_ERROR;
  
  oldstack = stack;
  align(stack);
  list = (player **) ALFIX stack; 
  
  msg = next_space(str);
  if (!*msg)
    {
      tell_player(p, " Format whisper <person(s)> <msg>\n");
      stack = oldstack;
      return;
    }
  if ( p->flags & FROGGED )
    {
      *msg = 0;
      msg = (char *) mesg;
  } else
    *msg++ = 0;
 
  /* repeat */
  p->repeat_command = whisper;
  strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);   
    
  for (s = msg; *s; s++);
  if (p->gender==PLURAL)
    {
      switch (*(--s))
	{
	case '?':
	  mid = "ask in a whisper";
	  break;
	case '!':
	  mid = "exclaim in a whisper";
	  break;
	default:
	  mid = "whisper";
	  break;
	}
    }
  else
    {
      switch (*(--s))
        {
        case '?':
          mid = "asks in a whisper";
          break;
        case '!':
          mid = "exclaims in a whisper";
          break;
        default:
          mid = "whispers";
          break;
        }
    }

  n = local_tag(p, str);
  if (!n){
    stack = oldstack;
    return;
  }
  everyone = tag_string(0, list, n);
  for (scan = p->location->players_top; scan; scan = scan->room_next)
    if (p != scan)
      if (scan->flags & TAGGED)
	{
	  pstring = tag_string(scan, list, n);
	  if (scan->saved_flags & NOPREFIX)
	    vtell_player(scan, "%s %s '%s' to %s.\n", p->name, mid, msg, pstring);
	  else
	    vtell_player(scan, "%s %s '%s' to %s.\n", full_name(p), mid, msg,
		    pstring);
	  stack = pstring;
      } else
	{
	  if (scan->saved_flags & NOPREFIX)
	    vtell_player(scan, "%s whisper%s something to %s.\n", p->name,
		    single_s(p), everyone);
	  else
	    vtell_player(scan, "%s whisper%s something to %s.\n", full_name(p),
		    single_s(p), everyone);
	}
  if (!(sys_flags & FAILED_COMMAND))
    {
      switch (*s)
	{
	case '?':
	  mid = "ask in a whisper";
	  break;
	case '!':
	  mid = "exclaim in a whisper";
	  break;
	default:
	  mid = "whisper";
	  break;
	}

      pstring = tag_string(p, list, n);
      text = stack;
      sprintf(stack, " You %s '%s' to %s.\n", mid, msg, pstring);
      stack = strchr(stack, 0);
      if (idlingstring(p, list, n))
	strcpy(stack, idlingstring(p, list, n));
      stack = end_string(stack);
      tell_player(p, text);
    }
  cleanup_tag(list, n);
  stack = oldstack;
}


/* exclude command */
void            exclude(player * p, char *str)
{
   char           *oldstack, *msg, *everyone, *pstring, *mid, *s;
   player        **list, *scan;
   int             n;
   char mesg[] = "Ribbet";

   command_type = ROOM | SEE_ERROR | EXCLUDE;
   oldstack = stack;
   align(stack);
   list = (player **) ALFIX stack; 

   msg = next_space(str);
   if (!*msg)
   {
      tell_player(p, " Format exclude <person(s)> <msg>\n");
      stack = oldstack;
      return;
   }
   if (!(p->residency & PSU) && (!strcasecmp(str, "sus") ||
                                  !strcasecmp(str, "supers")))
   {
      tell_player(p, " Do you REALLY want to go pissing off all the SUs "
                     "like that? I think not...\n");
      stack = oldstack;
      return;
   }
   if ( p->flags & FROGGED )
   {
      *msg = 0;
      msg = (char *) mesg;
   } else
      *msg++ = 0;
      
   /* repeat */
   p->repeat_command = exclude;
   strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
   
   for (s = msg; *s; s++);
   switch (*(--s))
   {
      case '?':
         mid = "asks";
         break;
      case '!':
         mid = "exclaims to";
         break;
      default:
         mid = "tells";
         break;
   }
   n = local_tag(p, str);
   if (!n)
   {
      stack = oldstack;
      return;
   }
   everyone = tag_string(0, list, n);
   for (scan = p->location->players_top; scan; scan = scan->room_next)
   {
      if (p != scan)
      {
         if ((scan->flags & TAGGED) && !(scan->residency & (SU | ADMIN )))
         {
            pstring = tag_string(scan, list, n);
            vtell_player(scan, "%s tells everyone something about %s\n",
                    full_name(p), pstring);
            stack = pstring;
         } else
         {
            vtell_player(scan, "%s %s everyone but %s '%s'\n",
                    full_name(p), mid, everyone, msg);
         }
      }
   }
   if (!(sys_flags & FAILED_COMMAND))
   {
      switch (*s)
      {
         case '?':
            mid = "ask";
            break;
         case '!':
            mid = "exclaim to";
            break;
         default:
            mid = "tell";
            break;
      }
      pstring = tag_string(p, list, n);
      vtell_player(p, " You %s everyone but %s '%s'\n", mid, pstring, msg);
   }
   cleanup_tag(list, n);
   stack = oldstack;
}


/* pemote command */
void            pemote(player * p, char *str)
{
   char           *oldstack, *scan;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: pemote <msg>\n");
      return;
   }
   if ( !(p->flags & FROGGED) )
   {
      for (scan = p->lower_name; *scan; scan++);
      if (*(scan - 1) == 's')
         *stack++ = 39;
      else
      {
         *stack++ = 39;
         *stack++ = 's';
      }
      *stack++ = ' ';
      while (*str)
         *stack++ = *str++;
      *stack++ = 0;
      emote(p, oldstack);
      stack = oldstack;
   } else
      emote(p, str);
}


/* premote command */
void premote(player * p, char *str)
{
   char *oldstack, *scan;
   oldstack = stack;

   scan = next_space(str);
   if (!*scan)
   {
      tell_player(p, " Format: premote <person> <msg>\n");
      return;
   }
   if ( !(p->flags & FROGGED) )
   {
      while (*str && *str != ' ')
         *stack++ = *str++;
      *stack++ = ' ';
      if (*str)
         str++;
      for (scan = p->lower_name; *scan; scan++);
      if (*(scan - 1) == 's')
         *stack++ = 39;
      else
      {
         *stack++ = 39;
         *stack++ = 's';
      }
      *stack++ = ' ';
      while (*str)
         *stack++ = *str++;
      *stack++ = 0;
      remote(p, oldstack);
      stack = oldstack;
   } else
      remote(p, str);
}


/* save command */
void            do_save(player * p, char *str)
{
   if (p->residency == NON_RESIDENT) {
      log("error", "Tried to save a non-resi, (chris)");
      return;
   }
   p->flags &= ~TRIED_QUIT;
   save_player(p);
}


/* show email */
void            check_email(player * p, char *str)
{
   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You are non resident and so cannot set an email "
                     "address.\n"
                     " Please ask a super user to make you resident.\n");
      return;
   }
   if (p->email[0]==0)
      tell_player(p, " You have set no email address, and will not save.\n");
   else if (p->email[0]==' ')
      tell_player(p, " You had had your email address set validated.\n");
   else
      vtell_player(p, " Your email address is set to: %s\n", p->email);
      
   if (p->saved_flags & PRIVATE_EMAIL)
      tell_player(p, " Your email is private.\n");
   else
      tell_player(p, " Your email is public for all to read.\n");
}


/* change whether an email is public or private */
void            public_com(player * p, char *str)
{
   if (!strcasecmp("on", str))
      p->saved_flags &= ~PRIVATE_EMAIL;
   else if (!strcasecmp("off", str))
      p->saved_flags |= PRIVATE_EMAIL;
   else
      p->saved_flags ^= PRIVATE_EMAIL;

   if (p->saved_flags & PRIVATE_EMAIL)
      tell_player(p, " Your email is private, only the admin will be able "
                     "to see it.\n");
   else
      tell_player(p, "Your email address is public, so everyone can see "
                     "it.\n");
}


/* email command */
void            change_email(player * p, char *str)
{
   p->flags &= ~TRIED_QUIT;
   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You may only use the email command once resident.\n"
                     " Please ask a superuser to become to grant you "
                     "residency.\n");
      return;
   }
   if (!*str)
   {
      check_email(p, str);
      return;
   }
   if (!strcasecmp(str, "private"))
   {
      p->saved_flags |= PRIVATE_EMAIL;
      tell_player(p, " Your email is private, only the Admin will be able "
                     "to see it.\n");
      return;
   }
   if (!strcasecmp(str, "public"))
   {
      p->saved_flags &= ~PRIVATE_EMAIL;
      tell_player(p, " Your email address is public, so everyone can see "
                     "it.\n");
      return;
   }
   if (p->email[0]==0)
      vlog("help", "%-18s %s (New)", p->name, str);
   strncpy(p->email, str, MAX_EMAIL - 2);
   lower_case(p->email);
   vtell_player(p, " Email address has been changed to: %s\n", p->email);
   if(p->password[0]!=0)
     p->residency &= ~NO_SYNC;
   save_player(p);
   return;
}


/* password changing routines */
char           *do_crypt(char *entered, player * p)
{
   char            key[9];
   strncpy(key, entered, 8);
   return crypt(key, p->lower_name);
}


void            got_password2(player * p, char *str)
{
   password_mode_off(p);
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   if (strcmp(p->password_cpy, str))
      tell_player(p, "\n But that doesn't match !!!\n"
                     " Password not changed ...\n");
   else
   {
      strcpy(p->password, do_crypt(str, p));
      tell_player(p, "\n Password has now been changed.\n");
      if (p->email[0]!=0)
         p->residency &= ~NO_SYNC;
      save_player(p);
   }
}


void            got_password1(player * p, char *str)
{
   if ((int)strlen(str) > (MAX_PASSWORD - 2))
   {
      do_prompt(p, "\n Password too long, please try again.\n"
                   " Please enter a shorter password: ");
      p->input_to_fn=got_password1;
   } 
   else if ((int)strlen(str)<3) 
   {
      do_prompt(p, "\n Password too short, please try again.\n"
      		   " Please enter a longer password: ");
      p->input_to_fn = got_password1;
   }
   else
   {
      strcpy(p->password_cpy, str);
      do_prompt(p, "\n Enter password again to verify: ");
      p->input_to_fn = got_password2;
   }
}


void            validate_password(player * p, char *str)
{
   if (!check_password(p->password, str, p))
   {
      tell_player(p, "\n Hey ! thats the wrong password !!\n");
      password_mode_off(p);
      p->input_to_fn = 0;
      p->flags |= PROMPT;
   } else
   {
      do_prompt(p, "\n Now enter a new password: ");
      p->input_to_fn = got_password1;
   }
}


void            change_password(player * p, char *str)
{
   p->flags &= ~TRIED_QUIT;
   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You may only set a password once resident.\n"
                     " To become a resident, please ask a superuser.\n");
      return;
   }
   password_mode_on(p);
   p->flags &= ~PROMPT;
   if (p->password[0]!=0)
   {
      do_prompt(p, " Please enter your current password: ");
      p->input_to_fn = validate_password;
   } else
   {
      do_prompt(p, " You have no password.\n"
                   " Please enter a password: ");
      p->input_to_fn = got_password1;
   }
}


/* the 'check' command */
void            check(player * p, char *str)
{
   if (!*str)
   {
      tell_player(p, " Format: check <sub command>\n");
      return;
   }
   sub_command(p, str, check_list);
}


/* view check commands */
void            view_check_commands(player * p, char *str)
{
   view_sub_commands(p, check_list);
}


/* show wrap info */
void            check_wrap(player * p, char *str)
{
   char           *oldstack;

   oldstack = stack;
   if (p->term_width)
   {
      sprintf(stack, " Line wrap on, with terminal width set to %d "
                     "characters.\n",
              p->term_width);
      stack = strchr(stack, 0);
      if (p->word_wrap)
         sprintf(stack, " Word wrap is on, with biggest word size "
                        "set to %d characters.\n",
                 p->word_wrap);
      else
         strcpy(stack, " Word wrap is off.\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
   } else
      tell_player(p, " Line wrap and word wrap turned off.\n");
   stack = oldstack;
}


/* Toggle the ignoreprefix flag (saved_flags) */
void            ignoreprefix(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~NOPREFIX;
   else if (!strcasecmp("on", str))
      p->saved_flags |= NOPREFIX;
   else
      p->saved_flags ^= NOPREFIX;

   if (p->saved_flags & NOPREFIX)
      tell_player(p, " You are now ignoring prefixes.\n");
   else
      tell_player(p, " You are now seeing prefixes.\n");
}


/* Toggle the ignore emote flag (saved_flags) */
void            ignoreemoteprefix(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~NOEPREFIX;
   else if (!strcasecmp("on", str))
      p->saved_flags |= NOEPREFIX;
   else
      p->saved_flags ^= NOEPREFIX;

   if (p->saved_flags & NOEPREFIX)
      tell_player(p, " You are now ignoring prefixes specifically on emotes\n");
   else
      tell_player(p, " You are now seeing prefixes specifically on emotes again.\n");
}


void            set_time_delay(player * p, char *str)
{
   int             time_difference;

   if (!*str)
   {
      if (p->jetlag)
         vtell_player(p, " Your time difference is currently set at %d hours.\n",
                 p->jetlag);
      else
         vtell_player(p, " Your time difference is not currently set.\n");
      return;
   }
   time_difference = atoi(str);
   if (!time_difference)
   {
      tell_player(p, " Time difference of 0 hours set. "
                     "(that was worth it, wasn't it... )\n");
      p->jetlag = 0;
      return;
   }
   if (time_difference < -23 || time_difference > 23)
   {
      tell_player(p, " That's a bit silly, isn't it?\n");
      return;
   }
   p->jetlag = time_difference;

   if (p->jetlag == 1)
      tell_player(p, " Time Difference set to 1 hour.\n");
   else
      vtell_player(p, " Time Difference set to %d hours.\n", p->jetlag);
}


void            set_ignore_msg(player * p, char *str)
{
   if (!*str)
   {
      tell_player(p, " You reset your ignore message.\n");
      strcpy(p->ignore_msg, "");
      return;
   }
   strncpy(p->ignore_msg, str, MAX_IGNOREMSG - 2);
   vtell_player(p, " Ignore message now set to ...\n %s\n", p->ignore_msg);
}


/* new think, tryig to fix the NOPREFIX thang by copying from say */
void            newthink(player * p, char *str)
{
   char           *oldstack, *piped, *prepipe;
   player         *s;
   char *temp, msg[]="Ribbet";

   oldstack = stack;
   command_type = ROOM;
   temp = str;

   if (!*str)
   {
      tell_player(p, " Format: think <msg>\n");
      return;
   }

   if ( p->flags & FROGGED )
      str = (char *)msg;
      
#ifdef INTERCOM
#ifdef INTERCOM_EXT
   if(p->location && p->location==intercom_room)
   {
     do_intercom_think(p, str);
     return;
   }
#endif
#endif

   extract_pipe_local(str);
   if (sys_flags & FAILED_COMMAND)
   {
      sys_flags &= ~FAILED_COMMAND;
      str = temp;
      return;
   }

   for (s = p->location->players_top; s; s = s->room_next)
   {
      if (s != current_player)
      {
         prepipe = stack;
         piped = do_pipe(s, str);
         if (!piped)
         {
            cleanup_tag(pipe_list, pipe_matches);
            stack = oldstack;
            str = temp;
            return;
         }
         if (s->saved_flags & NOPREFIX)
            vtell_player(s, "%s thinks . o O ( %s )\n", p->name, piped);
         else
            vtell_player(s, "%s thinks . o O ( %s )\n", full_name(p), piped);
         stack = prepipe;
      }
   }
   piped = do_pipe(p, str);
   if (!piped)
   {
      cleanup_tag(pipe_list, pipe_matches);
      stack = oldstack;
      str = temp;
      return;
   }
   vtell_player(p, " You think . o O ( %s )\n", piped);
   cleanup_tag(pipe_list, pipe_matches);
   stack = oldstack;
   sys_flags &= ~PIPE;
   str = temp;
}


/* Go to the nudist colony, where you can be as offensive as you like */
/* Well, nearly */
void go_colony(player *p, char *str)
{
   player *p2;

   if ((p->residency & SU) && (*str))
       /* Move someone else to the colony */
   {
       /* find them */
       p2 = find_player_global(str);
       if (!p2)
	   return;
	   
       /* no colonising your superiors... */
       if (p2->residency >= p->residency)
       {
	 tell_player(p, " Nya, I don't think you should do that to your superiors !\n");
	 tell_player(p2," Hehe, someone just tried to colonise you !/n");
	 return;
       }
       /* and they might already be there */
       if(p2->location==colony)
       {
	       tell_player(p, " They're already in the nudist colony!\n");
	       return;
       }
       /* tell the SU channel */
       command_type |= ADMIN_BARGE;
       vsu_wall("-=> %s puts %s in the colony.\n", p->name, p2->name);
       /* tell the player */
       tell_player(p2, "-=> You suddenly find yourself nude as the scene"
	" about you disappears and you reappear in...\n");
       /* move them */
       move_to(p2, "summink.colony", 0);
       /* stick them in place for 60 secs */
       p2->no_move=60;
       return;
   }

   /* no argument specified or player is not an SU */
   /* check if they're in the colony already */
   if(p->location==colony)
   {
       tell_player(p, " You're already in the nudist colony!\n");
       return;
   }
   /* or if they're stuck */
   if (p->no_move)
   {
      tell_player(p, " You seem to be stuck to the ground.\n");
      return;
   }
   /* otherwise move them */
   move_to(p, "summink.colony", 0);
}


/* tell to your friends, the short way */
void tell_friends(player *p, char *str)
{
   char *oldstack;

   if (!*str)
   {
      tell_player(p, " Format: tf <message>\n");
      return;
   }
   oldstack = stack;
   sprintf(stack, "friends %s", str);
   stack = end_string(stack);
   tell_fn(p, oldstack);
   stack = oldstack;
}


/* remote to your friends, the short way */
void remote_friends(player *p, char *str)
{
   char *oldstack;

   if (!*str)
   {
      tell_player(p, " Format: rf <remote>\n");
      return;
   }
   oldstack = stack;
   sprintf(stack, "friends %s", str);
   stack = end_string(stack);
   remote(p, oldstack);

   stack = oldstack;
}


/* remote think */
void remote_think(player *p, char *str)
{
   char *oldstack, *msg;

   oldstack = stack;

   msg = strchr(str, ' ');
   if (!msg)
   {
      tell_player(p, " Format: rt <player(s)> <think>\n");
      return;
   }

   *msg++ = '\0';
   sprintf(stack, "%s thinks . o O ( %s )", str, msg);
   stack = end_string(stack);
   remote(p, oldstack);
   stack = oldstack;
}


/* the reply to friends command */
void            tfreply(player * p, char *str)
{
    char           *msg, *final, *mid, *scan;
    char           *oldstack;
    player        **list, **step, *p2;
    int             i, n;
    char mesg[] = "Ribbet";
    list_ent	*l;    
    
    oldstack = stack;
    align(stack);
    list = (player **) ALFIX stack;
    
    msg = next_space(str);
    if (!*msg)
    {
	tell_player(p, " Format : replytf <player> <msg>\n");
	stack = oldstack;
	return;
    }
    
    if (p->saved_flags & BLOCK_TELLS)
    {
	tell_player(p, " You can't tell other people when you yourself "
		    "are blocking tells.\n");
	stack = oldstack;
	return;
    }
    if (p->sflags & BLOCK_TF_REPLIES || p->sflags & BLOCK_FRIEND)
    {
	tell_player(p, " You can't reply to peoples friends while blocking friends or replies.\n");
	stack = oldstack;
	return;
    }
    
    if (p->flags & FROGGED)
    {
	*msg = 0;
	msg = (char *) mesg;
    } else
    {
	*msg++ = 0;
    }
    
    /* repeat */
    p->repeat_command = tfreply;
    strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
    
    command_type = FRIEND_COM | PERSONAL | SEE_ERROR | TFREPLY;
    
    p2 = find_player_global(str);
    if (!p2) 
    {
	stack = oldstack;
	return;
    }
    if (p2==p)
    {
	tell_player(p, " Try using the tell friends command.\n");
	stack = oldstack;
	return;
    }
    if (p2->sflags & BLOCK_TF_REPLIES || p2->sflags & BLOCK_FRIEND)
    {
	tell_player(p, " Sorry they are blocking friend tells or replies.\n");
	stack = oldstack;
	return;
    }
    l = find_list_entry(p, p2->name);
    if (l && l->flags & FRNDBLOCK)
    {
       tell_player(p, " You can't reply to their friends while friendblocking them.\n");
       stack = oldstack;
       return;
    }
    if (l && l->flags & IGNORE)
    {
       tell_player(p, " You can't reply to their friends while ignoring them.\n");
       stack = oldstack;
       return;
    }
    if (l && l->flags & BLOCK && !(l->flags & NOISY))
    {
       tell_player(p, " You can't reply to their friends while blocking them.\n");
       stack = oldstack;
       return;
    }
    l = find_list_entry(p2, p->name);
    if ((!l) ||
	(l->flags & BLOCK || l->flags & IGNORE) ||
	!(l->flags & FRIEND))
    {
	tell_player(p, " You can't to reply to that persons friends.\n");
	stack = oldstack;
	return;
    } 
    if (time(0)<p2->tfreplytime)
	p2->tfreplytime = time(0)+REPLY_TIMEOUT;
    else
    {
	tell_player(p, " That person doesn't currently have a friend reply list.\n");
	stack = oldstack;
	return;
    }
    
    
    for (scan = msg; *scan; scan++);
    
    if (p->gender!=PLURAL)
    {
	switch (*(--scan))
	{
	  case '?':
	    mid = "asks of";
	    break;
	  case '!':
	    mid = "exclaims to";
	    break;
	  default:
	    mid = "tells";
	    break;
	}
    }
    else
    {
	switch (*(--scan))
	{
	  case '?':
	    mid = "ask of";
	    break;
	  case '!':
	    mid = "exclaim to";
	    break;
	  default:
	    mid = "tell";
	    break;
	}
    }  
    
    n = friend_tag_but(p, p2);
    
    if (!n)
    {
	stack = oldstack;
	return;
    }
    for (step = list, i = 0; i < n; i++, step++)
    {
	if (*step != p)
	{
	    if ((*step)->saved_flags & NOPREFIX)
		vtell_player(*step, "%s %s the friends of %s '%s'\n", p->name, mid, p2->name, msg);
	    else
		vtell_player(*step, "%s %s the friends of %s '%s'\n", full_name(p), mid, p2->name, msg);
	}
    }
    if (p2->saved_flags & NOPREFIX)
       vtell_player(p2, "%s %s your friends '%s'\n", p->name, mid, msg);
    else
       vtell_player(p2, "%s %s your friends '%s'\n", full_name(p), mid, msg);
    if (sys_flags & FRIEND_TAG || !(sys_flags & FAILED_COMMAND))
    {
	switch (*scan)
	{
	  case '?':
            mid = "ask of";
            break;
	  case '!':
            mid = "exclaim to";
            break;
	  default:
            mid = "tell";
            break;
	}
	final = stack;
	sprintf(stack, " You %s the friends of %s '%s'\n", mid, p2->name, msg);
	stack = strchr(stack, 0);
	if (idlingstring(p, list, n))
	    strcpy(stack, idlingstring(p, list, n));
	stack = end_string(stack);
	tell_player(p, final);
    }
    cleanup_tag(list, n);
    stack = oldstack;
}


/* remote command */
void rfreply(player * p, char *str)
{
    char *msg, *final;
    char *oldstack;
    player **list, **step, *p2;
    int i, n;
    char tname[MAX_NAME + MAX_PRETITLE + 3];
    char mesg[] = "croaks";
    list_ent *l;
    
    command_type = PERSONAL | SEE_ERROR | FRIEND_COM | TFREPLY;
    
    
    oldstack = stack;
    align(stack);
    list = (player **) ALFIX stack;
    
    msg = next_space(str);
    if (!*msg)
    {
	tell_player(p, " Format : ereplytf <player> <msg>\n");
	stack = oldstack;
	return;
    }
    if (p->saved_flags & BLOCK_TELLS)
    {
	tell_player(p, " You may not remote while you are blocking tells.\n");
	stack = oldstack;
	return;
    }
    if (p->sflags & BLOCK_TF_REPLIES || p->sflags & BLOCK_FRIEND)
    {
	tell_player(p, " You can't ereply to peoples friends while blocking friends or replies.\n");
	stack = oldstack;
	return;
    }
    if ( p->flags & FROGGED )
    {
	*msg = 0;
	msg = (char *)mesg;
    } else
    {
	*msg++ = 0;
    }
    
    /* repeat */
    p->repeat_command = rfreply;
    strncpy(p->repeat_string, msg, MAX_REPEAT_STRING-1);
    
    p2 = find_player_global(str);
    if (!p2) 
    {
	stack = oldstack;
	return;
    }
    if (p2==p)
    {
	tell_player(p, " Try using the remote friends command.\n");
	stack = oldstack;
	return;
    }
    if (p2->sflags & BLOCK_TF_REPLIES || p2->sflags & BLOCK_FRIEND)
    {
	tell_player(p, " Sorry they are blocking friend tells or replies.\n");
	stack = oldstack;
	return;
    }
    l = find_list_entry(p, p2->name);
    if (l && l->flags & FRNDBLOCK)
    {
       tell_player(p, " You can't reply to their friends while friendblocking them.\n");
       stack = oldstack;
       return;
    }
    if (l && l->flags & IGNORE)
    {
       tell_player(p, " You can't reply to their friends while ignoring them.\n");
       stack = oldstack;
       return;
    }
    if (l && l->flags & BLOCK && !(l->flags & NOISY))
    {
       tell_player(p, " You can't reply to their friends while blocking them.\n");
       stack = oldstack;
       return;
    }
    l = find_list_entry(p2, p->name);
    if ((!l) ||
	(l->flags & BLOCK || l->flags & IGNORE) ||
	!(l->flags & FRIEND))
    {
	tell_player(p, " You can't ereply to that persons friends.\n");
	stack = oldstack;
	return;
    } 
    if (time(0)<p2->tfreplytime)
	p2->tfreplytime = time(0)+REPLY_TIMEOUT;
    else
    {
	tell_player(p, " That person doesn't currently have a friend reply list.\n");
	stack = oldstack;
	return;
    }
    
    
    n = friend_tag_but(p, p2);
    if (!n)
    {
	stack = oldstack;
	return;
    }
    for (step = list, i = 0; i < n; i++, step++)
    {
	if ((*step)->saved_flags & (NOPREFIX | NOEPREFIX))
	    strcpy(tname, p->name);
	else
	    strcpy(tname, full_name(p));
	if (*step != p)
	{
	    if (*msg == '\'')
		vtell_player(*step, "%s%s (to friends of %s)\n", tname, msg, p2->name);
	    else
		vtell_player(*step, "%s %s (to friends of %s)\n", tname, msg, p2->name);
	}
    }
    if (p2->saved_flags & (NOPREFIX | NOEPREFIX))
        strcpy(tname, p->name);
    else
	strcpy(tname, full_name(p));
    if (*msg == '\'')
	vtell_player(p2, "%s%s (to your friends)\n", tname, msg);
    else
	vtell_player(p2, "%s %s (to your friends)\n", tname, msg);
    
    if (sys_flags & FRIEND_TAG || !(sys_flags & FAILED_COMMAND))
    {
	final = stack;
	if (p->saved_flags & (NOPREFIX | NOEPREFIX))
	    strcpy(tname, p->name);
	else
	    strcpy(tname, full_name(p));
	if (*msg == 39)
	    sprintf(stack, " You emote '%s%s' to the friends of %s.\n", tname, msg, p2->name);
	else
	    sprintf(stack, " You emote '%s %s' to the friends of %s.\n", tname, msg, p2->name);
	stack = strchr(stack, 0);
	if (idlingstring(p, list, n))
	    strcpy(stack, idlingstring(p, list, n));
	stack = end_string(stack);
	tell_player(p, final);
	stack = final;
    }
    cleanup_tag(list, n);
    stack = oldstack;
}


/* thing to let us toggle blocking of friend replies */
void tf_reply_block(player *p, char *str)
{
    if (!strcmp("on", str))
	p->sflags |= BLOCK_TF_REPLIES;
    else if (!strcmp("off", str))
	p->sflags &= ~BLOCK_TF_REPLIES;
    else
	p->sflags ^= BLOCK_TF_REPLIES;
    
    if (p->sflags & BLOCK_TF_REPLIES)
	tell_player(p, " You block friend tell replies.\n");
    else
	tell_player(p, " You allow friend tell replies.\n");
}


#ifdef SOCIALS
#include "include/social_commands.c"
#endif
