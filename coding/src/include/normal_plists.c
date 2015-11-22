
/* write one player file out */

void            write_to_file(saved_player * sp)
{
   char           *oldstack;
   int             length;
   oldstack = stack;
   
   if(!sp)
     return;
   if(sp->residency==STANDARD_ROOMS || sp->residency & NO_SYNC)
     return;
     
   if (sys_flags & VERBOSE && sys_flags & PANIC)
   {
      sprintf(oldstack, "Attempting to write player '%s'.", sp->lower_name);
      stack = end_string(oldstack);
      log("sync", oldstack);
      stack = oldstack;
   }
   /* in main plists.c */
   length = write_to_file_common(sp);
}



/* init everything needed for the plist file */
void            init_plist(void)
{
   char           *oldstack;
   int             i;

   oldstack = stack;
   vlog("boot", "SensiSummink V%s Booting...", VERSION);
   log("boot", "Loading player files...");
   hard_load_files();
   for (i = 0; i < 26; i++)
      update[i] = 0;

   stack = oldstack;
}



/* extract one player */

void           extract_player(char *where, int length)
{
   int             len, sum;
   char           *oldstack, *c;
   saved_player   *old, *sp, **hash;

   oldstack = stack;
   where = get_int(&len, where);
   where = get_string(oldstack, where);
   stack = end_string(oldstack);
   old = find_saved_player(oldstack);
   sp = old;
   if (!old)
   {
      sp = (saved_player *) MALLOC(sizeof(saved_player));
      memset((char *) sp, 0, sizeof(saved_player));
      strncpy(sp->lower_name, oldstack, MAX_NAME);
      sp->saved_email[0]=0;
      strncpy(player_loading, sp->lower_name, MAX_NAME);
      sp->rooms = 0;
      sp->mail_sent = 0;
      sp->received_list = 0;
      sp->list_top = 0;
      hash = saved_hash[((int) sp->lower_name[0] - (int) 'a')];
      for (sum = 0, c = sp->lower_name; *c; c++)
         sum += (int) (*c) - 'a';
      hash = (hash + (sum % HASH_SIZE));
      sp->next = *hash;
      *hash = sp;
   }
   else if(old->residency==STANDARD_ROOMS) {
     sp->last_host[0] = 0;
     stack = oldstack;
     return;
   }
   where = get_int(&sp->last_on, where);
   where = get_int(&sp->saved_flags, where);
   where = get_int(&sp->residency, where);

   if (sp->residency == BANISHD)
   {
      sp->last_host[0] = 0;
      sp->data.where = 0;
      sp->data.length = 0;
      stack = oldstack;
      return;
   }
   /*
    * This bit controls when idle players get wiped. After summer hols, this
    * bit MUST be uncommented
    */

     if ( ((time(0)-(sp->last_on)) > PLAYER_TIMEOUT) &&
       !(sp->residency&NO_TIMEOUT))
     {
       log("timeouts", sp->lower_name);
       remove_player_file(sp->lower_name);
       stack=oldstack;
       return;
     }

/* PUT ANYTHING TO CHANGE RESIDENCY OR OTHER FLAGS HERE */
   where = get_string(sp->last_host, where);
   where = get_string(sp->saved_email, where);
   where = get_int(&sp->data.length, where);
   sp->data.where = (char *) MALLOC(sp->data.length);
   memcpy(sp->data.where, where, sp->data.length);
   where += sp->data.length;
   where = retrieve_room_data(sp, where);
   where = retrieve_list_data(sp, where);
   where = retrieve_mail_data(sp, where);
   stack = oldstack;
}


/* hard load in on player file */
void            hard_load_one_file(char c)
{
   char           *oldstack, *where, *scan;
   int             fd, length, len2, i, fromjmp;

   oldstack = stack;
   if (sys_flags & VERBOSE)
   {
      sprintf(oldstack, "Loading player file '%c'.", c);
      stack = end_string(oldstack);
      log("boot", oldstack);
      stack = oldstack;
   }
   sprintf(oldstack, "files/players/%c", c);
   fd = open(oldstack, O_RDONLY | O_NDELAY);
   if (fd < 0)
   {
      sprintf(oldstack, "Failed to load player file '%c'", c);
      stack = end_string(oldstack);
      log("error", oldstack);
   } else
   {
      length = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
      if (length)
      {
         where = (char *) MALLOC(length);
         if (read(fd, where, length) < 0)
            handle_error("Can't read player file.");
         for (i = 0, scan = where; i < length;)
         {
            get_int(&len2, scan);
            fromjmp = setjmp(jmp_env);
            if (!fromjmp)
            {
               extract_player(scan, len2);
            } else
            {
	       fromjmp = setjmp(jmp_env);
	       if (!fromjmp)
	       {
                  sprintf(oldstack, "Bad Player \'%s\' deleted on load.",
                          player_loading);
                  stack = end_string(oldstack);
                  log("boot", oldstack);
                  stack = oldstack;
                  remove_player_file(player_loading);
	       } else
	       {
	       /* Oh FUCK, couldn't even delete the bad player */
	          log("boot", "Delete of bad player failed!");
	       }
            }
            i += len2;
            scan += len2;
         }
         FREE(where);
      }
      close(fd);
   }
   stack = oldstack;
}



