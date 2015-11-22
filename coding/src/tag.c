/*
 * tag.c
 */
 
#include "include/config.h" 
 

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "include/missing_headers.h"
#include "include/proto.h"


/* print a list of players and prefix's */

void            construct_name_list_swho(player ** list, int count, int swho_mode)
{
   int             max_length = 0, n, m, cols, col_count;
   int             len;
   char           *fname;
   player        **scan;
   if (!count)
      return;

   for (scan = list, n = count; n; n--, scan++)
   {
      if (current_player->saved_flags & NOPREFIX)
        len = strlen((*scan)->name);
      else
        len = strlen(full_name(*scan));
      if (len > max_length)
        max_length = len;
   }
   if (current_player->term_width)
      cols = (current_player->term_width) / (max_length + 3);
   else
      cols = 78 / (max_length + 3);
   col_count = cols;

   for (scan = list, n = count; n; n--, scan++)
   {
      if (current_player->saved_flags & NOPREFIX)
        fname = (*scan)->name;
      else
        fname = full_name(*scan);
      len = strlen(fname);
      for (m = (max_length - len); m; m--)
        *stack++ = ' ';
      while (*fname)
        *stack++ = *fname++;
      *stack++ = ' ';
      *stack++ = ' ';
      col_count--;
      if (!col_count)
      {
        *stack++ = '\n';
        col_count = cols;
      }
   }
   *stack++ = '\n';
}


void            construct_name_list(player ** list, int count)
{
   construct_name_list_swho(list, count, 0);
}


/* routine to find someone absolutly and quietly */
player         *find_player_absolute_quiet(char *name)
{
   player         *scan;

   if (!isalpha(*name))
      return 0;

   strcpy(stack, name);
   lower_case(stack);

   scan = hashlist[(int) (*stack) - (int) 'a' + 1];
   for (; scan; scan = scan->hash_next)
      if (!strcmp(stack, scan->lower_name))
    return scan;

   return 0;
}

/* big monster routine to find someone globally */

player         *find_player_global_quiet(char *name)
{
   player         *scan, *pref=0;
   int             test, count = 0, preferred = 0;
   list_scan ls;
   char           *oldstack, *lname;
   player        **list;

   oldstack = stack;

   if (!isalpha(*name))
      return 0;
   stack = oldstack;
   lname = stack;
   strcpy(lname, name);
   lower_case(lname);
   stack = end_string(lname);
   align(stack);

   list = (player **) ALFIX stack;
   scan = hashlist[(int) (*lname) - (int) 'a' + 1];
   
   for (; scan; scan = scan->hash_next) {
      test = match_player(scan->lower_name, lname);
      if (test > 0) {
         stack = oldstack;
         return scan;
      }
      if (test < 0) {
         if(current_player) {
           ls = listscan(current_player, scan);
           if(ls.l && (ls.l->flags & PREFERRED || ls.lf->flags & PREFERRED)) {
              pref = scan;
              preferred++;
           }
         }
         *list++ = scan;
         count++;
      }
   }
   
   if (count == 1) {
      stack = oldstack;
      return (*(--list));
   }
   if(preferred==1) {
     stack = oldstack;
     return pref;
   }
   
   stack = oldstack;
   return 0;
}

/* big monster routine to find someone globally */

player         *find_player_global(char *name)
{
   player         *scan, *pref=0;
   int             test, count = 0, preferred = 0;
   list_scan ls;
   char           *oldstack, *lname, *text;
   player        **list;

   oldstack = stack;

   if (!isalpha(*name))
   {
      vtell_current(" No such name - %s -\n", name);
      return 0;
   }
   stack = oldstack;
   lname = stack;
   strcpy(lname, name);
   lower_case(lname);
   stack = end_string(lname);
   align(stack);

   list = (player **) ALFIX stack; 
   scan = hashlist[(int) (*lname) - (int) 'a' + 1];
   for (; scan; scan = scan->hash_next)
   {
      test = match_player(scan->lower_name, lname);
      if (!(command_type & NO_P_MATCH && scan == current_player))
      {
         if (test > 0) {
            stack = oldstack;
            return scan;
         }
         if (test < 0) {
           if(current_player) {
             ls = listscan(current_player, scan);
             if(ls.l && (ls.l->flags & PREFERRED || ls.lf->flags & PREFERRED)) {
               pref = scan;
               preferred++;
             }
             *list++ = scan;
             count++;
           }
         }
      }
   }
   
   if (!count) {
      stack = oldstack;
      if (!strcmp(lname, current_player->lower_name))
        vtell_current(" \'%s\' is you, you spoon!\n", name);
      else
        vtell_current(" No-one of the name '%s' on at the moment.\n", name);
      return 0;
   }
   if (count == 1) {
      stack = oldstack;
      return (*(--list));
   }
   if(preferred==1) {
     stack = oldstack;
     return pref;
   }
   
   stack = (char *) list;
   text = stack;
   list--;
   sprintf(text, " Multiple matches : %s", (*list)->name);
   while (*stack)
      stack++;
   for (list--, count--; count > 1; count--, list--) {
      sprintf(stack, ", %s", (*list)->name);
      while (*stack)
        stack++;
   }
   sprintf(stack, " and %s.\n", (*list)->name);
   stack = end_string(stack);
   tell_current(text);
   stack = oldstack;
   return 0;
}


