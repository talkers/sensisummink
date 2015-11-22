/*
 * colour_socket.c
 * working title - harry the hedgehog was a happy serial killer.c
 * ansi colour version of code to write out to a player
 */
 
#define COL_VERS "1.2"

#include "colours.h"

const char *cols[17] = {"", BLK, RED, GRN, BLU, YEL, MAG, CYN, WHT, HBLK, HRED, HGRN, HBLU, HYEL, HMAG, HCYN, HWHT};
const char *col_names[17] = {"None", "Black", "Red", "Green", "Blue", "Yellow", "Purple", "Cyan", "White", "LBlack", "LRed", "LGreen", "LBlue", "LYellow", "LPurple", "LCyan", "LWhite"};


/* OK, this is copied in from the socket.c that runs ok in the live*/
file            colour_process_output(player * p, char *str)
{
   int i, hi = 0, nline=0, aftercol;
   char *save;
   file o;

   o.where = stack;
   o.length = 0;

   if(command_type & CHANNEL && p->colours[6]!=0) {
         strcpy(stack, cols[p->colours[6]]);
         while(*stack) {
           stack++;
           o.length++;
         }
      }
   aftercol = o.length;
#ifdef CRASH_RECOVER
   if ((p != current_player || p->flags & FORCE_COLOUR) && !(sys_flags & CRASH_RECOVERING))
#else
   if (p != current_player || p->flags & FORCE_COLOUR)
#endif
   {
      /* colours. *sigh* */
      if((command_type & PERSONAL || p->flags & TAGGED) && 
        !(sys_flags & EVERYONE_TAG || sys_flags & ROOM_TAG)) {
        if(sys_flags & FRIEND_TAG) {
          if(p->colours[2]!=0) {
            strcpy(stack, cols[p->colours[2]]);
            hi=1; /* now used as a 'we put a colour in' thing */
          }
        }
        else {
          if(p->colours[1]!=0) {
            strcpy(stack, cols[p->colours[1]]);
            hi=1;
          }
        }
      }
      else if(command_type & EVERYONE || sys_flags & EVERYONE_TAG) { /* shout*/
        if(p->colours[5]!=0) {
          strcpy(stack, cols[p->colours[5]]);
          hi=1;
        }
      }
      else if(command_type & AUTO) {
        if(p->colours[4]!=0) {
          strcpy(stack, cols[p->colours[4]]);
          hi=1;
        }
      }
      else if(command_type & ROOM || sys_flags & ROOM_TAG) {
        if(p->colours[3]!=0) {
          strcpy(stack, cols[p->colours[3]]);
          hi=1;
        }
      }
      if(hi)
        while(*stack) {
          stack++;
          o.length++;
        }
      aftercol = o.length;
      save = stack;
      /* tags */  
      if (command_type & ECHO_COM && p->saved_flags & TAG_ECHO)
      {
         *stack++ = '+';
         o.length++;
      }
      if ((command_type & PERSONAL ||
           p->flags & TAGGED) && p->saved_flags & TAG_PERSONAL &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG) || (command_type & WARNING)))
      {
         if (sys_flags & FRIEND_TAG)
         {
            if(command_type & TFREPLY)
               *stack++ = '}';
            else
               *stack++ = '*';
         } 
         else if (sys_flags & REPLY_TAG || command_type & MULTITELL)
         {
            *stack++ = '&';
         } else
         {
            *stack++ = '>';
         }
         o.length++;
      }
      if ((command_type & EVERYONE || sys_flags & EVERYONE_TAG) &&
          p->saved_flags & TAG_SHOUT)
      {
         *stack++ = '!';
         o.length++;
      }
      if ((command_type & ROOM || sys_flags & ROOM_TAG)
          && p->saved_flags & TAG_ROOM && !(command_type & ECHO_COM))
      {
         *stack++ = '-';
         o.length++;
      }
      if (command_type & AUTO && p->saved_flags & TAG_AUTOS)
      {
         *stack++ = '#';
         o.length++;
      }
      if (stack != save)
      {
         *stack++ = ' ';
         o.length++;
      }
      if (command_type & ECHO_COM && p->saved_flags & SEEECHO &&
      (command_type & PERSONAL || (p->location && p->location->flags & OPEN)))
      {
         sprintf(stack, "[%s] ", current_player->name);
         while (*stack)
         {
            stack++;
            o.length++;
         }
      }
   }
   p->column += (o.length-aftercol);

   while (*str)
   {
      switch (*str)
      {
         case '\n':
            if(!(*(str+1))) { /* no more text, fall out */
              str++;
              nline=1;
              break;
            }
            *stack++ = '\r';
            *stack++ = '\n';
            p->column = 0;
            str++;
            o.length += 2;
            break;
         default:
            if (p->term_width && (p->column >= p->term_width))
            {
               for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
                  if (isspace(*(--str)))
                     break;
               if (i != p->word_wrap)
               {
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  str++;
                  o.length += 5;
               } else
               {
                  for (; i; stack++, str++, o.length++)
                     i--;
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  o.length += 5;
               }
       }
       *stack++ = *str++;
       o.length++;
       p->column++;
       break;
      }
   }
   /* reset any highlighting or colouring */
   strcpy(stack, NOR);
   while(*stack) {
     stack++;
     o.length++;
   }
   /* reset to their default text colour if any */
   if(p->colours[0]!=0 && !(p->flags & CHUCKOUT || sys_flags & SHUTDOWN)) {
     strcpy(stack, cols[p->colours[0]]);
     while(*stack) {
       stack++;
       o.length++;
     }
   }
   /* append final newline if required - pity anyfunction that doesnt have this */
   if(nline) {
     *stack++ = '\r';
     *stack++ = '\n';
     o.length +=2;
   }
   p->column=0;
   return o;
}


