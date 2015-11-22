/*
 * dynamic_plists.c - included into plists.c if required during compile
 *
 * OR "Doctor Hideous The Nasty Monkey.c"
 */

#define DYN_VERS "1.2"

/* write one player file out */

void            write_to_file(saved_player * sp)
{
   char           *oldstack;
   int             length, fd;
   oldstack = stack;
   
   if(!sp) return;
   
   if (sp->residency == STANDARD_ROOMS || sp->residency & NO_SYNC)
     return;
       
   if (sys_flags & VERBOSE && sys_flags & PANIC)
      vlog("sync", "Attempting to write player '%s'.", sp->lower_name);
   
   sprintf(stack, "files/dynamic/%.1s/%.1s/%s.saved", 
   	   sp->lower_name, (sp->lower_name)+1, sp->lower_name);
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
      vlog("dynamic", "Eeeek!  Couldn't save saved data file for '%s'", sp->lower_name);
      return;
   }
   
   /* in main plists.c */
   length = write_to_file_common(sp);
   
   /* DYNAMIC */
   write(fd, oldstack, length);
   close(fd);
   stack = oldstack;
}
 
 
/* support function - return int for cache item given saved_player */
int	find_cached_player(char *name)
{
   int count;
   
   /* scan the cache */
   for(count=0; count<MAX_DYNAMIC_CACHE; count++)
     if(dynamic_cache[count].sp && !strcasecmp(name, dynamic_cache[count].sp->lower_name))
       return count;
       
   return -1;
}


/* clear a cache item */
void	empty_cache_item(int item)
{
  /* if someone is in the cache, then kill their pointer to it */
  
  if(dynamic_cache[item].sp) 
    dynamic_cache[item].sp->cache = -1;
   
  if(dynamic_cache[item].data.where)
    FREE(dynamic_cache[item].data.where);
  dynamic_cache[item].data.length = 0;
  dynamic_cache[item].sp = 0;
  dynamic_cache[item].last_access = 0;
}


/* wipe the cache.. */
void	wipe_dynamic_cache(void)
{
   int  count;
   for(count = 0; count<MAX_DYNAMIC_CACHE; count++)
     empty_cache_item(count);
}


/* pity the admin who misuses this */
void	nuke_cache(player *p, char *str)
{
  saved_player *scan, **hash;
  char let;
  int i;
  
  tell_player(p, " Done, I hope you really meant to do that or used sync_cache first ;).\n");
  wipe_dynamic_cache();
  
  /* wipe -all- cache pointers */
  for(let='a'; let<='z'; let++) {
    hash = saved_hash[((int) (tolower(let)) - (int) 'a')];
    for(i=0; i<HASH_SIZE; i++, hash++) {
      for(scan=*hash; scan; scan = scan->next)
        scan->cache = -1;
    }
  }
}


/* thing to see the cache contents */
void	view_dynamic_cache(player *p, char *str)
{
  int count;
  char *oldstack;
  
  oldstack = stack;
  
  sprintf(stack, " Current dynamic_cache contents (size %d):\n", MAX_DYNAMIC_CACHE);
  stack = strchr(stack, 0);
  /* scan through the cache */
  for(count=0; count<MAX_DYNAMIC_CACHE; count++) {
    if(dynamic_cache[count].sp) {
      sprintf(stack, "  Pos %-3d: %-20s (%d bytes)\n", count, 
      	dynamic_cache[count].sp->lower_name, dynamic_cache[count].data.length);
      stack = strchr(stack, 0);
    }
  }
  *stack++=0;
  pager(p, oldstack, 0);
  stack = oldstack;
}


/* return a saved_player who think they are at a cache item */
saved_player 	*locate_by_cache(int find)
{
  saved_player *scan, **hash;
  char let;
  int i;
  
  if(find==-1) 
    return 0;
  
  for(let='a'; let<='z'; let++) {
    hash=saved_hash[((int) (tolower(let)) - (int) 'a')];
    for(i=0; i<HASH_SIZE; i++, hash++)
      for(scan=*hash; scan; scan=scan->next)
        if(scan->cache==find)
          return scan;
  }
  return 0;
}