/* second monster routine for local finds */

player         *find_player_local(char *name)
{
  player         *scan, *pref=0;
  int             test, count = 0, preferred = 0;
  list_scan 	  ls;
  char           *oldstack, *lname, *text;
  player        **list;

  oldstack = stack;

  if (!*name) /* this should never be allowed to happen */
    return 0;
  
  if (!isalpha(*name)) {
    vtell_current(" No such name - %s -\n", name);
    return 0;
  }
  stack = oldstack;

  lname = stack;
  strcpy(lname, name);
  lower_case(lname);
  stack = end_string(lname);
  align(stack);

  list = (player **) ALFIX stack;
  scan = current_player->location->players_top;
  for (; scan; scan = scan->room_next) {
    test = match_player(scan->lower_name, lname);
    if (test > 0) {
      stack = oldstack;
      return scan;
    }
    if (test < 0) {
      if(current_player) {
        ls = listscan(current_player, scan);
        if(ls.l && (ls.l->flags & PREFERRED || ls.lf->flags & PREFERRED)) {
          pref = scan;
          preferred++;
        }
      }
      *list++ = scan;
      count++;
    }
  }
   
  if (!count) {
    stack = oldstack;
    vtell_current(" No-one of the name '%s' in the room.\n", name);
    return 0;
  }
  if (count == 1) {
    stack = oldstack;
    return (*(--list));
  }
  if(preferred==1) {
    stack = oldstack;
    return pref;
  }
  stack = (char *) list;
  text = stack;
  list--;
  sprintf(text, " Multiple matches : %s", (*list)->name);
  while (*stack)
    stack++;
  for (list--, count--; count > 1; count--, list--) {
    sprintf(stack, ", %s", (*list)->name);
    while (*stack)
      stack++;
  }
  sprintf(stack, " and %s.\n", (*list)->name);
  stack = end_string(stack);
  tell_current(text);
  stack = oldstack;
  return 0;
}


void            su_wall_but(player * p, char *str)
{
   player         *scan;

   command_type |= CHANNEL;
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->residency & PSU && !(scan->flags & BLOCK_SU)
          && scan != p)
      {
            if (scan->saved_flags & SU_HILITED)
               command_type |= HIGHLIGHT;
            tell_player(scan, str);
            if (scan->saved_flags & SU_HILITED)
               command_type &= ~HIGHLIGHT;
      }
   }
   command_type &= ~CHANNEL;
}


/* shout to su's */
void            su_wall(char *str)
{
   player         *scan;

   command_type |= CHANNEL;
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->residency & PSU && !(scan->flags & BLOCK_SU))
      {
            if (scan->saved_flags & SU_HILITED)
            {
               command_type |= HIGHLIGHT;
            }
            tell_player(scan, str);
            if (scan->saved_flags & SU_HILITED)
            {
               command_type &= ~HIGHLIGHT;
            }
      }
   }
   command_type &= ~CHANNEL;
}


/* non blockable shout to everybody */
void            raw_wall(char *str)
{
   player         *scan;
   for (scan = flatlist_start; scan; scan = scan->flat_next)
      non_block_tell(scan, str);
}


/* tell a room something */

void            tell_room(room * r, char *str)
{
   player         *scan;
   if (!r)
      return;
   command_type |= ROOM;
   for (scan = r->players_top; scan; scan = scan->room_next)
     if (scan != current_player)
       tell_player(scan, str);
   command_type &= ~ROOM;
}


/* tell a room and not a person something */

void            tell_room_but(player * p, room * r, char *str)
{
   player         *scan;
   if (!r)
      return;
   command_type |= ROOM;
   for (scan = r->players_top; scan; scan = scan->room_next)
     if (scan != p)
       tell_player(scan, str);
   command_type &= ~ROOM;
}


/* cleans up after a tag */

void            cleanup_tag(player ** tagged_list, int matches)
{
   sys_flags &= ~(EVERYONE_TAG | ROOM_TAG | FRIEND_TAG);
   for (; matches; matches--, tagged_list++)
      (*tagged_list)->flags &= ~TAGGED;
   pipe_matches = 0;
   pipe_list = 0;
}


/* stack a string of players in tag list */