/* OK, this is copied in from the socket.c that runs ok in the live*/
file            process_output(player * p, char *str)
{
   int i, hi = 0, afterhi=0;
   char *save;
   file o;

   /* check for whether or not user wants colours */
   if(p->sflags & SEE_COLOUR && p->term)
     return colour_process_output(p, str);
     
   o.where = stack;
   o.length = 0;

#ifdef CRASH_RECOVER
   if ((p != current_player || p->flags & FORCE_COLOUR) && !(sys_flags & CRASH_RECOVERING))
#else
   if (p != current_player || p->flags & FORCE_COLOUR)
#endif
   {

      if ((command_type & PERSONAL || p->flags & TAGGED) && p->term &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG)))
      {
         strcpy(stack, terms[(p->term) - 1].bold);
         while (*stack)
         {
            stack++;
            o.length++;
         }
         hi = 1;
      }
      afterhi = o.length;
      save = stack;
      if (command_type & ECHO_COM && p->saved_flags & TAG_ECHO)
      {
         *stack++ = '+';
         o.length++;
      }
      if ((command_type & PERSONAL ||
           p->flags & TAGGED) && p->saved_flags & TAG_PERSONAL &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG) || (command_type & WARNING)))
      {
         if (sys_flags & FRIEND_TAG)
         {
            if(command_type & TFREPLY)
               *stack++ = '}';
            else
               *stack++ = '*';
         } 
         else if (sys_flags & REPLY_TAG || command_type & MULTITELL)
         {
            *stack++ = '&';
         } else
         {
            *stack++ = '>';
         }
         o.length++;
      }
      if ((command_type & EVERYONE || sys_flags & EVERYONE_TAG) &&
          p->saved_flags & TAG_SHOUT)
      {
         *stack++ = '!';
         o.length++;
      }
      if ((command_type & ROOM || sys_flags & ROOM_TAG)
          && p->saved_flags & TAG_ROOM && !(command_type & ECHO_COM))
      {
         *stack++ = '-';
         o.length++;
      }
      if (command_type & AUTO && p->saved_flags & TAG_AUTOS)
      {
         *stack++ = '#';
         o.length++;
      }
      if (stack != save)
      {
         *stack++ = ' ';
         o.length++;
      }
      if (command_type & ECHO_COM && p->saved_flags & SEEECHO &&
      (command_type & PERSONAL || (p->location && p->location->flags & OPEN)))
      {
         sprintf(stack, "[%s] ", current_player->name);
         while (*stack)
         {
            stack++;
            o.length++;
         }
      }
   }
   if ((!hi) && (command_type & HIGHLIGHT) && (p->term))
   {
      strcpy(stack, terms[(p->term) - 1].bold);
      while (*stack)
      {
         stack++;
         o.length++;
         afterhi++;
      }
      hi = 1;
   }
   p->column += (o.length-afterhi);

   while (*str)
   {
      switch (*str)
      {
         case '\n':
            if (hi)
            {
               strcpy(stack, terms[(p->term) - 1].off);
               while (*stack)
               {
                  stack++;
                  o.length++;
               }
               hi = 0;
            }
            *stack++ = '\r';
            *stack++ = '\n';
            p->column = 0;
            str++;
            o.length += 2;
            break;
         default:
            if (p->term_width && (p->column >= p->term_width))
            {
               for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
                  if (isspace(*(--str)))
                     break;
               if (i != p->word_wrap)
               {
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  str++;
                  o.length += 5;
               } else
               {
                  for (; i; stack++, str++, o.length++)
                     i--;
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  o.length += 5;
               }
       }
       *stack++ = *str++;
       o.length++;
       p->column++;
       break;
      }
   }
   p->column = 0;
   return o;
}