/* support routine for the below, syncs an item to disk */
int	sync_cache_item_to_disk(int item)
{
  int fd;
  char *oldstack;
  saved_player *scan;
  
  if(!dynamic_cache[item].sp) {
    oldstack = stack;
    sprintf(stack, "---------------------------------\n"
    		   "-=> Problem in sync_cache_item with cache item %d\n", item);
    stack = strchr(stack, 0);
    /* find a player pointing to it */
    scan = locate_by_cache(item);
    if(scan)
      sprintf(stack, "-=> Saved player '%s' points to item", scan->lower_name);
    else
      strcpy(stack, "-=> No saved player pointing to item.\n");
    stack = end_string(stack);
    log("cache", oldstack);
    stack = oldstack;
    if(scan) {
      scan->residency |= NO_SYNC;
      scan->cache = -1;
    }
    empty_cache_item(item);
    return 0;
  }
  
  sprintf(stack, "files/dynamic/%.1s/%.1s/%s.data",
  		dynamic_cache[item].sp->lower_name,
  		(dynamic_cache[item].sp->lower_name)+1,
  		dynamic_cache[item].sp->lower_name);
#if defined( FREEBSD )
  fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
  if(fd<0) {
    vlog("cache", "Failed to open dynamic data file for '%s'", dynamic_cache[item].sp->lower_name);
    return 0;
  }
  
  write(fd, dynamic_cache[item].data.where, dynamic_cache[item].data.length);
  close(fd);
  return 1;
} 


/* sync partial cache */
void	sync_cache_letter(char l)
{
  int count;
  
  for(count=0; count<MAX_DYNAMIC_CACHE; count++) {
    if(dynamic_cache[count].sp && *(dynamic_cache[count].sp->lower_name)==l) /* if theres a player there */
      sync_cache_item_to_disk(count);
  }
}


/* sync whole cache to disk */
void	sync_whole_cache(void)
{
  int count;
  
  for(count=0; count<MAX_DYNAMIC_CACHE; count++) {
    if(dynamic_cache[count].sp)
      sync_cache_item_to_disk(count);
  }
}


/* find space in the cache, if we cant find one, sync oldest, and empty it
   returns an available cache item or -1 if failed */
int	find_available_cache_space(void)
{
  int count, oldest_last_access, oldest;
  
  /* step through, but keep track of oldest one */
  oldest_last_access = dynamic_cache[0].last_access;
  oldest = 0;
  
  for(count=0; count<MAX_DYNAMIC_CACHE; count++) {
    if(!dynamic_cache[count].sp)
      return count;
    else if(oldest_last_access > dynamic_cache[count].last_access) {
      oldest_last_access = dynamic_cache[count].last_access;
      oldest = count;
    }
  }
  
  /* if we get here, then there were no free spaces!
     this means we should sync the oldest, and free it */
  if(sync_cache_item_to_disk(oldest)) {
    empty_cache_item(oldest);
    return oldest;
  }
  else {
    /* in here, we need to log the fact and then return a negative number */
    log("cache", "Meep!  Couldn't allocate any cache space!");
    return -1;
  }
}


/* debugging function - scan who is logged in, and output who is in the cache
   (alledgedly) and where */
void	scan_players_for_caching(player *p, char *str)
{
  char *oldstack, let;
  int i;
  saved_player *scan, **hash;
  
  oldstack = stack;
  strcpy(stack, " Scanning for people in the cache from players:\n");
  stack = strchr(stack, 0);
  for(let='a'; let<='z'; let++) {
    hash = saved_hash[((int) (tolower(let)) - (int) 'a')];
    for(i=0; i<HASH_SIZE; i++, hash++) {
      for(scan=*hash; scan; scan=scan->next)
        if(scan->cache>=0) {
          sprintf(stack, " %-20s: %-3d\n", scan->lower_name, scan->cache);
          stack = strchr(stack, 0);
        }
    }
  }
  stack = end_string(stack);
  pager(p, oldstack, 0);
  stack = oldstack;
}