char           *name_string(player * p, player * tagged)
{
   if (tagged == current_player)
      if (p == current_player)
        return "yourself";
      else
    switch (tagged->gender)
    {
       case VOID_GENDER:
          return "itself";
          break;
       case OTHER:
          return "itself";
          break;
       case MALE:
          return "himself";
          break;
       case FEMALE:
          return "herself";
          break;
    }
   if (tagged == p)
      return "you";
   return tagged->name;
}

/* self string */

char           *self_string(player * p)
{
    switch (p->gender)
    {
       case VOID_GENDER:
          return "itself";
          break;
       case OTHER:
          return "itself";
          break;
       case MALE:
          return "himself";
          break;
	case PLURAL:
          return "themselves";
          break;	  
       case FEMALE:
          return "herself";
          break;
    }
    return "wibble (FROGGED in gender)";
}

char           *tag_string(player * p, player ** tagged_list, int matches)
{
   char           *start;
   start = stack;
   if (!matches)
   {
      *stack = 0;
      return 0;
   }
   if (sys_flags & EVERYONE_TAG)
   {
      strcpy(stack, "everyone");
      stack = end_string(stack);
      return start;
   }
   if (sys_flags & ROOM_TAG)
   {
      strcpy(stack, "everyone in the room");
      stack = end_string(stack);
      return start;
   }
   if (sys_flags & FRIEND_TAG)
   {
      if (current_player == p)
        strcpy(stack, "your friends");
      else
        sprintf(stack, "%s friends", gstring_possessive(current_player));
      stack = end_string(stack);
      return start;
   }
   strcpy(stack, name_string(p, *tagged_list++));
   while (*stack)
      stack++;
   matches--;
   for (; matches > 1; matches--)
   {
      sprintf(stack, ", %s", name_string(p, *tagged_list++));
      while (*stack)
    stack++;
   }
   if (matches)
   {
      sprintf(stack, " and %s", name_string(p, *tagged_list++));
      while (*stack)
    stack++;
   }
   *stack++ = 0;
   return start;
}


/* finds people globally and tags them */
int global_tag(player * p, char *tag_str, int ptag)
{
   list_ent *entry;
   char *oldstack, *start;
   player *tag, **list_start;
   int matches = 0;

   oldstack = stack;
   align(stack);
   list_start = (player **) ALFIX stack; 

   if (!*tag_str)
   {
      tell_player(p, " No names to tag !.\n");
      return 0;
   }
   if (!strcasecmp(tag_str, "everyone"))
   {
      if (p->shout_index > 60 && command_type & SORE)
      {
         tell_player(p, " You seem to have developed a sore throat.\n");
         return 0;
      }
      p->shout_index += (count_caps(tag_str) * 2) + 20;

      if (p->no_shout && command_type & PERSONAL)
      {
         tell_player(p, " You have been prevented from shouting.\n");
         return 0;
      }
      /*sys_flags |= EVERYONE_TAG;*/
      tag = flatlist_start;
      while (tag)
      {
         if (tag->location && (tag!=p || ptag))
         {
            *((player **) ALFIX stack) = tag; 
            stack += sizeof(player **);
            tag->flags |= TAGGED;
            matches++;
         }
         tag = tag->flat_next;
      }
      return matches;
   }
   if (!strcasecmp(tag_str, "room"))
   {
      sys_flags |= ROOM_TAG;
      tag = p->location->players_top;
      while (tag)
      {
         if (tag->location && (tag!=p || ptag))
         {
            *((player **) ALFIX stack) = tag;
            stack += sizeof(player **);
            tag->flags |= TAGGED;
            matches++;
         }
         tag = tag->room_next;
      }
      return matches;
   }
   if (!strcasecmp(tag_str, "friends") && p->residency != NON_RESIDENT)
   {
      sys_flags |= FRIEND_TAG;
      if (!p->saved || !p->saved->list_top)
      {
         tell_player(p, "This command requires that you have a list.\n");
         return 0;
      }
      entry = p->saved->list_top;
      while (entry)
      {
         if (entry->flags & FRIEND && entry->name != "everyone")
         {
            tag = find_player_absolute_quiet(entry->name);
            if (tag && tag->location && (tag!=p || ptag))
            {
               *((player **) ALFIX stack) = tag; 
               stack += sizeof(player **);
               tag->flags |= TAGGED;
               matches++;
            }
         }
         entry = entry->next;
      }
      if (matches == 0)
         tell_player(p, " You have no friends on at the moment.\n");
      return matches;
   }

   /* tell newbies for superusers */
   if (!strcasecmp(tag_str, "newbies") && p->residency & PSU)
   {
      sys_flags |= NEWBIE_TAG;
      tag = flatlist_start;
      while (tag)
      {
         if (tag->residency == NON_RESIDENT && tag->location && (tag!=p || ptag)) 
         {
            *((player **) ALFIX stack) = tag; 
            stack += sizeof(player **);
            tag->flags |= TAGGED;
            matches++;
         }
         tag = tag->flat_next;
      }
      if (matches == 0)
         tell_player(p, " There are no newbies on at the moment.\n");
      return matches;
   }
   if ((!strcasecmp(tag_str, "supers") || !strcasecmp(tag_str, "sus")))
   {
      tag = flatlist_start;
      while (tag)
      {
         if (tag != p
             && (tag->residency & PSU && (p->residency & PSU
                                               || !(tag->flags & BLOCK_SU)))
             && tag->location)
         {
            *((player **) ALFIX stack) = tag; 
            stack += sizeof(player **);
            tag->flags |= TAGGED;
            matches++;
         }
         tag = tag->flat_next;
      }
      if (matches == 0)
         tell_player(p, " There are no other Super Users on at the moment.\n");
      return matches;
   }

   /* initial check to find anything non comma */
   while(*tag_str && *tag_str == ',')
     tag_str++;
   if(!*tag_str) {
     tell_player(p, " Badly formed <user_list> (see 'help user_list').\n");
     cleanup_tag(list_start, matches);
     return 0;
   }

   while (*tag_str)
   {
      /* keep going till the next non comma */
      while(*tag_str && *tag_str == ',')
        tag_str++;
      
      /* extract if possible non commas to stack */
      start = stack;
      while (*tag_str && *tag_str != ',')
         *stack++ = *tag_str++;
         
      /* terminate this, start points to name */
      *stack++ = 0;
      
      if (!*start)
      {
         if(matches)
           return matches;
                  
         cleanup_tag(list_start, matches);
         return 0;
      }
      
      if (*tag_str)
         tag_str++;

      if (!strcasecmp(start, "me"))
         tag = p;
      else
      {
         command_type |= NO_P_MATCH;
         tag = find_player_global(start);
         command_type &= ~NO_P_MATCH;
      }

      stack = start;
      if ((tag) && !(tag->flags & TAGGED))
      {
         tag->flags |= TAGGED;
         *((player **) ALFIX stack) = tag;
         stack += sizeof(player *);
         matches++;
         if (matches > NAME_MAX_IN_PIPE)
         {
            tell_player(p, " Too many names in tag list to evaluate "
                             "...\n");
            cleanup_tag(list_start, matches);
            return 0;
         }
      }
   }
   return matches;
}


