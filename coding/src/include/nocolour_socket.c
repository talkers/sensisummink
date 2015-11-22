/*
 * nocolour_socket.c
 * plain version of code that writes out to a player
 */
 

/* OK, this is copied in from the socket.c that runs ok in the live*/
file            process_output(player * p, char *str)
{
   int i, hi = 0, afterhi = 0;
   char *save;
   file o;

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
            afterhi++;
         }
         hi = 1;
      }
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