/* cache_data TEST - write the data into the cache from dynamic_tempfile
   this is used on loading routines */
int	dynamic_tempfile_to_cache(saved_player *sp, file readfrom)
{
  int found=-1;
  
  /* first check if they are already in the cache! */
  if(sp->cache<0) {
    sp->cache=find_available_cache_space();
    if(sp->cache<0) {
      vlog("cache", "dynamic_tempfile_to_cache() was unable to cache %s!", sp->lower_name);
      return 0;
    }
  }
  else {
    found = find_cached_player(sp->lower_name);
    if(found>=0 && sp->cache!=found) {
      log("cache", "tempfile to cache error 2");
      return 0;
    }
    /* at this point, a cache item for the user has been found, this was
       forgotten, but thanks to john kolesar for spotting that the original
       where data wasn't being freed... */
    /*if(dynamic_cache[sp->cache].data.where) {
      FREE(dynamic_cache[sp->cache].data.where);
      dynamic_cache[sp->cache].data.length=0;
    }*/
    /* above fix DISABLED - was causing havoc on pfile writes.
       oddly, I can't find any evidence of a leak either even though it
       appears that there should be one. */
  }
      
  /* cache them according to found, methinks */
  dynamic_cache[sp->cache].last_access = time(0);
  dynamic_cache[sp->cache].data.length = readfrom.length;
  dynamic_cache[sp->cache].data.where = (char *) MALLOC(dynamic_cache[sp->cache].data.length+1);
  memset(dynamic_cache[sp->cache].data.where, 0, dynamic_cache[sp->cache].data.length+1);
  memcpy(dynamic_cache[sp->cache].data.where, readfrom.where, dynamic_cache[sp->cache].data.length);
  dynamic_cache[sp->cache].sp = sp;
  return 1;
}
  
            