/* sync player files corresponding to one letter */

void            sync_to_file(char c, int background)
{
   saved_player   *scan, **hash;
   char           *oldstack;
   int             fd, i, length;

   if (background && fork())
      return;

   oldstack = stack;
   if (sys_flags & VERBOSE)
   {
      sprintf(oldstack, "Syncing File '%c'.", c);
      stack = end_string(oldstack);
      log("sync", oldstack);
      stack = oldstack;
   }
   hash = saved_hash[((int) c - (int) 'a')];
   for (i = 0; i < HASH_SIZE; i++, hash++)
      for (scan = *hash; scan; scan = scan->next)
        write_to_file(scan);
   length = (int) stack - (int) oldstack;


   /* test that you can write out a file ok */

   strcpy(stack, "files/players/backup_write");
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
   if (fd < 0)
      handle_error("Primary open failed (player back)");
   if (write(fd, oldstack, length) < 0)
      handle_error("Primary write failed "
         "(playerback)");
   close(fd);

   sprintf(stack, "files/players/%c", c);
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
   if (fd < 0)
      handle_error("Failed to open player file.");
   if (write(fd, oldstack, length) < 0)
      handle_error("Failed to write player file.");
   close(fd);
   update[(int) c - (int) 'a'] = 0;
   stack = oldstack;

   if (background)
      exit(0);
}


/* sync everything to disk */
void            sync_all(void)
{
   char            c, *oldstack;
   oldstack = stack;
   for (c = 'a'; c <= 'z'; c++)
      sync_to_file(c, 0);
   log("sync", "Full Sync Completed.");
}


/* removes an entry from the saved player lists */

int             remove_player_file(char *name)
{
   saved_player   *previous = 0, **hash, *list;
   char           *c;
   int             sum = 0;
   recmail	  *rescan;

   if (!isalpha(*name))
   {
      log("error", "Tried to remove non-player from save files.");
      return 0;
   }
   lower_case(name);

   hash = saved_hash[((int) (*name) - (int) 'a')];
   for (c = name; *c; c++)
   {
      if (isalpha(*c))
         sum += (int) (*c) - 'a';
      else
      {
         log("error", "Remove bad name from save files");
         return 0;
      }
   }

   hash += (sum % HASH_SIZE);
   list = *hash;
   for (; list; previous = list, list = list->next)
   {
      if (!strcmp(name, list->lower_name))
      {
         if (previous)
            previous->next = list->next;
         else
            *hash = list->next;
         if (list->data.where)
            FREE(list->data.where);
            
         /* new mail system part */
         while(list->received_list) {
           rescan = list->received_list;
           list->received_list = rescan->next;
           FREE(rescan);
         }
         
         free_room_data(list);
         set_update(*name);
         FREE((void *) list);
         
         return 1;
      }
   }
   return 0;
}


/* remove an entire hash of players */

void            remove_entire_list(char c)
{
   saved_player  **hash, *sp, *next;
   int             i;
   if (!isalpha(c))
      return;
   hash = saved_hash[((int) (c) - (int) 'a')];
   for (i = 0; i < HASH_SIZE; i++, hash++)
   {
      sp = *hash;
      while (sp)
      {
        if(!(sp->residency==STANDARD_ROOMS))
        {
          if(sp==*hash)
            *hash=sp->next;
          next = sp->next;
          if (sp->data.where)
            FREE(sp->data.where);
          free_room_data(sp);
          FREE((void *) sp);
          sp = next;
        }
        else
          sp = sp->next;
      }
   }
   set_update(c);
}


/* the routine that sets everything up for the save */