/* finds people locally and tags them */

int             local_tag(player * p, char *tag_str)
{
   char           *oldstack, *start;
   player         *tag, **list_start;
   int             matches = 0;
   oldstack = stack;
   align(stack);
   list_start = (player **) ALFIX stack; 

   if (!*tag_str)
   {
      tell_player(p, " No names to tag !.\n");
      return 0;
   }
   if (!strcasecmp(tag_str, "everyone") || !strcasecmp(tag_str, "room"))
   {
      sys_flags |= ROOM_TAG;
      tag = p->location->players_top;
      while (tag)
      {
/*         if ((command_type & EXCLUDE) && (tag->residency & SU))
         {
            tell_player(p, " You cannot exclude SUs\n");
            cleanup_tag(list_start, matches);
            return 0;
         }*/
         *((player **) ALFIX stack) = tag;
         stack += sizeof(player **);
         tag->flags |= TAGGED;
         tag = tag->room_next;
         matches++;
      }
      return matches;
   }
   
   while(*tag_str && *tag_str==',')
     tag_str++;
   if(!*tag_str) {
     tell_player(p, " Badly formed <user_list> (see 'help user_list').\n");
     cleanup_tag(list_start, matches);
     return 0;
   }
   
   while (*tag_str)
   {
      /* skip any extrenious ,'s */
      while(*tag_str && *tag_str==',')
        tag_str++;
      
      start = stack;
      while (*tag_str && *tag_str != ',')
         *stack++ = *tag_str++;
      *stack++ = 0;
      
      /*if (*tag_str)
         tag_str++;*/
      
      if (!strcasecmp(start, "me"))
         tag = p;
      else
         tag = find_player_local(start);
      stack = start;
      
      if (!tag)
      {
         if(!*tag_str) { /* out of tag string */
           if(matches)
             return matches;
           
           cleanup_tag(list_start, matches);
           return 0;
         }
         /* skip to next one */
      }
/*      if ((command_type & EXCLUDE) && (tag->residency & SU))
      {
         tell_player(p, " You cannot exclude SUs.\n");
         cleanup_tag(list_start, matches);
         return 0;
      }*/
      if (tag && !(tag->flags & TAGGED))
      {
         tag->flags |= TAGGED;
         *((player **) ALFIX stack) = tag; 
         stack += sizeof(player *);
         matches++;
         if (matches > NAME_MAX_IN_PIPE)
         {
            tell_player(p, " Too many names in tag list to evaluate ... "
                           "aborting.\n");
            cleanup_tag(list_start, matches);
            return 0;
         }
      }
   }
   return matches;
}


