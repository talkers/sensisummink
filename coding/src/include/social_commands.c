/*
 * socials.c - i fucking HATE socials, but the (l)users who cant type or
 * use pipe seem to love them, so, to smite them, here are socials implemented
 * using: emote with pipe ;)   Slaine...
 */
 
#define SOC_VERS "1.3b"
 
/* btw - this gets included into commands.c - muahahahaha and stuff. */
struct soc_struct {
  char		*name;
  char		*string;
  char		*format;
};
typedef struct soc_struct soc;

/* set up arrays as shown for descriptive words.  If you add any more, then
   remember to update the code in the switch statement somewhere below to
   know they can be used.  In the current arrays, word_list4 has spaces
   before things so we can have 'a fish' and 'an antelope' - ie grammar */
char *word_list1[] = {"evilly", "mischievously", "toothily", "derangedly", 0};
char *word_list2[] = {"happily", "cheerfully", "warmly", "brightly", 0};
char *word_list3[] = {"blankly", "thoughtfully", "strangely", 0};
char *word_list4[] = {"n alarmed", " spiky", " scary", " fluffy", "n inflatable", 0};
char *word_list5[] = {"fish", "kitten", "wok", "sheep", 0};

/* list of socials commands - format is:
{"name of social", "string to execute", "format return string", descriptives list to use if needed (0-n},
   the name of the social should be the same as its command line entry, the
   string to execute should just be a message string.  The code works out
   whether you want a personal, a room, or a piped room message by the order
   of the arguments given.  If you specify a |, it means the social can be piped,
   or in other words, that the user can do it on its own, or give the names of
   some users.  This translates into it allowing if required the user to 
   give a text string.  * translates literally as 'insert users text here' -
   if it follows a | directly it means the user will have to give name(s) of
   people to direct the social at.  $n means 'insert a random word from the
   specified words list 'n' here, so if descriptives4 existed, $4 would give
   you a random word from it.  The format return is what you want users to
   see if they give a string when it wasnt needed, or no string when it was.
   
   Adding these to clist.h is easy, just do it as normal, but always have 
   'social_command' as the function to call, and make sure you have this
   part set up right. */
soc socials[] =	{
			{"grin", "grins $1 |", " Format: grin [player(s)]\n"},
			{"smile", "smiles $2 |*", " Format: smile <player(s)>\n"},
			{"bop", "* bops you $3 with a$4 $5", " Format: bop <player(s)>\n"},
			{"ponder", "ponders", " Format: ponder\n"},
			{0, 0, 0}
		};


void	social_command(player *p, char *str)
{
   char *oldstack, *scan;
   soc  *runme;
   int count;
   
   oldstack = stack;

   for(runme=socials; runme->name; runme++)
     if(p->command_used && !strcmp(p->command_used->text, runme->name))
       break;
   if(!runme->name) {
     vtell_player(p, " Missing social '%s'.\n", p->command_used->text);
     vlog("socials", "Missing social '%s'", p->command_used->text);
     return;
   }

   /* ok, we have the social to use, try a format check for users! */
   if((str && *str && strchr(str, '|')) || 
      (strchr(runme->string, '*') && !*str)) { /* player/s wanted for room social (pipe) */
     tell_player(p, runme->format);
     return;
   }
   
   scan = runme->string;
   while(*scan) {
     if(*scan=='*') {
       strcpy(stack, str);
       stack = strchr(stack, 0);
     }
     else if(*scan=='$') {
       switch(*(scan+1)) {
         case 0:
                stack = oldstack;
                vlog("socials", "Incorrect social '%s' (missing word list)", runme->name);
                vtell_player(p, " Sorry - this social is incorrect, report this message to admin.\n");
		return;
         case '1':
         	for(count=0; word_list1[count]; count++);
         	strcpy(stack, word_list1[rand()%count]);
         	break;
         case '2':
         	for(count=0; word_list2[count]; count++);
         	strcpy(stack, word_list2[rand()%count]);
         	break;
         case '3':
         	for(count=0; word_list3[count]; count++);
         	strcpy(stack, word_list3[rand()%count]);
         	break;
         case '4':
                for(count=0; word_list4[count]; count++);
                strcpy(stack, word_list4[rand()%count]);
                break;
         case '5':
                for(count=0; word_list5[count]; count++);
                strcpy(stack, word_list5[rand()%count]);
                break;
         default:
                stack = oldstack;
                vlog("socials", "Incorrect social '%s' (wrong word list)", runme->name);
                vtell_player(p, " Sorry - this social is incorrect, report this message to admin.\n");
		return;
       }
       scan++;
       stack = strchr(stack, 0);
     }
     else if(*scan=='|') {
       if(*str) {
         sprintf(stack, "at you | %s", str);
         stack = strchr(stack, 0);
       }
       if(*(scan+1)=='*')
         scan++;
     }
     else
       *stack++ = *scan;
     scan++;
   }
   *stack++=0;
   if(strchr(runme->string, '*')) { /* was remote */
     if(strchr(runme->string, '|'))
       emote(p, oldstack);
     else
       remote(p, oldstack);
   }
   else
     emote(p, oldstack);
   stack = oldstack;
}


void	socials_version(void)
{
   sprintf(stack, " -=> SensiSummink socials V%s enabled.\n", SOC_VERS);
   stack = strchr(stack, 0);
}