void            save_player(player * p)
{
   saved_player   *old, **hash, *sp;
   int             sum;
   file            data;
   char           *c, *oldstack;
   int verb = 1;

   oldstack = stack;

   if (!(p->location) || !(p->name[0])
       || p->residency == NON_RESIDENT || p->residency==STANDARD_ROOMS)
      return;

   if (sys_flags & PANIC)
   {
      c = stack;
      sprintf(c, "Attempting to save player %s.", p->name);
      stack = end_string(c);
      log("boot", c);
      stack = c;
   }
   if (!(isalpha(p->lower_name[0])))
   {
      log("error", "Tried to save non-player.");
      stack = oldstack;
      return;
   }

   if (p!=current_player)
      verb = 0;
   if (sys_flags & INVIS_SAVE)
      verb = 0;
      
   if (verb)
   {
      if (p->password[0]==0)
      {
         if(!(p->flags & CHUCKOUT))
           tell_player(p, " Tried to save character but failed ...\n"
           " Your character will not save until you set a password.\n"
           " Simply type 'password' whilst in command mode to set one.\n");
         p->residency |= NO_SYNC;
         if(p->flags & CHUCKOUT)
           tell_player(p, "\n\n Character Abandoned.\n");
         else
           tell_player(p, " NOT saved.\n");
         stack = oldstack;
         return;
      }
      else if (p->email[0] == 0)
      { 
         if(!(p->flags & CHUCKOUT))
           tell_player(p, " Tried to save character but failed ...\n"
           " Your character will not save until you set an email address.\n"
           " To set this just type 'email <whatever>', where <whatever> is your\n"
           " email address.\n"
           " If you do not have an email, please speak to one of the superusers.\n");
         p->residency |= NO_SYNC;
         if(p->flags & CHUCKOUT)
           tell_player(p, "\n\n Character Abandoned.\n");
         else
           tell_player(p, " NOT saved.\n");
         stack = oldstack;
         return;
      }
   }
   p->residency &= ~NO_SYNC;
   p->saved_residency = p->residency;
   old = p->saved;
   sp = old;
   if (!old)
   {
      sp = (saved_player *) MALLOC(sizeof(saved_player));
      memset((char *) sp, 0, sizeof(saved_player));
      strncpy(sp->lower_name, p->lower_name, MAX_NAME);
      strncpy(sp->saved_email, p->email, MAX_EMAIL-1);
      sp->rooms = 0;
      sp->mail_sent = 0;
      sp->received_list = 0;
      sp->list_top = 0;
      hash = saved_hash[((int) p->lower_name[0] - (int) 'a')];
      for (sum = 0, c = p->lower_name; *c; c++)
      {
         if (isalpha(*c))
            sum += (int) (*c) - 'a';
         else
         {
            tell_player(p, " Eeek, trying to save bad player name !!\n");
            FREE(sp);
            stack = oldstack;
            return;
         }
      }
      hash = (hash + (sum % HASH_SIZE));
      sp->next = *hash;
      *hash = sp;
      p->saved = sp;
      sp->saved_flags = p->saved_flags;
      create_room(p);
   }
   data = construct_save_data(p);
   if (!data.length)
   {
      log("error", "Bad construct save.");
      stack = oldstack;
      return;
   }
   if (old && sp->data.where)
      FREE((void *) sp->data.where);
   sp->data.where = (char *) MALLOC(data.length);
   sp->data.length = data.length;
   memcpy(sp->data.where, data.where, data.length);
   sp->residency = p->saved_residency;
   sp->saved_flags = p->saved_flags;
   strncpy(sp->last_host, p->inet_addr, MAX_INET_ADDR);
   strncpy(sp->saved_email, p->email, MAX_EMAIL-1);
   set_update(*(sp->lower_name));
   p->saved = sp;
   sp->last_on = time(0);
   if (verb)
      tell_player(p, " Character Saved ...\n");
   stack = oldstack;
}


void            create_banish_file(char *name)
{
   saved_player  **hash, *sp, *scan;
   int             sum;
   char           *c;

   sp = (saved_player *) MALLOC(sizeof(saved_player));
   memset((char *) sp, 0, sizeof(saved_player));
   strcpy(stack, name);
   lower_case(stack);
   strncpy(sp->lower_name, stack, MAX_NAME - 2);
   sp->saved_email[0]=' ';
   sp->saved_email[1]=0;
   sp->rooms = 0;
   sp->mail_sent = 0;
   sp->received_list = 0;
   hash = saved_hash[((int) name[0] - (int) 'a')];
   for (sum = 0, c = name; *c; c++)
      if (isalpha(*c))
    sum += (int) (*c) - 'a';
      else
      {
    log("error", "Tried to banish bad player");
    FREE(sp);
    return;
      }
   hash = (hash + (sum % HASH_SIZE));
   scan = *hash;
   while (scan)
   {
      hash = &(scan->next);
      scan = scan->next;
   }
   *hash = sp;
   sp->residency = BANISHD;
   sp->saved_flags = 0;
   sp->last_host[0] = 0;
   sp->last_on = time(0);
   sp->next = 0;
   set_update(tolower(*(sp->lower_name)));
}


/* load from a saved player into a current player */

/* actually do load */

int             load_player(player * p)
{
   saved_player   *sp;

   lower_case(p->lower_name);
   sp = find_saved_player(p->lower_name);
   p->saved = sp;
   if (!sp)
      return 0;

   p->residency = sp->residency;

   p->saved_residency = p->residency;
   p->saved_flags = sp->saved_flags;
   if (sp->residency == STANDARD_ROOMS
        || sp->residency == BANISHD)
      return 1;

   load_player_common(p, sp->data);

   decompress_list(sp);
   p->saved_flags = sp->saved_flags;
   return 1;
}