/* user function for the above */
void	sync_all_cache(player *p, char *str)
{
  sync_whole_cache();
  tell_player(p, " Dynamic Players cache synced to disk.\n");
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
      strncpy(sp->lower_name, oldstack, MAX_NAME-1);
      sp->saved_email[0]=0;
      strncpy(player_loading, sp->lower_name, MAX_NAME-1);
      sp->rooms = 0;
      sp->mail_sent = 0;
      sp->received_list = 0;
      sp->list_top = 0;
      sp->cache = -1;
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
   else if(old->cache>=0) {
     empty_cache_item(old->cache);
     old->cache = -1;
   }
   
   where = get_int(&sp->last_on, where);
   where = get_int(&sp->saved_flags, where);
   where = get_int(&sp->residency, where);

   if (sp->residency == BANISHD)
   {
      sp->last_host[0] = 0;
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
   where = retrieve_room_data(sp, where);
   where = retrieve_list_data(sp, where);
   where = retrieve_mail_data(sp, where);
   stack = oldstack;
}


/* hard load in one player file (the .save bits) */
void		hard_load_one_file(char c)
{
  char		*oldstack, *where;
  int		fd, length, di;
  DIR		*dr;
  struct dirent *fname;

  oldstack = stack;
  if(sys_flags & VERBOSE) 
    vlog("boot", "Loading player file '%c'.", c);
  
  /* this bit is for when we have some sort of hashing */
  for(di=0; di<26; di++) {
    sprintf(stack, "files/dynamic/%c/%c", c, 'a'+di);
    dr = opendir(stack);
    if(!dr) {
      vlog("dynamic", "FUCK!  files/dynamic/%c/%c won't open!", c, 'a'+di);
      return;
    }
    
    for(fname=readdir(dr); fname!=NULL; fname=readdir(dr)) {
      if(strstr(fname->d_name, ".saved")) { /* a saved player */    
        sprintf(stack, "files/dynamic/%c/%c/%s", c, 'a'+di, fname->d_name);
        fd = open(stack, O_RDONLY | O_NDELAY);
        if(fd<0) 
          vlog("dynamic", "Failed to load saved player file '%s'", fname->d_name);
        else {
          length = lseek(fd, 0, SEEK_END);
          lseek(fd, 0, SEEK_SET);
          if(length) {
            where = (char *) MALLOC(length+1);
            memset(where, 0, length+1);
            if(read(fd, where, length)<0)
              handle_error("Can't read player file.");
            extract_player(where, length);
            FREE(where);
          }
          else {
            vlog("dynamic", "Bad player '%s' deleted on load.", player_loading);
            remove_player_file(player_loading);
            signal(SIGSEGV, error_on_load);
            signal(SIGBUS, error_on_load);
          }
          close(fd);
        }
      }
    }
    closedir(dr);
  }          
  stack = oldstack;
}


/* sync player files corresponding to one letter */
void		sync_to_file(char c, int background)
{
  saved_player *scan, **hash;
  char		*oldstack;
  int		i;
  player	*online;
  
  if(background && fork())
    return;
    
  oldstack = stack;
  if(sys_flags & VERBOSE) {
    sprintf(oldstack, "Syncing File '%c'.", c);
    stack = end_string(oldstack);
    log("sync", oldstack);
    stack = oldstack;
  }
  
  sys_flags |= INVIS_SAVE;
  
  hash = saved_hash[((int) c - (int) 'a')];
  for(i=0; i<HASH_SIZE; i++, hash++)
    for(scan=*hash; scan; scan=scan->next) {
      /* are they online? */
      online = find_player_absolute_quiet(scan->lower_name);
      if(online)
        save_player(online);
        
      if(scan->cache>=0)
        sync_cache_item_to_disk(scan->cache);
      write_to_file(scan);
    }
    
  sys_flags &= ~INVIS_SAVE;
  update[(int) c - (int) 'a'] = 0;
  stack = oldstack;
  
  if(background)
    exit(0);
}


/* sync everything to disk */
void		sync_all()
{
  char	c, *oldstack;
  oldstack = stack;
  
  /* sync the cache first */
  sync_whole_cache();
  
  for(c='a'; c<='z'; c++)
    sync_to_file(c, 0);
  log("sync", "Full Sync Completed.");
}


/* removes an entry from the saved player lists */
int             remove_player_file(char *name)
{
   saved_player   *previous = 0, **hash, *list;
   char           *c;
   int             sum = 0, cachekill;
   recmail	  *rescan;

   if (!isalpha(*name))
   {
      log("error", "Tried to remove non-player from save files.");
      return 0;
   }
   /* all things coming in here should already be lower case. this is if
      someone has spooned. */
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
	 /* first, save their files to disk. */
         write_to_file(list);
         /* take them out of the cache! */
         cachekill = find_cached_player(list->lower_name);
         if(cachekill!=-1) {
            sync_cache_item_to_disk(cachekill);
            empty_cache_item(cachekill);
         }
         list->cache = -1;   
         /* uncache them to disk */
         /* DYNAMIC - well this bit needs to rm the data file presumably */
         sprintf(stack, "mv -f files/dynamic/%.1s/%.1s/%s.data "
         		"files/dynamic/deadzone/%s.data",
         	 list->lower_name, (list->lower_name)+1, list->lower_name, list->lower_name);
         system(stack);
         sprintf(stack, "mv -f files/dynamic/%.1s/%.1s/%s.saved "
         		"files/dynamic/deadzone/%s.saved",
         	 list->lower_name, (list->lower_name)+1, list->lower_name, list->lower_name);
         system(stack);
         if (previous)
            previous->next = list->next;
         else
            *hash = list->next;
         
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
      while(sp)
      {
         if(!(sp->residency==STANDARD_ROOMS))
         {
            if(sp==*hash)
               *hash=sp->next;
            next = sp->next;
            free_room_data(sp);
            FREE((void*) sp);
            sp = next;
         }
         else
            sp = sp->next;
      }
   }
}