/* finds people in the save file */
int             saved_tag(player * p, char *tag_str, int type)
{
   char           *oldstack, *start;
   saved_player   *tag, **list_start, **scan;
   int             matches = 0, n;
   list_ent	*l, *l2, *l3;
   oldstack = stack;

   align(stack);
   list_start = (saved_player **) ALFIX stack;

   if (!*tag_str)
   {
      tell_player(p, " No names to tag !.\n");
      return 0;
   }
   
   while (*tag_str)
   {
      start = stack;
      while (*tag_str && *tag_str != ',')
        *stack++ = *tag_str++;
      *stack++ = 0;
      lower_case(start);
      if (*tag_str)
         tag_str++;
      if (!strcasecmp(start, "me"))
         tag = p->saved;
      else
         tag = find_saved_player(start);
      if (!tag)
      {
        vtell_player(p, " Can't find player '%s'\n", start);
        stack = start;
      } 
      else
      {
         l = fle_from_save(tag, "friends");
         l2 = fle_from_save(tag, p->lower_name);
         l3 = find_list_entry(p, tag->lower_name);
         
         /* forgot these first two in 2.6x */
         if(tag->residency==STANDARD_ROOMS) {
           vtell_player(p, " [%s is a standard rooms file.]\n", start);
           stack = start;
         }
#ifdef ROBOTS
         else if(tag->residency & ROBOT_PRIV) {
           vtell_player(p, " [%s is a robot and unmailable!]\n", start);
           stack = start;
         }
#endif
         else if (l3 && l3->flags & MAILBLOCK && !(p->residency & ADMIN && type ==0)) {
           vtell_player(p, " [%s is mailblocked by you.]\n", start);
           stack = start;
         }
         else if (l && l->flags & MAILBLOCK && type==1) {
           vtell_player(p, " [%s is blocking all mail to friends.]\n", start);
           stack = start;
         }
         else if(l2 && l2->flags & MAILBLOCK && !(p->residency & ADMIN && type==0)) {
           vtell_player(p, " [%s is blocking mail from you.]\n", start);
           stack = start;
         }
         else
         {
            stack = start;
            for (scan = list_start, n = 0; n < matches; n++)
               if (*scan == tag)
                  break;
            if (*scan != tag)
            {
               *((saved_player **) ALFIX stack) = tag; 
               stack += sizeof(saved_player *);
               matches++;
            }
         }
      }
   }
   return matches;
}


/* stack a string of players in tag list */

char           *you_name_string(player * p, player * tagged)
{
   if (tagged == p)
      return "you";
   if (tagged == current_player)
      switch (tagged->gender)
      {
    case MALE:
       return "himself";
    case FEMALE:
       return "herself";
    case VOID_GENDER:
       return "itself";
    case OTHER:
       return "itself";
      }
   return tagged->name;
}

char           *you_tag_string(player * p, player ** tagged_list, int matches)
{
   char           *start;
   start = stack;
   if (!matches)
   {
      *stack = 0;
      return 0;
   }
   if (sys_flags & EVERYONE_TAG)
   {
      strcpy(stack, "everyone");
      stack = end_string(stack);
      return start;
   }
   if (sys_flags & ROOM_TAG)
   {
      if (p->location == (*pipe_list)->location)
    strcpy(stack, "everyone here");
      else
    sprintf(stack, "everyone in %s", (*pipe_list)->location->name);
      stack = end_string(stack);
      return start;
   }
   strcpy(stack, you_name_string(p, *tagged_list++));
   while (*stack)
      stack++;
   matches--;
   for (; matches > 1; matches--)
   {
      sprintf(stack, ", %s", you_name_string(p, *tagged_list++));
      while (*stack)
    stack++;
   }
   if (matches)
   {
      sprintf(stack, " and %s", you_name_string(p, *tagged_list++));
      while (*stack)
    stack++;
   }
   *stack++ = 0;
   return start;
}

/* works out 'their' for a player */

char           *their_player(player * p)
{
    switch (p->gender)
    {
      case VOID_GENDER:
	return ("its");
      case OTHER:
	return ("its");
      case MALE:
	return ("his");
      case FEMALE:
	return ("her");
      case PLURAL:
	return ("their");
      default:
	return ("its (GENDER FROGGED)");
    }
}


/* works out the 'your' version on a list of people */


void           your_name_string(player * p, player * tagged)
{
   if (tagged == current_player)
   {
      if (p == current_player)
        strcpy(stack, "your");
      else
        strcpy(stack, "my");
      while (*stack)
    stack++;
      return;
   }
   if (tagged == p)
   {
      strcpy(stack, "your");
      while (*stack)
    stack++;
      return;
   }
   strcpy(stack, tagged->name);
   while (*stack)
      stack++;
   if (*(stack - 1) == 's')
      *stack++ = 39;
   else
   {
      *stack++ = 39;
      *stack++ = 's';
   }
}