void            clear_screen(player * p, char *str)
{
   if (p->term)
      tell_player(p, terms[(p->term) - 1].cls);
   else
      tell_player(p, " You have to have a termtype set for this command to"
        " work. Use the command: termtype <termtype>\n");
}


/* the hitells command */
void            termtype(player * p, char *str)
{
   char           *oldstack;
   int             i;

   oldstack = stack;
   if (!*str && !(p->term))
   {
      tell_player(p, " Format: termtype <termtype/?/off>\n");
      return;
   }
   if (!*str)
   {
      sprintf(stack, " Termtype currently set to %s.\n",
         terms[(p->term) - 1].name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (*str == '?')
   {
      strcpy(stack, " Current terminal types available : ");
      stack = strchr(stack, 0);
      for (i = 0; terms[i].name[0]!='\0'; i++)
      {
    sprintf(stack, "%s, ", terms[i].name);
    stack = strchr(stack, 0);;
      }
      stack -= 2;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (!strcasecmp("off", str))
   {
      p->term = 0;
      if(p->sflags & SEE_COLOUR) {
        p->sflags &= ~SEE_COLOUR;
        tell_player(p, " Colours turned off.\n");
        tell_player(p, NOR);
      }
      tell_player(p, " Termtype turned off.\n");
      stack = oldstack;
      return;
   }
   for (i = 0;terms[i].name[0]!='\0'; i++)
      if (!strcasecmp(str, terms[i].name))
      {
    p->term = i + 1;
    sprintf(stack, " Termtype now set to %s.\n",
            terms[i].name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
      }
   sprintf(stack, " Terminal type '%s' not supported.\n"
      " Do termtype '?' to list currently supported terminals.\n", str);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
   return;
}

/* the command functions associated with colour */
void		test_colours(player *p, char *str)
{
  char *oldstack;
  
  /* this is going to be amusing if they dont have a terminal type, so */
  if(!p->term) {
    tell_player(p, " Sorry - you can't use colours without a terminal type.\n");
    return;
  }
  
  oldstack = stack;
  vtell_player(p, "This is the currently implemented list of colours on %s, if you "
  		 " can see all or most of the colours, then you may use the 'colour "
  		 "on' command to enable ansi colours, and use the 'setcolour' command "
  		 "to set what messages are coloured in what way.\n\n"
  		 "%sBlack     %sLBlack\n%s"
  		 "%sRed       %sLRed\n%s"
  		 "%sGreen     %sLGreen\n%s"
  		 "%sBlue      %sLBlue\n%s"
  		 "%sYellow    %sLYellow\n%s"
  		 "%sPurple    %sLPurple\n%s"
  		 "%sCyan      %sLCyan\n%s"
  		 "%sWhite     %sLWhite\n%s",
  		 TALKER_NAME, BLK, HBLK, NOR, 
  		 RED, HRED, NOR, 
  		 GRN, HGRN, NOR, 
  		 BLU, HBLU, NOR, 
  		 YEL, HYEL, NOR, 
  		 MAG, HMAG, NOR, 
  		 CYN, HCYN, NOR, 
  		 WHT, HWHT, NOR);
}


/* command to toggle colours on and off */
void	toggle_colours(player *p, char *str)
{
  if(!p->term) {
    tell_player(p, " You can't use ansi colours without having a terminal type (type 'termtype ?')\n");
    return;
  }    
  
  if(!strcasecmp(str, "off")) 
    p->sflags &= ~SEE_COLOUR;
  else if(!strcasecmp(str, "on"))
    p->sflags |= SEE_COLOUR;
  else
    p->sflags ^= SEE_COLOUR;
  
  if(p->sflags & SEE_COLOUR)
    tell_player(p, " You are now seeing colour output.\n");
  else {
    tell_player(p, " You are now ignoring colour output.\n");
    tell_player(p, NOR);
  }
}


/* ok, huge monster command to set what type of message is in what colour
   should accept 'wipe' to erase all colours, 'check' for the person to
   see what colours they have currently got set and 'type' 'col' */
void		set_colours(player *p, char *str)
{
  int count, setme, setto = -1;
  char *oldstack, *colset, creset[12];
  
  if(!p->term) {
    tell_player(p, " You can't use colour without a termtype.\n");
    return;
  }
  
  if(!*str) { /* format return */
    if(p->residency & PSU)
      tell_player(p, " Format: setcolour check/wipe/<type> <colour/off>\n\n"
    		   " Available message types are: normal, tell, friend, room, auto, shout, channel\n"
    		   " Available colours can be seen with 'testcolour'\n");
    else
      tell_player(p, " Format: setcolour check/wipe/<type> <colour/off>\n\n"
    		   " Available message types are: normal, tell, friend, room, auto, shout\n"
    		   " Available colours can be seen with 'testcolour'\n");
    return;
  }
  
  /* check for wipe */
  if(!strcasecmp(str, "wipe")) { /* kill all colours */
    for(count = 0;count<MAX_COLS;count++)
      p->colours[count] = 0;
    tell_player(p, " Colour settings wiped.\n");
    return;
  }
  
  oldstack = stack;
  
  /* check for check! :-) */
  if(!strcasecmp(str, "check")) {
    if(p->colours[0]!=0 && p->sflags & SEE_COLOUR)
      strcpy(creset, cols[p->colours[0]]);
    else
      strcpy(creset, NOR);
    strcpy(stack, " Your currently coloured message types:\n");
    stack = strchr(stack, 0);
    sprintf(stack, "  Your normal text colour is set to %s%s%s%s\n", cols[p->colours[0]], col_names[p->colours[0]], NOR, creset);
    stack = strchr(stack, 0);
    if(p->colours[1]!=0) {
      sprintf(stack, "  Tells are set to %s%s%s%s\n", cols[p->colours[1]], col_names[p->colours[1]], NOR, creset);
      stack = strchr(stack, 0);
    }
    if(p->colours[2]!=0) {
      sprintf(stack, "  Friend tells are set to %s%s%s%s\n", cols[p->colours[2]], col_names[p->colours[2]], NOR, creset);
      stack = strchr(stack, 0);
    }
    if(p->colours[3]!=0) {
      sprintf(stack, "  Room messages are set to %s%s%s%s\n", cols[p->colours[3]], col_names[p->colours[3]], NOR, creset);
      stack = strchr(stack, 0);
    }
    if(p->colours[4]!=0) {
      sprintf(stack, "  Automessages are set to %s%s%s%s\n", cols[p->colours[4]], col_names[p->colours[4]], NOR, creset);
      stack = strchr(stack, 0);
    }
    if(p->colours[5]!=0) {
      sprintf(stack, "  Shouts set to %s%s%s%s\n", cols[p->colours[5]], col_names[p->colours[5]], NOR, creset);
      stack = strchr(stack, 0);
    }
    if(p->colours[6]!=0) {
      sprintf(stack, "  Channels are set to %s%s%s%s\n", cols[p->colours[6]], col_names[p->colours[6]], NOR, creset);
      stack = strchr(stack, 0);
    }
    *stack++=0;
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  
  /* must want to set something */
  colset = next_space(str);
  if(*colset) *colset++ = 0;
  if(!*colset) {
    if(p->residency & PSU)
      tell_player(p, " Format: setcolour check/wipe/<type> <colour/off>\n\n"
    		   " Available message types are: normal, tell, friend, room, auto, shout, channel\n"
    		   " Available colours can be seen with 'testcolour'\n");
    else
      tell_player(p, " Format: setcolour check/wipe/<type> <colour/off>\n\n"
    		   " Available message types are: normal, tell, friend, room, auto, shout\n"
    		   " Available colours can be seen with 'testcolour'\n");
    return;
  } /* again.. BAD repetition ;) */
  
  /* what do they want to change? */
  if(!strcasecmp(str, "normal"))	setme = 0;
  else if(!strcasecmp(str, "tell")) 	setme = 1;
  else if(!strcasecmp(str, "friend"))	setme = 2;
  else if(!strcasecmp(str, "room"))	setme = 3;
  else if(!strcasecmp(str, "auto"))	setme = 4;
  else if(!strcasecmp(str, "shout"))	setme = 5;
  else if(!strcasecmp(str, "channel"))  setme = 6;
  else {
    tell_player(p, " Not a valid colour type to set.\n");
    return;
  }
  
  /* what do they want to change it to? */
  if(!strcasecmp(colset, "off")) 
    setto = 0;
  else {  
    for(count = 0;count<17;count++)
#ifdef SUNOS
       if(!strcasecmp(colset, (char *)&col_names[count])) {
#else
       if(!strcasecmp(colset, col_names[count])) {
#endif
        setto = count;
        break;
      }
    if(setto==-1) {
      tell_player(p, " Check 'testcolour' to see what the valid colours are.\n");
      return;
    }
  }
  
  /* make the change */
    p->colours[setme] = setto; 
  sprintf(stack, " Your %s messages are now %s%s%s\n", str, cols[setto], col_names[setto], NOR);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/*
 o - none/off 
 1 - tell
 2 - friend
 3 - room
 4 - auto
 5 - shout
 6 - channel */

void ansi_cols_version(void)
{
  sprintf(stack, " -=> SensiSummink ansi colours V%s enabled.\n", COL_VERS);
  stack = strchr(stack, 0);
}