/* the routine that sets everything up for the save */
/* caching attempt now added.. */
void            save_player(player * p)
{
   saved_player   *old, **hash, *sp;
   int             sum, fd=-1;
   file            data;
   char           *c, *oldstack;
   int verb = 1, cached=-1;

   if(p->residency==STANDARD_ROOMS)
     return;
     
   oldstack = stack;

   if (!(p->location) || !(p->name[0])
       || p->residency == NON_RESIDENT)
      return;

   if (sys_flags & PANIC)
      vlog("boot", "Attempting to save player %s.", p->name);

   if (!(isalpha(p->lower_name[0]))) {
      log("error", "Tried to save non-player.");
      return;
   }

   if (p != current_player)
      verb = 0;
   if (sys_flags & INVIS_SAVE)
      verb = 0;
   
   if (verb) {
      if (p->password[0]==0) {
         if(!(p->flags & CHUCKOUT))
           tell_current(" Tried to save this character but failed ...\n"
           " Your character will not save until you set a password.\n"
           " Simply type 'password' whilst in command mode to set one.\n");
         p->residency |= NO_SYNC;
         if(p->flags & CHUCKOUT)
           tell_current("\n\n Character Abandoned.\n");
         else
           tell_current(" NOT saved.\n");
         return;
      }
      else if (p->email[0]==0) {
         if(!(p->flags & CHUCKOUT))
           tell_current(" Tried to save this character but failed ...\n"
           " Your character will not save until you set an email address.\n"
           " To set this just type 'email <whatever>', where <whatever> is your\n"
           " email address.\n"
           " If you do not have an email, please speak to one of the superusers.\n");
         p->residency |= NO_SYNC;
         if(p->flags & CHUCKOUT)
           tell_current("\n\n Character Abandoned.\n");
         else
           tell_player(p, " NOT saved.\n");
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
      strncpy(sp->lower_name, p->lower_name, MAX_NAME-1);
      strncpy(sp->saved_email, p->email, MAX_EMAIL-1);
      sp->rooms = 0;
      sp->mail_sent = 0;
      sp->received_list = 0;
      sp->list_top = 0;
      sp->cache = -1;
      hash = saved_hash[((int) p->lower_name[0] - (int) 'a')];
      for (sum = 0, c = p->lower_name; *c; c++)
      {
         if (isalpha(*c))
            sum += (int) (*c) - 'a';
         else {
            tell_player(p, " Eeek, trying to save bad player name !!\n");
            FREE(sp);
            return;
         }
      }
      hash = (hash + (sum % HASH_SIZE));
      sp->next = *hash;
      *hash = sp;
      p->saved = sp;
      sp->saved_flags = p->saved_flags;
      create_room(p);
      set_update(*(p->lower_name));
   }
   
   /* DYNAMIC */
   /* ok, check to see if they are, cache them in their old place.  if not, then
      stick them into the cache, or dump to disk if that fails */
   
   /* we dont want to cache them if they arent to be cached */
   if(p->flags & ARGH_DONT_CACHE_ME) {
      cached = -1;
      sp->cache = -1;
   }
   else {
      cached = find_cached_player(sp->lower_name);
      if(cached!=-1) 
         sp->cache=cached;
      else {
         cached = find_available_cache_space();
         sp->cache = cached;
      }
   }
   
   /* if they arent in the cache and there is no space, sync it. */   
   if(cached<0) {
      sprintf(stack, "files/dynamic/%.1s/%.1s/%s.data", 
   	   p->lower_name, (p->lower_name)+1, p->lower_name);
#if defined( FREEBSD )
      fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
      fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
      if(fd<0) {
         vlog("dynamic", "Failed to open dynamic data file for '%s'", p->lower_name);
         return;
      }
   }
   /* after here, stack is affected */
   data = construct_save_data(p);
   if (!data.length) {
      log("error", "Bad construct save.");
      stack = oldstack;
      if(fd>-1)
        close(fd);
      return;
   }
      
   /* DYNAMIC */
   /* cache checking! */
   if(sp->cache<0) {
      if(fd>-1) {
        write(fd, data.where, data.length);
        close(fd);
      }
      else
        log("dynamic", "save_player tried to write to non existant fd due to bad cache value");
   }
   else {
      if(!dynamic_tempfile_to_cache(sp, data)) {
         vlog("cache", "Caching problem encountered with %s", sp->lower_name);
         vsu_wall("-=> Meep! %s failed to cache on save!\n", sp->lower_name);
      }
   }

   sp->residency = p->saved_residency;
   sp->saved_flags = p->saved_flags;
   strncpy(sp->last_host, p->inet_addr, MAX_INET_ADDR - 1);
   strncpy(sp->saved_email, p->email, MAX_EMAIL-1);
   p->saved = sp;
   sp->last_on = time(0);
   /* DYNAMIC - we do need to save their file however.. */
   /* this bit is commented out since i think we only need to save it if we are
      delinking it, or if we are shutting down/crashing */
   if (verb)
      tell_current(" Character Saved ...\n");
   stack = oldstack;
}


/* the routine that sets everything up for the save */

void            create_banish_file(char *str)
{
   saved_player  **hash, *sp, *scan;
   int             sum;
   char           *c, name[20];

   strncpy(name, str, MAX_NAME - 1);
   sp = (saved_player *) MALLOC(sizeof(saved_player));
   memset((char *) sp, 0, sizeof(saved_player));
   strcpy(stack, name);
   lower_case(stack);
   strncpy(sp->lower_name, stack, MAX_NAME - 1);
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
   sp->cache = -1;
   write_to_file(sp);
}


/* this bit is the same as ever, but we are going to cache the person loaded
   in, either in a new place, or in their old place in the cache. fear wildly.
 */
int             load_player(player * p)
{
   saved_player   *sp;
   char           *oldstack;
   int		   fd, cached;

   oldstack = stack;
   
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

   /* ok, let's check the cache for the person to load, if it is found, load
      them from the cache, if it is not found, then panic slightly and put it
      in after laoding from disky-wisky */
   cached = find_cached_player(p->lower_name);   
   if(cached<0) 
      sp->cache = -1;
   else
      sp->cache = cached;
   
   if(cached<0) /* grab from disk and cache them */
   {   
      /* dynamic files - load the player file in! */
      sprintf(stack, "files/dynamic/%.1s/%.1s/%s.data", 
   	   p->lower_name, (p->lower_name)+1, p->lower_name);
      fd = open(stack, O_RDONLY);
      if(fd<0)
      {
         vlog("dynamic", "Failed to find player data file '%s'", sp->lower_name);
         return 0;
      }
      dynamic_tempfile.length = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
      dynamic_tempfile.where = (char *) MALLOC(dynamic_tempfile.length + 1);
      memset(dynamic_tempfile.where, 0, dynamic_tempfile.length + 1);
      if(read(fd, dynamic_tempfile.where, dynamic_tempfile.length) < 0)
      {
         close(fd);
         if(dynamic_tempfile.where)
            FREE(dynamic_tempfile.where);
         dynamic_tempfile.length = 0;
         vlog("dynamic", "Failed to load player data file '%s'", sp->lower_name);
         return 0;
      }
      close(fd);
      /* cache the sucker now it's loaded, it's in dynamic_tempfile, so.. */
      if(!(p->flags & ARGH_DONT_CACHE_ME))
         dynamic_tempfile_to_cache(sp, dynamic_tempfile);
   }
   else /* grab from their cache position */
   {
      /* seeing as the rest of this function likes to use dynamic_tempfile,
         we will simply copy the data from a->b, rather than setting a pointer
         which would be far quicker. */
      /* we don't need a +1 bit on the length as we will use the literal data*/
      dynamic_tempfile.length = dynamic_cache[cached].data.length;
      dynamic_tempfile.where = (char *) MALLOC(dynamic_tempfile.length+1);
      memset(dynamic_tempfile.where, 0, dynamic_tempfile.length+1);
      memcpy(dynamic_tempfile.where, dynamic_cache[cached].data.where, dynamic_tempfile.length);
      dynamic_cache[cached].last_access=time(0);
   }
   
   load_player_common(p, dynamic_tempfile);
   
   decompress_list(sp);
   
   p->saved_flags = sp->saved_flags;
   
   if(dynamic_tempfile.where)
      FREE(dynamic_tempfile.where);
   dynamic_tempfile.length = 0;
   stack = oldstack;
   return 1;
}



/* make sure all the dynamic directories are there,
   a routine that should be run at bootup */
/* cos this is a beginner routine thingy, and we will look at the
   world through rose tinted glasses, let's just assume if it can't
   open them its due to dir not existing */
void	create_dynamic_directories(void)
{
   int di, di2;
   DIR *dr;
   
   log("boot", "Checking Dynamic Player Directories...");
   /* create the deadzone directory */
   dr = opendir("files/dynamic/deadzone");
   if(!dr)
   {
      system("mkdir files/dynamic/deadzone");
      dr = opendir("files/dynamic/deadzone");
      if(!dr)      
      {
         log("boot", "Can't create 'files/dynamic/deadzone!'");
         exit(1);
      }
   }
   closedir(dr);
   log("boot", " Base dir 'deadzone', OK.");
   /* external loop for first character */
   for(di=0; di<26; di++)
   {
      /* ping the angel */
      do_alive_ping();

      /* first check that the initial directory exists. */
      sprintf(stack, "files/dynamic/%c", 'a'+di);
      dr = opendir(stack);
      if(!dr)
      {
         /* lazy mans way - let's just spammily use mkdir */
         sprintf(stack, "mkdir files/dynamic/%c", 'a'+di);
         system(stack);
         sprintf(stack, "files/dynamic/%c", 'a'+di);
         dr = opendir(stack);
         if(!dr)
         {
            printf("FUCKED!\n");
            sprintf(stack, "Meep! :-( I can't create files/dynamic/%c", 'a'+di);
            printf(stack);
            exit(1);
         }
      }
      closedir(dr);
      /* internal loop for second character */
      for(di2=0; di2<26; di2++)
      {
         sprintf(stack, "files/dynamic/%c/%c", 'a'+di, 'a'+di2);
         dr = opendir(stack);
         if(!dr)
         {
            /* lazy mans way - let's just spammily use mkdir */
            sprintf(stack, "mkdir files/dynamic/%c/%c", 'a'+di, 'a'+di2);
            system(stack);
            sprintf(stack, "files/dynamic/%c/%c", 'a'+di, 'a'+di2);
            dr = opendir(stack);
            if(!dr)
            {
               printf("FUCKED!\n");
               sprintf(stack, "Meep! :-( I can't create files/dynamic/%c/%c", 'a'+di, 'a'+di2);
               printf(stack);
               exit(1);
            }
         }
         closedir(dr);
      }
      vlog("boot", " Base dir '%c' (directories a-z), OK.", 'a'+di);
   }
}


/* find a player in the caches if they are in there
   init everything needed for the plist file */
void            init_plist(void)
{
   char           *oldstack;
   int             i;

   oldstack = stack;
   vlog("boot", "SensiSummink V%s -=DYNAMIC PLAYERS VERSION=- Booting...", VERSION);
   create_dynamic_directories();
   log("boot", "Initialising dynamic player files cache...");
   for(i=0; i<MAX_DYNAMIC_CACHE; i++)
     dynamic_cache[i].sp = 0;
   wipe_dynamic_cache();
   log("boot", "Scanning/Loading dynamic player files...");
   hard_load_files();
   for (i = 0; i < 26; i++)
      update[i] = 0;

   stack = oldstack;
}


void dynamic_version(void)
{
  sprintf(stack, " -=> SensiSummink cached dynamic playerfiles V%s enabled.\n", DYN_VERS);
  stack = strchr(stack, 0);
}