void            your_string(player * p)
{
   char           *start;
   player        **tagged_list;
   int             matches;
   matches = pipe_matches;
   tagged_list = pipe_list;
   start = stack;
   if (!pipe_matches)
   {
      *stack = 0;
      return;
   }
   if (sys_flags & EVERYONE_TAG)
   {
      strcpy(stack, "everyones'");
      while (*stack)
    stack++;
      return;
   }
   if (sys_flags & ROOM_TAG && p->location == (*pipe_list)->location)
   {
      strcpy(stack, "everyones'");
      while (*stack)
    stack++;
      return;
   }
   your_name_string(p, *tagged_list++);
   matches--;
   for (; matches > 1; matches--)
   {
      *stack++ = ',';
      *stack++ = ' ';
      your_name_string(p, *tagged_list++);
   }
   if (matches)
   {
      strcpy(stack, " and ");
      while (*stack)
    stack++;
      your_name_string(p, *tagged_list++);
   }
   return;
}

/* works out the 'you're' version on a list of people */

void            you_are_string(player * p)
{
   char           *start;
   start = stack;
   if (!pipe_matches)
   {
      *stack = 0;
      return;
   }
   if (sys_flags & EVERYONE_TAG)
   {
      strcpy(stack, "everyone is");
      while (*stack)
    stack++;
      return;
   }
   if (sys_flags & ROOM_TAG)
   {
      if (p->location == (*pipe_list)->location)
    strcpy(stack, "everyone here is");
      else
    sprintf(stack, "everyone in %s is", (*pipe_list)->location->name);
      while (*stack)
    stack++;
      return;
   }
   if (pipe_matches == 1)
   {
      if (*pipe_list == current_player)
        strcpy(stack, "I'm");
      else if (*pipe_list == p)
        strcpy(stack, "you're");
      else
        sprintf(stack, "%s is", (*pipe_list)->name);
      while (*stack)
        stack++;
      return;
   }
   you_tag_string(p, pipe_list, pipe_matches);
   stack--;
   strcpy(stack, " are");
   while (*stack)
      stack++;
   return;
}

/*
 * the routine that does the hard work in a pipe I sure hope the compiler
 * optimises how it should
 */

char           *pipe_cases(player * p, char *str)
{
   if (!isalpha(*(str + 3)) && (*(str + 3) != 39))
   {
      (void) you_tag_string(p, pipe_list, pipe_matches);
      stack--;
      str += 3;
      return str;
   }
   if (tolower(*(str + 3)) == 'r' && (!isalpha(*(str + 4))))
   {
      your_string(p);
      str += 4;
      return str;
   }
   if (tolower(*(str + 3)) == 39 && tolower(*(str + 4)) == 'r' &&
       tolower(*(str + 5)) == 'e' && (!isalpha(*(str + 6))))
   {
      you_are_string(p);
      str += 6;
      return str;
   }
   while (*str && *str != ' ')
      *stack++ = *str++;
   while (*str && *str == ' ')
      *stack++ = *str++;
   return str;
}


char           *do_pipe(player * p, char *str)
{
   char           *fill;
   int             you_matches = 0;
   if (!(sys_flags & PIPE))
      return str;
   fill = stack;
   while (*str)
      if (tolower(*str) == 'y' && tolower(*(str + 1)) == 'o'
     && tolower(*(str + 2)) == 'u')
      {
    str = pipe_cases(p, str);
    you_matches++;
    if (you_matches > YOU_MAX_IN_PIPE)
    {
       tell_current(" Too many inserts in tag ... aborting.\n");
       return 0;
    }
      } else
      {
    while (*str && *str!=' ')
       *stack++ = *str++;
    while (*str && *str==' ')
       *stack++ = *str++;
      }
   *stack++ = 0;
   return fill;
}


/* extract any pipe type command from a string */
void            extract_pipe_global(char *str)
{
  char           *scan, *oldstack, *start = 0;
  oldstack = stack;
  sys_flags |= FAILED_COMMAND;
  for (scan = str; *scan; scan++)
    if (*scan == '|') {
      if (!start) {
        tell_current(" No body message.\n");
        return;
      }
      scan++;
      while (*scan && *scan == ' ')
        scan++;
      if (!*scan)
        return;
      while (*scan && *scan != ' ')
        *stack++ = *scan++;
      while (*scan && *scan == ' ')
        scan++;
      if (!*scan) { 
        *stack++ = 0;
        pipe_list = (player **) ALFIX stack; 
        align(pipe_list);
        pipe_matches = global_tag(current_player, oldstack, 1);
        if (pipe_matches) {
          sys_flags |= PIPE;
          start++;
          *start = 0;
          sys_flags &= ~FAILED_COMMAND;
        } 
        else
          stack = oldstack;
        return;
      } 
      else
        stack = oldstack;
    } 
    else if (*scan != ' ')
      start = scan;
  sys_flags &= ~FAILED_COMMAND;
  stack = oldstack;
  return;
}


/* extract any pipe type command from a string */
void            extract_pipe_local(char *str)
{
  char           *scan, *oldstack, *start = 0;
  oldstack = stack;
  sys_flags |= FAILED_COMMAND;
  for (scan = str; *scan; scan++)
    if (*scan == '|') {
      if (!start) {
        tell_current(" No body message.\n");
        return;
      }
      scan++;
      while (*scan && *scan == ' ')
	scan++;
      if (!*scan)
	return;
      while (*scan && *scan != ' ')
	*stack++ = *scan++;
      while (*scan && *scan == ' ')
	scan++;
      if (!*scan) {
	*stack++ = 0;
        pipe_list = (player **) ALFIX stack; 
        align(pipe_list);
        pipe_matches = local_tag(current_player, oldstack);
        if (pipe_matches) {
	  sys_flags |= PIPE;
          start++;
          *start = 0;
          sys_flags &= ~FAILED_COMMAND;
        }
        else
	  stack = oldstack;
        return;
      }
      else
	stack = oldstack;
    }
    else if (*scan != ' ')
      start = scan;
  sys_flags &= ~FAILED_COMMAND;
  stack = oldstack;
  return;
}


/* who routines */


void swho(player * p, char *str)
{
   player **list, *scan;
   char *oldstack, *text;
   int count = 0;

   oldstack = stack;
   align(stack);
   list = (player **) ALFIX stack; 

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->name[0] && scan->location)
      {
         *(player **) ALFIX stack = scan; 
          stack += sizeof(player **);
          count++;
      }
   }

   if (count == 1 && p->name[0] && p->location) /* logged in, 1 person */
   {
      tell_player(p, " You are all alone *schniff*.\n");
      stack = oldstack;
      return;
   }
   text = stack;
   if(count==1)
     strcpy(stack, " There is one person connected to the program.\n");
   else
     sprintf(stack, " There are %s people connected to the program.\n", number2string(count));
   stack = strchr(stack, 0);

   if ((str && *str == '-') || p->saved_flags & NOPREFIX)
      sys_flags |= NO_PRETITLES;
   construct_name_list_swho(list, count, 1);
   sys_flags &= ~NO_PRETITLES;
   *stack++ = 0;
   if (p->flags & NO_PAGER || !(p->logged_in))
      tell_player(p, text);
   else
      pager(p, text, 0);
   stack = oldstack;
}

void qwho(player * p, char *str)
{
   player **list, *f;
   list_ent *l;
   int count = 0;
   char *oldstack, *text;
   int sus=0,newbies=0;
   player *scan;
   
   if (!p->saved || (!p->saved->list_top))
   {
      tell_player(p, " This command requires that you have a list.\n");
      return;
   }
   l = find_list_entry(p, "everyone");
   if (l && l->flags & FRIEND)
   {
      swho(p, str);
      return;
   }
   oldstack = stack;

   align(stack);
   list = (player **) ALFIX stack;

   l=find_list_entry(p,"sus");
   if (l && l->flags & FRIEND)
     {
       sus=1;
       for (scan = flatlist_start; scan; scan = scan->flat_next)
	 if(scan->residency & PSU && !(scan->flags & BLOCK_SU))
	   {
	     *(player **) ALFIX stack = scan; 
	     stack += sizeof(player **);
	     count++;
	   }
     }

   l=find_list_entry(p,"newbies");
   if (l && l->flags & FRIEND)
     {
       newbies=1;
       for (scan = flatlist_start; scan; scan = scan->flat_next)
         if(scan->residency == NON_RESIDENT)
           {
             *(player **) ALFIX stack = scan; 
             stack += sizeof(player **);
             count++;
           }
     }

   for (l = p->saved->list_top; l; l = l->next)
     {
       if (l->flags & FRIEND)
      {
         f = find_player_absolute_quiet(l->name);
         if (f)
	   if (!((sus && f->residency & SU) ||
		 (newbies && f->residency==NON_RESIDENT)))
	     {
	       *(player **) ALFIX stack = f; 
	       stack += sizeof(player **);
	       count++;
	     }
       }
    }

   if (!count)
   {
      tell_player(p, " None of your friends are on at the moment.\n");
      stack = oldstack;
      return;
   }
   text = stack;
   if (count == 1)
      strcpy(stack, " You have just one friend on at the moment.\n");
   else
      sprintf(stack, " You have %s friends on at the moment.\n",
         number2string(count));
   while (*stack)
      stack++;
   if (!str || *str == '-' || p->saved_flags & NOPREFIX)
   {
      sys_flags |= NO_PRETITLES;
   }
   construct_name_list(list, count);
   sys_flags &= ~NO_PRETITLES;
   *stack++ = 0;
   pager(p, text, 0);
   stack = oldstack;
}

/* put something in the middle of the screen */

void pstack_mid(char *str)
{
   int half, n;

   half = (68 - (int)strlen(str)) >> 1;
   for (n = 0; n < half; n++)
   {
      *stack++ = '-';
   }
   *stack++ = ' ';
   for (n++; *str; n++)
   {
      *stack++ = *str++;
   }
   *stack++ = ' ';
   for (n++; n < 79; n++)
   {
      *stack++ = '-';
   }
   *stack++ = '\n';
}

/* the who command */

void            who(player * p, char *str)
{
   char           *oldstack, middle[80];
   player         *scan;
   int             page, pages, count;
   oldstack = stack;

#ifdef INTERCOM
   if (*str && *str=='@' && *(str+1))
   {
     do_intercom_who(p, str);
     return;
   }
#endif

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
      if (!scan)
      {
    tell_player(p, " Bad who listing, abort.\n");
    log("error", "Bad who list (tag.c)");
    stack = oldstack;
    return;
      } else if (scan->name[0])
    count--;
   for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
      if (scan->name[0] && scan->location)
      {
    strcpy(stack, scan->name);
    while (*stack)
       stack++;
    *stack++ = ' ';
    strcpy(stack, scan->title);
    while (*stack)
       stack++;
    *stack++ = '\n';
    count++;
      }
   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   pstack_mid(middle);
   *stack++ = 0;
   tell_player(p, oldstack);

   stack = oldstack;
}




/* whos with title */

void            twho(player * p, char *str)
{
   int             yessess = 0;
   if (p->saved_flags & YES_SESSION)
      yessess = 1;
   p->saved_flags |= YES_SESSION;
   who(p, str);
   if (!yessess)
      p->saved_flags &= ~YES_SESSION;
}




char           *idlingstring(player * p, player ** tagged_list, int matches)
{
   char           *start;
   player         *temp;
   start = stack;

   if (!tagged_list)
      return 0;

   if (!matches || sys_flags & EVERYONE_TAG || sys_flags & ROOM_TAG ||
       sys_flags & FRIEND_TAG)
   {
      return 0;
   }
   for (; matches > 0; matches--)
   {
      temp = find_player_global_quiet(name_string(p, *tagged_list++));
      if (temp && temp->idle_msg[0] != 0)
      {
    sprintf(stack, "  Idling> %s %s\n", temp->name, temp->idle_msg);
    while (*stack)
       stack++;
      }
   }
   *stack++ = 0;
   if (!*start)
      return "";

   return start;
}


void            set_yes_session(player * p, char *str)
{
   if (p->saved_flags & YES_SESSION)
      p->saved_flags &= ~YES_SESSION;
   else
      p->saved_flags |= YES_SESSION;

   if (p->saved_flags & YES_SESSION)
      tell_player(p, " You will now see titles with 'who'\n");
   else
      tell_player(p, " You start to see sessions with 'who'\n");
}

/* version of walling for news report ppl only */
void            news_wall_but(player * p, char *str)
{
   player         *scan;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->saved_flags & NEWS_INFORM && scan != p)
      {
            command_type |= (HIGHLIGHT|PERSONAL|WARNING);
            tell_player(scan, str);
            command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
      }
   }
}


/* version of walling for snews report ppl only */
void            snews_wall_but(player * p, char *str)
{
   player         *scan;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->residency & SU && scan->saved_flags & SNEWS_INFORM && scan != p)
      {
            command_type |= (HIGHLIGHT|PERSONAL|WARNING);
            tell_player(scan, str);
            command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);
      }
   }
}



/*version of global_tag to be mutilated to write friend_tag */
/*p2 is the persons whos list we are tagging, p is the person we want to omit*/
int friend_tag_but(player * p, player *p2)
{
    list_ent *entry, *l;
    char *oldstack;
    player *tag, **list_start;
    int matches = 0;
    
    oldstack = stack;
    align(stack);
    list_start = (player **) ALFIX stack; 
    
    sys_flags |= FRIEND_TAG;
    entry = p2->saved->list_top;
    while (entry)
    {
	if (!(!strcasecmp(entry->name, p->name)) && 
	    entry->flags & FRIEND && (entry->name != "everyone" && strcasecmp(entry->name, p2->name)))
	{
	    tag = find_player_absolute_quiet(entry->name);
	    if (tag && tag->location)
	    {
		l = find_list_entry(tag,p2->name);
		if (!((l && (l->flags & FRNDBLOCK || l->flags & BLOCK || l->flags & IGNORE))))
		{
		    *((player **) ALFIX stack) = tag;
		    stack += sizeof(player **);
		    tag->flags |= TAGGED;
		    matches++;
		}
	    }
	}
	entry = entry->next;
    }
    if (matches == 0)
	tell_player(p, "They have no other non-friendblocking friends on at the moment.\n");
    return matches;
}

