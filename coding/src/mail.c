/*
 * mail.c
 */

#include "include/config.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef SUNOS
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"
#include "include/proto.h"

/* interns */
note           *n_hash[NOTE_HASH_SIZE];
int             nhash_update[NOTE_HASH_SIZE];
int             unique = 1;
int             news_start = 0, news_count = 0;
int		snews_start = 0, snews_count = 0;
void            unlink_mail(note *);
char		*tidy_article_title(char *);


recmail *destroy_recmail(saved_player *sp, recmail *killme)
{
  recmail *scan;
  if(!sp)
    return 0;
  if(!sp->received_list)
    return 0;
  if(killme==sp->received_list) /* first one */
  {
    sp->received_list = sp->received_list->next;
    FREE(killme);
    return sp->received_list; /* return next one in list */
  }
  for(scan=sp->received_list; scan; scan=scan->next)
    if(scan->next==killme) {
      scan->next=killme->next;
      FREE(killme);
      return scan->next; /* return next one */
    }
  return 0;
}

/* find note in hash */

note           *find_note(int id)
{
   note           *scan;
   if (!id)
      return 0;
   for (scan = n_hash[id % NOTE_HASH_SIZE]; scan; scan = scan->hash_next)
      if (scan->id == id)
         return scan;
   return 0;
}

void inc_unique(void)
{
  unique++;
  if(unique>MYMAXINT)
    unique=1;
}

/* locate the next free note ID. */
int find_unique_note_id(void)
{
  int cur = unique;
  /* theres probably a nicer way of doing the next few lines before the loop */
  if(!find_note(unique))
    return unique;
  /* found the current 'unique' one... */
  unique++;
  while(unique!=cur) {
    if(!find_note(unique)) {
      vlog("unique", "unique: %d", unique);
      return unique;
    }
    inc_unique();
  }
  return 0;
}


/* create a new note */

note           *create_note(void)
{
   note           *n;
   int             num;
   n = (note *) MALLOC(sizeof(note));
   memset(n, 0, sizeof(note));
   n->flags = NOT_READY;
   n->date = time(0);
   n->next_item = 0;
   n->text.where = 0;
   n->text.length = 0;
   strcpy(n->header, "DEFUNCT");
   strcpy(n->name, "system");
   
   /* find the next unused note id */
   n->id = find_unique_note_id();
   if(n->id==0) {
     free(n);
     return 0;
   }
   /* may avoid the loop next time, we can assume this one to be used and move on */
   inc_unique();
   
   num = (n->id) % NOTE_HASH_SIZE;
   n->hash_next = n_hash[num];
   n_hash[num] = n;
   nhash_update[num] = 1;
   return n;
}

/* remove a note */

void            remove_note(note * n)
{
   note           *scan, *prev;
   saved_player **hash, *pscan;
   recmail	   *rscan;
   int             num, i;
   char		   let;
   if (n->text.where)
      FREE(n->text.where);
   num = (n->id) % NOTE_HASH_SIZE;
   scan = n_hash[num];
   if (scan == n)
      n_hash[num] = n->hash_next;
   else
   {
      do
      {
    prev = scan;
    scan = scan->hash_next;
      } while (scan != n);
      prev->hash_next = n->hash_next;
   }
   /* now we should really scan all users to see if thats a note that should
      be erased */
   /* if its mail */
   if(!(n->flags & NEWS_ARTICLE || n->flags & SNEWS_ARTICLE))
   {
     /* players from a-z */
     for(let='a'; let<='z'; let++) {
       hash=saved_hash[((int) (tolower(let)) - (int) 'a')];
       /* player in hash */
       for(i=0; i<HASH_SIZE; i++, hash++)
         for(pscan=*hash; pscan; pscan=pscan->next) 
           if(pscan->residency!=STANDARD_ROOMS && pscan->residency!=BANISHD && pscan->received_list) 
             for(rscan=pscan->received_list; rscan; rscan=rscan->next)
               if(rscan->mail_received==n->id) {
                 destroy_recmail(pscan, rscan);
                 break;
               }
     }
   }
   FREE(n);
   nhash_update[num] = 1;
}

/* remove various types of note */

void            remove_any_note(note * n)
{
   saved_player   *sp;
   note           *scan = 0;
   char           *oldstack;
   int            *change=0;
   oldstack = stack;
   if (n->flags & NEWS_ARTICLE)
   {
      scan = find_note(news_start);
      change = &news_start;
      while (scan && scan != n)
      {
         change = &(scan->next_item);
         scan = find_note(scan->next_item);
      }
      if (scan == n) news_count--;
   }
   else if (n->flags & SNEWS_ARTICLE)
   {
      scan = find_note(snews_start);
      change = &snews_start;
      while (scan && scan != n)
      {
         change = &(scan->next_item);
         scan = find_note(scan->next_item);
      }
      if(scan==n) snews_count--;
   } 
   else
   {
      strcpy(stack, n->name);
      lower_case(stack);
      stack = end_string(stack);
      sp = find_saved_player(oldstack);
      if (!sp)
      {
         log("mail_error", "Bad owner name in mail(1)");
         log("mail_error", oldstack);
         unlink_mail(find_note(n->next_item));
         scan = 0;
      } 
      else
      {
         change = &(sp->mail_sent);
         scan = find_note(sp->mail_sent);
         while (scan && scan != n)
         {
            change = &(scan->next_item);
            scan = find_note(scan->next_item);
         }
      }
   }
   if (scan == n)
      (*change) = n->next_item;
   remove_note(n);
   stack = oldstack;
}



/* dump one note onto the stack */

int             save_note(note * d)
{
   if (d->flags & NOT_READY)
      return 0;

   stack = store_int(stack, d->id);
   stack = store_int(stack, d->flags);
   stack = store_int(stack, d->date);
   stack = store_int(stack, d->read_count);
   stack = store_int(stack, d->next_item);
   stack = store_string(stack, d->header);
   stack = store_int(stack, d->text.length);
   memcpy(stack, d->text.where, d->text.length);
   stack += d->text.length;
   stack = store_string(stack, d->name);
   return 1;
}


/* sync one hash bucket to disk */

void            sync_note_hash(int number)
{
   char           *oldstack, name[MAX_NAME + 2];
   note           *scan, *check;
   int             length, count = 0, fd, t;
   saved_player   *sp;

   oldstack = stack;

   if (sys_flags & VERBOSE)
      vlog("sync", "Syncing note hash %d.", number);

   t = time(0);

   for (scan = n_hash[number]; scan; scan = check)
   {
      if (scan->flags & NEWS_ARTICLE)
      {
         strcpy(name, scan->name);
         lower_case(name);
         sp = find_saved_player(name);
         if ((t - (scan->date)) > NEWS_TIMEOUT)
         {
            if (!(sp && sp->residency & ADMIN))
            {
               check = scan->hash_next;
               remove_any_note(scan);
               /* scan=n_hash[number];  */
            }
         }
      }
      else if (scan->flags & SNEWS_ARTICLE)
      {
         strcpy(name, scan->name);
         lower_case(name);
         sp = find_saved_player(name);
         if ((t - (scan->date)) > NEWS_TIMEOUT)
         {
            if (!(sp && sp->residency & ADMIN))
            {
               check = scan->hash_next;
               remove_any_note(scan);
               /* scan=n_hash[number];  */
            }
         }
      } 
      else if ((t - (scan->date)) > MAIL_TIMEOUT)
      {
         check = scan->hash_next;
         remove_any_note(scan);
         /* scan=n_hash[number];  */
      }
      check = scan->hash_next;
   }

   stack = store_int(stack, 0);

   for (scan = n_hash[number]; scan; scan = scan->hash_next)
      count+=save_note(scan);

   store_int(oldstack, count);
#ifdef OSF
    length = (long) stack - (long) oldstack;
#else
    length = (int) stack - (int) oldstack;
#endif
   sprintf(stack, "files/notes/hash%d", number);
#if defined( FREEBSD )
   fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
   if (fd < 0)
      handle_error("Failed to open note file.");
   if (write(fd, oldstack, length) < 0)
      handle_error("Failed to write note file.");
   close(fd);

   nhash_update[number] = 0;
   stack = oldstack;
}

/* throw all the notes to disk */

void            sync_notes(int background)
{
   int             n, fd;
   char           *oldstack;
   oldstack = stack;

   if (background && fork())
      return;

   if (sys_flags & VERBOSE || sys_flags & PANIC)
      log("sync", "Dumping notes to disk");
   for (n = 0; n < NOTE_HASH_SIZE; n++)
      sync_note_hash(n);
   if (sys_flags & VERBOSE || sys_flags & PANIC)
      log("sync", "Note dump completed");
#if defined( FREEBSD )
   fd = open("files/notes/track", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open("files/notes/track", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
   if (fd < 0)
      handle_error("Failed to open track file.");
   stack = store_int(stack, unique);
   stack = store_int(stack, news_start);
   stack = store_int(stack, news_count);
   stack = store_int(stack, snews_start);
   stack = store_int(stack, snews_count);
   if (write(fd, oldstack, 20) < 0)
      handle_error("Failed to write track file.");
   close(fd);

   stack = oldstack;

   if (background)
      exit(0);
}

/* su command to dump all or single notes to disk */

void            dump_com(player * p, char *str)
{
   int             num;

   if (!isdigit(*str))
     {
       if (p!=NULL)
	 tell_player(p, " Dumping all notes to disk.\n");
       sync_notes(1);
       return;
     }

   num = atoi(str);

   if (num < 0 || num >= NOTE_HASH_SIZE)
   {
     if (p!=NULL)
       tell_player(p, " Argument not in hash range !\n");
     return;
   }
   if (p!=NULL)
      vtell_player(p, " Dumping hash %d to disk.\n", num);
   sync_note_hash(num);
}


/* throw away a hash of notes */

void            discard_hash(int num)
{
   note           *scan, *next;
   for (scan = n_hash[num]; scan; scan = next)
   {
      next = scan->hash_next;
      if (scan->text.where)
    FREE(scan->text.where);
      FREE(scan);
   }
}


/* extracts one note into the hash list */

char           *extract_note(char *where)
{
   int             num;
   note           *d;
   d = (note *) MALLOC(sizeof(note));
   memset(d, 0, sizeof(note));
   where = get_int(&d->id, where);
   where = get_int(&d->flags, where);
   where = get_int(&d->date, where);
   where = get_int(&d->read_count, where);
   where = get_int(&d->next_item, where);
   where = get_string(d->header, where);
   where = get_int(&d->text.length, where);
   d->text.where = (char *) MALLOC(d->text.length);
   memcpy(d->text.where, where, d->text.length);
   where += d->text.length;
   where = get_string(d->name, where);
   num = (d->id) % NOTE_HASH_SIZE;
   if (n_hash[num])
      d->hash_next = n_hash[num];
   else
      d->hash_next = 0;
   n_hash[num] = d;
   return where;
}

/*
 * load all notes from disk this should be changed for arbitary hashes
 */

void            init_notes(void)
{
   int             n, length, fd, count;
   char           *oldstack, *where, *scan;
   oldstack = stack;

   log("boot", "Loading notes from disk.");
   fd = open("files/notes/track", O_RDONLY | O_NDELAY);
   if (fd < 0)
   {
      vlog("mail_error", "Failed to load track file");
      unique = 1;
      news_start = 0;
      snews_start = 0;
      news_count = 0;
      snews_count = 0;
   } else
   {
      if (read(fd, oldstack, 20) < 0)
    handle_error("Can't read track file.");
      stack = get_int(&unique, stack);
      stack = get_int(&news_start, stack);
      stack = get_int(&news_count, stack);
      stack = get_int(&snews_start, stack);
      stack = get_int(&snews_count, stack);
      close(fd);
      stack = oldstack;
   }


   for (n = 0; n < NOTE_HASH_SIZE; n++)
   {

      discard_hash(n);
      nhash_update[n] = 0;
        sprintf(oldstack, "files/notes/hash%d", n);
      fd = open(oldstack, O_RDONLY | O_NDELAY);
      if (fd < 0)
        vlog("mail_error", "Failed to load note hash%d", n);
      else
      {
        length = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        if (length)
        {
          where = (char *) MALLOC(length);
          if (read(fd, where, length) < 0)
            handle_error("Can't read note file.");
          scan = get_int(&count, where);
          for (; count; count--)
            scan = extract_note(scan);
          FREE(where);
        }
        close(fd);
      }
   }
   stack = oldstack;
}



/* store info for a player save */

void            construct_mail_save(saved_player * sp)
{
   int             count = 0;
   recmail *rscan, *temp_rec[100];
   stack = store_int(stack, sp->mail_sent);
   if (!(sp->received_list)) /* SPANG */
      stack = store_int(stack, 0);
   else
   {
      for(rscan = sp->received_list; count<100 && rscan; rscan=rscan->next, count++)
        temp_rec[count] = rscan;
      
      /* save the number of items here */
      stack = store_int(stack, count);
      if(count) {
        /* step backwards through temp_rec */
        for(count--; count>=0; count--) {
          stack = store_int(stack, temp_rec[count]->mail_received);
          stack = store_int(stack, temp_rec[count]->flags);
        }
      }
   }
}


recmail *create_recmail(saved_player *sp)
{
  recmail *new;
  new = (recmail *) MALLOC(sizeof(recmail));
  new->next = sp->received_list;
  sp->received_list = new;
  new->flags = 0;
  new->mail_received = -1;
  return new;
}

/* get info back from a player save */

char           *retrieve_mail_data(saved_player * sp, char *where)
{
   int             count = 0;
   recmail *new;
   where = get_int(&sp->mail_sent, where);
   where = get_int(&count, where);
   sp->received_list=0;
   while(count)
   {
      new = create_recmail(sp);
      where = get_int(&new->mail_received, where);
      where = get_int(&new->flags, where);
      count--;
   } 
   return where;
}


/* su command to check out note hashes */

void            list_notes(player * p, char *str)
{
   int             num;
   note           *scan;
   char           *oldstack;

   oldstack = stack;

   num = atoi(str);
   if (num < 0 || num >= NOTE_HASH_SIZE)
   {
      tell_player(p, " Number not in range.\n");
      return;
   }
   strcpy(stack, " Notes:\n");
   stack = strchr(stack, 0);
   for (scan = n_hash[num]; scan; scan = scan->hash_next)
   {
      if (scan->flags & NEWS_ARTICLE)
         sprintf(stack, "  [%d] %s + %s", scan->id,
                 scan->name, scan->header);
      else if (scan->flags & SNEWS_ARTICLE)
         sprintf(stack, "  [%d] %s + %s", scan->id,
                 scan->name, scan->header);
      else
         sprintf(stack, "  [%d] %s - %s", scan->id, scan->name,
                 bit_string(scan->flags));
      stack = strchr(stack, 0);
      if (scan->flags & NOT_READY)
      {
    strcpy(stack, "(DEFUNCT)\n");
    stack = strchr(stack, 0);
      } else
    *stack++ = '\n';
   }
   strcpy(stack, " --\n");
   stack = strchr(stack, 0);
   stack++;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* the news command */
void            news_command(player * p, char *str)
{
   if (p->edit_info)
   {
      tell_player(p, " Can't do news commands whilst in editor/pager.\n");
      return;
   }
   if ((*str == '/') && (p->input_to_fn == news_command))
   {
      match_commands(p, str + 1);
      if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
      {
         do_prompt(p, "News Mode >");
         p->mode |= NEWSEDIT;
      }
      return;
   }
   if (!*str)
      if (p->input_to_fn == news_command)
      {
    tell_player(p, " Format: news <action>\n");
    if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
    {
       do_prompt(p, "News Mode >");
       p->mode |= NEWSEDIT;
    }
    return;
      } else
      {
    tell_player(p, " Entering news mode. Use 'end' to leave.\n"
           " '/<command>' does normal commands.\n");
    p->flags &= ~PROMPT;
    p->input_to_fn = news_command;
      }
   else
      sub_command(p, str, news_list);
   if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
   {
      do_prompt(p, "News Mode >");
      p->mode |= NEWSEDIT;
   }
}



/* the news command */
void            snews_command(player * p, char *str)
{
   if (p->edit_info)
   {
      tell_player(p, " Can't do snews commands whilst in editor/pager.\n");
      return;
   }
   if ((*str == '/') && (p->input_to_fn == snews_command))
   {
      match_commands(p, str + 1);
      if (!(p->flags & PANIC) && (p->input_to_fn == snews_command))
      {
         do_prompt(p, "SuNews Mode >");
         p->mode |= SNEWSEDIT;
      }
      return;
   }
   if (!*str)
      if (p->input_to_fn == snews_command)
      {
    tell_player(p, " Format: snews <action>\n");
    if (!(p->flags & PANIC) && (p->input_to_fn == snews_command))
    {
       do_prompt(p, "SuNews Mode >");
       p->mode |= SNEWSEDIT;
    }
    return;
      } else
      {
    tell_player(p, " Entering snews mode. Use 'end' to leave.\n"
           " '/<command>' does normal commands.\n");
    p->flags &= ~PROMPT;
    p->input_to_fn = snews_command;
      }
   else
      sub_command(p, str, snews_list);
   if (!(p->flags & PANIC) && (p->input_to_fn == snews_command))
   {
      do_prompt(p, "SuNews Mode >");
      p->mode |= SNEWSEDIT;
   }
}


/* ok, this is done too many times not to standardise it */
int		mail_saved_check(player *p)
{
  if(!(p->saved)) {
    tell_player(p, " You have no save information, "
                     "and therefore no mail either.\n");
    return 0;
  }
  return 1;
}


/* the mail command */

void            mail_command(player * p, char *str)
{
   if (p->edit_info)
   {
      tell_player(p, " Can't do mail commands whilst in editor/pager.\n");
      return;
   }
   if(!mail_saved_check(p))
     return;
   if ((*str == '/') && (p->input_to_fn == mail_command))
   {
      match_commands(p, str + 1);
      if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
      { 
         do_prompt(p, "Mail Mode >");
         p->mode |= MAILEDIT;
      }
      return;
   }
   if (!*str)
      if (p->input_to_fn == mail_command)
      {
         tell_player(p, " Format: mail <action>\n");
         if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
         {
            do_prompt(p, "Mail Mode >");
            p->mode |= MAILEDIT;
         }
         return;
      } else
      {
    tell_player(p, " Entering mail mode. Use 'end' to leave.\n"
           " '/<command>' does normal commands.\n");
    p->flags &= ~PROMPT;
    p->input_to_fn = mail_command;
      }
   else
      sub_command(p, str, mail_list);
   if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
   {
      do_prompt(p, "Mail Mode >");
      p->mode |= MAILEDIT;
   }
}

/* view news commands */

void            view_news_commands(player * p, char *str)
{
   view_sub_commands(p, news_list);
}

/* view snews commands */

void            view_snews_commands(player * p, char *str)
{
   view_sub_commands(p, snews_list);
}

/* view mail commands */

void            view_mail_commands(player * p, char *str)
{
   view_sub_commands(p, mail_list);
}

/* exit news mode */

void            exit_news_mode(player * p, char *str)
{
   if (p->input_to_fn != news_command)
      return;
   tell_player(p, " Leaving news mode.\n");
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   p->mode &= ~NEWSEDIT;
}

/* exit snews mode */

void            exit_snews_mode(player * p, char *str)
{
   if (p->input_to_fn != snews_command)
      return;
   tell_player(p, " Leaving snews mode.\n");
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   p->mode &= ~SNEWSEDIT;
}

/* exit mail mode */

void            exit_mail_mode(player * p, char *str)
{
   if (p->input_to_fn != mail_command)
      return;
   tell_player(p, " Leaving mail mode.\n");
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   p->mode &= ~MAILEDIT;
}

/* finds news article number x */

note           *find_news_article(int n)
{
   int             integrity = 0;
   note           *scan;
   if (n < 1 || n > news_count)
      return 0;

   for (n--, scan = find_note(news_start); n; n--, integrity++)
      if (scan)
    scan = find_note(scan->next_item);
      else
      {
    news_count = integrity;
    return 0;
      }
   return scan;
}


/* finds snews article number x */

note           *find_snews_article(int n)
{
   int             integrity = 0;
   note           *scan;
   if (n < 1 || n > snews_count)
      return 0;

   for (n--, scan = find_note(snews_start); n; n--, integrity++)
      if (scan)
    scan = find_note(scan->next_item);
      else
      {
    snews_count = integrity;
    return 0;
      }
   return scan;
}

/* list news articles */

void            list_news(player * p, char *str)
{
   char           *oldstack, middle[80];
   int             page, pages, count, article, ncount = 1;
   note           *scan;
   oldstack = stack;

   if (!news_count)
   {
      tell_player(p, " No news articles to view.\n");
      return;
   }
   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;

   pages = (news_count - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;

   if (news_count == 1)
      strcpy(middle, "There is one news article");
   else
      sprintf(middle, "There are %s articles",
         number2string(news_count));
   titled_line(p, middle);

   count = page * (TERM_LINES - 2);

   for (article = news_start; count; count--, ncount++)
   {
      scan = find_note(article);
      if (!scan)
      {
    tell_player(p, " Bad news listing, aborted.\n");
    log("mail_error", "Bad news list");
    stack = oldstack;
    return;
      }
      article = scan->next_item;
   }
   for (count = 0; (count < (TERM_LINES - 1)); count++, ncount++)
   {
      scan = find_note(article);
      if (!scan)
    break;
      if (p->residency & ADMIN)
    sprintf(stack, "(%d) [%d] ", scan->id, ncount);
      else
    sprintf(stack, "[%d] ", ncount);
      while (*stack)
    stack++;
      if (ncount < 10)
    *stack++ = ' ';
      strcpy(stack, scan->header);
      while (*stack)
    stack++;
      if (scan->flags & ANONYMOUS)
      {
    if (p->residency & ADMIN)
       sprintf(stack, " <%s>\n", scan->name);
    else
       strcpy(stack, "\n");
      } else
    sprintf(stack, " (%s)\n", scan->name);
      stack = strchr(stack, 0);
      article = scan->next_item;
   }

   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   titled_line(p, middle);

   *stack++ = 0;
   tell_player(p, oldstack);

   stack = oldstack;
}


/* list snews articles */

void            list_snews(player * p, char *str)
{
   char           *oldstack, middle[80];
   int             page, pages, count, article, ncount = 1;
   note           *scan;
   oldstack = stack;

   if (!snews_count)
   {
      tell_player(p, " No snews articles to view.\n");
      return;
   }
   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;

   pages = (snews_count - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;

   if (snews_count == 1)
      strcpy(middle, "There is one snews article");
   else
      sprintf(middle, "There are %s articles",
         number2string(snews_count));
   titled_line(p, middle);

   count = page * (TERM_LINES - 2);

   for (article = snews_start; count; count--, ncount++)
   {
      scan = find_note(article);
      if (!scan)
      {
    tell_player(p, " Bad snews listing, aborted.\n");
    log("mail_error", "Bad snews list");
    stack = oldstack;
    return;
      }
      article = scan->next_item;
   }
   for (count = 0; (count < (TERM_LINES - 1)); count++, ncount++)
   {
      scan = find_note(article);
      if (!scan)
    break;
      if (p->residency & ADMIN)
    sprintf(stack, "(%d) [%d] ", scan->id, ncount);
      else
    sprintf(stack, "[%d] ", ncount);
      while (*stack)
    stack++;
      if (ncount < 10)
    *stack++ = ' ';
      strcpy(stack, scan->header);
      while (*stack)
    stack++;
      if (scan->flags & ANONYMOUS)
      {
    if (p->residency & ADMIN)
       sprintf(stack, " <%s>\n", scan->name);
    else
       strcpy(stack, "\n");
      } else
    sprintf(stack, " (%s)\n", scan->name);
      stack = strchr(stack, 0);
      article = scan->next_item;
   }

   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   titled_line(p, middle);

   *stack++ = 0;
   tell_player(p, oldstack);

   stack = oldstack;
}


/* post a news article */

void            quit_post(player * p)
{
   tell_player(p, " Article NOT posted.\n");
   remove_note((note *) p->edit_info->misc);
   p->mode &= ~NEWSEDIT;
}


/* post a news article */

void            quit_spost(player * p)
{
   tell_player(p, " Article NOT posted.\n");
   remove_note((note *) p->edit_info->misc);
   p->mode &= ~SNEWSEDIT;
}


void            end_post(player * p)
{
   note           *article;
   char           *oldstack;
   oldstack = stack;

   article = (note *) p->edit_info->misc;
   stack = store_string(oldstack, p->edit_info->buffer);
#ifdef OSF
    article->text.length = (long) stack - (long) oldstack;
#else
    article->text.length = (int) stack - (int) oldstack;
#endif
   article->text.where = (char *) MALLOC(article->text.length);
   memcpy(article->text.where, oldstack, article->text.length);
   stack = oldstack;

   article->next_item = news_start;
   news_start = article->id;
   news_count++;
   article->flags &= ~NOT_READY;
   article->read_count = 0;
   tell_player(p, " Article posted....\n");
   p->mode &= ~NEWSEDIT;
   sprintf(stack, "-=> A new news article '%s' has been posted", article->header);
   stack = strchr(stack, 0);
   if (article->flags & ANONYMOUS)
      strcpy(stack, " anonymously. <=-\n");
   else
      sprintf(stack, " by %s. <=-\n", p->name);
   stack = end_string(oldstack);
   news_wall_but(p, oldstack);
   stack = oldstack;
   if (p->edit_info->input_copy == news_command)
   {
      do_prompt(p, "News Mode >");
      p->mode |= NEWSEDIT;
   }
}


void            end_spost(player * p)
{
   note           *article;
   char           *oldstack;
   oldstack = stack;

   article = (note *) p->edit_info->misc;
   stack = store_string(oldstack, p->edit_info->buffer);
#ifdef OSF
    article->text.length = (long) stack - (long) oldstack;
#else
    article->text.length = (int) stack - (int) oldstack;
#endif
   article->text.where = (char *) MALLOC(article->text.length);
   memcpy(article->text.where, oldstack, article->text.length);
   stack = oldstack;

   article->next_item = snews_start;
   snews_start = article->id;
   snews_count++;
   article->flags &= ~NOT_READY;
   article->read_count = 0;
   tell_player(p, " Article posted....\n");
   p->mode &= ~SNEWSEDIT;
   sprintf(stack, "-=> A new snews article '%s' has been posted", article->header);
   stack = strchr(stack, 0);
   if (article->flags & ANONYMOUS)
      strcpy(stack, " anonymously. <=-\n");
   else
      sprintf(stack, " by %s. <=-\n", p->name);
   stack = end_string(oldstack);
   snews_wall_but(p, oldstack);
   stack = oldstack;
   if (p->edit_info->input_copy == news_command)
   {
      do_prompt(p, "SuNews Mode >");
      p->mode |= SNEWSEDIT;
   }
}


void            post_news(player * p, char *str)
{
   note           *article;
   char           *secure;
   if (!*str)
   {
      tell_player(p, " Format: post <header>\n");
      return;
   }

   secure=strchr(str,')');
   if (secure)
     if (*(++secure)=='\0')
       if (strchr(str,'('))
	 {
	   tell_player(p," You may not have bracketed comments at the end "
		       "of news titles\n");
	   return;
	 }

   secure=strchr(str,'>');
   if (secure)
     if (*(++secure)=='\0')
       if (strchr(str,'>'))
	 {
	   tell_player(p," You may not have bracketed comments at the end "
		       "of news titles\n");
	   return;
	 }
   
   str = tidy_article_title(str);
   
   article = create_note();
   if(!article) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   strncpy(article->header, str, MAX_TITLE - 1);
   article->flags |= NEWS_ARTICLE;
   if(!strcmp(p->command_used->text, "apost"))
      article->flags |= ANONYMOUS;
   strcpy(article->name, p->name);
   tell_player(p, " Now enter the main body text for the article.\n");
   *stack = 0;
   start_edit(p, MAX_ARTICLE_SIZE, end_post, quit_post, stack);
   if (p->edit_info)
      p->edit_info->misc = (void *) article;
   else
      remove_note(article);
}


void            post_snews(player * p, char *str)
{
   note           *article;
   char           *secure;
   if (!*str)
   {
      tell_player(p, " Format: post <header>\n");
      return;
   }

   secure=strchr(str,')');
   if (secure)
     if (*(++secure)=='\0')
       if (strchr(str,'('))
	 {
	   tell_player(p," You may not have bracketed comments at the end "
		       "of snews titles\n");
	   return;
	 }

   secure=strchr(str,'>');
   if (secure)
     if (*(++secure)=='\0')
       if (strchr(str,'>'))
	 {
	   tell_player(p," You may not have bracketed comments at the end "
		       "of snews titles\n");
	   return;
	 }
   
   str = tidy_article_title(str);
   
   article = create_note();
   if(!article) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   strncpy(article->header, str, MAX_TITLE - 1);
   article->flags |= SNEWS_ARTICLE;
   strcpy(article->name, p->name);
   tell_player(p, " Now enter the main body text for the article.\n");
   *stack = 0;
   start_edit(p, MAX_ARTICLE_SIZE, end_spost, quit_spost, stack);
   if (p->edit_info)
      p->edit_info->misc = (void *) article;
   else
      remove_note(article);
}

/* follow up an article */

void            followup(player * p, char *str)
{
   char           *oldstack, *body, *indent;
   note           *article, *old;
   oldstack = stack;

   old = find_news_article(atoi(str));
   if (!old)
   {
      vtell_player(p, " No such news article '%s'\n", str);
      return;
   }
   article = create_note();
   if(!article) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   if (strstr(old->header, "Re: ") == old->header)
      strcpy(article->header, old->header);
   else
   {
      sprintf(stack, "Re: %s", old->header);
      strncpy(article->header, stack, MAX_TITLE - 1);
   }
   article->flags |= NEWS_ARTICLE;
   
   if(!strcmp(p->command_used->text, "afollowup"))
      article->flags |= ANONYMOUS;

   strcpy(article->name, p->name);

   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;

   if (old->flags & ANONYMOUS)
      sprintf(stack, "From anonymous article written on %s ...\n",
         convert_time(old->date));
   else
      sprintf(stack, "On %s, %s wrote ...\n",
         convert_time(old->date), old->name);
   stack = strchr(stack, 0);

   while (*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while (*indent && *indent != '\n')
    *stack++ = *indent++;
      *stack++ = '\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;

   tell_player(p, " Please trim article as much as is possible ....\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_post, quit_post, body);
   if (p->edit_info)
      p->edit_info->misc = (void *) article;
   else
      remove_note(article);
   stack = oldstack;
}


/* follow up an article */

void            sfollowup(player * p, char *str)
{
   char           *oldstack, *body, *indent;
   note           *article, *old;
   oldstack = stack;

   old = find_snews_article(atoi(str));
   if (!old)
   {
      vtell_player(p, " No such snews article '%s'\n", str);
      return;
   }
   article = create_note();
   if(!article) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   if (strstr(old->header, "Re: ") == old->header)
      strcpy(article->header, old->header);
   else
   {
      sprintf(stack, "Re: %s", old->header);
      strncpy(article->header, stack, MAX_TITLE - 1);
   }
   article->flags |= SNEWS_ARTICLE;
   strcpy(article->name, p->name);
   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;

      sprintf(stack, "On %s, %s wrote ...\n",
         convert_time(old->date), old->name);
   stack = strchr(stack, 0);

   while (*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while (*indent && *indent != '\n')
    *stack++ = *indent++;
      *stack++ = '\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;

   tell_player(p, " Please trim article as much as is possible ....\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_spost, quit_spost, body);
   if (p->edit_info)
      p->edit_info->misc = (void *) article;
   else
      remove_note(article);
   stack = oldstack;
}

/* wipe a news article */

void            remove_article(player * p, char *str)
{
   note           *article, *prev, *scan;
   char           *oldstack;
   oldstack = stack;
   article = find_news_article(atoi(str));
   if (!article)
   {
      vtell_player(p, " No such news article '%s'\n", str);
      return;
   }
   strcpy(stack, article->name);
   lower_case(stack);
   if (!(p->residency & ADMIN) && strcmp(stack, p->lower_name))
   {
      tell_player(p, " You can't remove an article that isn't yours.\n");
      return;
   }
   scan = find_note(news_start);
   if (scan == article)
      news_start = article->next_item;
   else
   {
      do
      {
    prev = scan;
    scan = find_note(scan->next_item);
      } while (scan != article);
      prev->next_item = article->next_item;
   }
   news_count--;
   remove_note(article);
   tell_player(p, " Article removed.\n");
   stack = oldstack;
}


/* wipe a snews article */

void            remove_sarticle(player * p, char *str)
{
   note           *article, *prev, *scan;
   char           *oldstack;
   oldstack = stack;
   article = find_snews_article(atoi(str));
   if (!article)
   {
      vtell_player(p, " No such snews article '%s'\n", str);
      return;
   }
   strcpy(stack, article->name);
   lower_case(stack);
   if (!(p->residency & ADMIN) && strcmp(stack, p->lower_name))
   {
      tell_player(p, " You can't remove an article that isn't yours.\n");
      return;
   }
   scan = find_note(snews_start);
   if (scan == article)
      snews_start = article->next_item;
   else
   {
      do
      {
    prev = scan;
    scan = find_note(scan->next_item);
      } while (scan != article);
      prev->next_item = article->next_item;
   }
   snews_count--;
   remove_note(article);
   tell_player(p, " Article removed.\n");
   stack = oldstack;
}

/* read an article */

void            read_article(player * p, char *str)
{
   char           *oldstack;
   note           *article;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: read <article-number>\n");
      return;
   }
   article = find_news_article(atoi(str));
   if (!article)
   {
      vtell_player(p, " No such news article '%s'\n", str);
      return;
   }
   article->read_count++;
   if (article->flags & ANONYMOUS)
   {
      if (p->residency & ADMIN)
    sprintf(stack, "Subject: %s\nPosted anonymously on %s by %s.\n",
       article->header, convert_time(article->date), article->name);
      else
    sprintf(stack, "Subject: %s\nPosted anonymously on %s.\n",
       article->header, convert_time(article->date));
   } else
      sprintf(stack, "Subject: %s\nPosted by %s on %s.\n", article->header,
         article->name, convert_time(article->date));
   stack = strchr(stack, 0);
   if (article->read_count == 1)
      strcpy(stack, "Article read once only.\n\n");
   else
      sprintf(stack, "Article has been read %s times.\n\n",
         number2string(article->read_count));
   stack = strchr(stack, 0);
   get_string(stack, article->text.where);
   stack = end_string(stack);
   pager(p, oldstack, 0);
   stack = oldstack;
}




/* read an article */

void            read_sarticle(player * p, char *str)
{
   char           *oldstack;
   note           *article;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: read <article-number>\n");
      return;
   }
   article = find_snews_article(atoi(str));
   if (!article)
   {
      vtell_player(p, " No such snews article '%s'\n", str);
      return;
   }
   article->read_count++;
   if (article->flags & ANONYMOUS)
   {
      if (p->residency & ADMIN)
    sprintf(stack, "Subject: %s\nPosted anonymously on %s by %s.\n",
       article->header, convert_time(article->date), article->name);
      else
    sprintf(stack, "Subject: %s\nPosted anonymously on %s.\n",
       article->header, convert_time(article->date));
   } else
      sprintf(stack, "Subject: %s\nPosted by %s on %s.\n", article->header,
         article->name, convert_time(article->date));
   stack = strchr(stack, 0);
   if (article->read_count == 1)
      strcpy(stack, "Article read once only.\n\n");
   else
      sprintf(stack, "Article has been read %s times.\n\n",
         number2string(article->read_count));
   stack = strchr(stack, 0);
   get_string(stack, article->text.where);
   stack = end_string(stack);
   pager(p, oldstack, 0);
   stack = oldstack;
}

/* mail stuff */



/* count how many mails have been posted */

int             posted_count(player * p)
{
   int             scan, count = 0;
   note           *mail, *next;

   if (!p->saved)
      return 0;
   scan = p->saved->mail_sent;
   mail = find_note(scan);
   if (!mail)
   {
      p->saved->mail_sent = 0;
      return 0;
   }
   while (mail)
   {
      count++;
      scan = mail->next_item;
      next = find_note(scan);
      if (!next && scan)
      {
    mail->next_item = 0;
    mail = 0;
      } else
    mail = next;
   }
   return count;
}



/* view mail that has been sent */

void view_sent(player * p, char *str)
{
   char *oldstack;
   note *mail, *next;
   int count = 1, scan;
   saved_player   *sp;

   oldstack = stack;
   if (*str && p->residency & ADMIN)
   {
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " No such player in save files.\n");
         return;
      } else
      {
         scan = sp->mail_sent;
         mail = find_note(scan);
         if (!mail)
         {
            sp->mail_sent = 0;
            tell_player(p, " They have sent no mail.\n");
            return;
         }
      }
   } else
   {
      if (!mail_saved_check(p))
         return;
      scan = p->saved->mail_sent;
      mail = find_note(scan);
      if (!mail)
      {
         p->saved->mail_sent = 0;
         tell_player(p, " You have sent no mail.\n");
         return;
      }
   }
   strcpy(stack, "Listing mail sent...\n");
   stack = strchr(stack, 0);
   while (mail)
   {
      if (p->residency & ADMIN)
      {
         sprintf(stack, "(%d) [%d] %d - %s\n", mail->id, count,
                 mail->read_count, mail->header);
      } else
      {
         sprintf(stack, "[%d] %d - %s\n", count, mail->read_count,
                 mail->header);
      }
      stack = strchr(stack, 0);
      scan = mail->next_item;
      count++;
      next = find_note(scan);
      if (!next && scan)
      {
         mail->next_item = 0;
         mail = 0;
      } else
      {
         mail = next;
      }
   }
   sprintf(stack, " You have sent %d out of your maximum of %d mails.\n",
           count - 1, p->max_mail);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


/* this fn goes through the received list and removes dud note pointers */

void            reconfigure_received_list(saved_player * sp)
{
   recmail *scan;

   scan = sp->received_list;
   while(scan) {
     if(scan->mail_received<0 || scan->mail_received>MYMAXINT)
     {
        /* delete this one, moving pointer to next item if any */
        scan = destroy_recmail(sp, scan);
     }
     else if(find_note(scan->mail_received)) /* try to find it */
     {
        /* found mail, leave it alone, move to next one */
        scan=scan->next;
     }
     else /* couldnt find it, so delete */
     {
        /* delete this one, moving pointer to next item if any */
        scan = destroy_recmail(sp, scan);
     }
   }
}


/* view mail that has been received */

void            view_received(player * p, char *str)
{
   char           *oldstack, middle[80], *page_input;
   int             page, pages, mcount = 0, ncount = 1,
                   n;
   recmail	  *scan;
   note           *mail;
   saved_player   *sp;
   oldstack = stack;

   if (*str && !isdigit(*str) && p->residency & ADMIN)
   {
      /* admin checking someone elses mail */
      page_input = next_space(str);
      if (*page_input)
         *page_input++ = 0;
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " No such player in save files.\n");
         return;
      } 
      else
         scan = sp->received_list;
      if(sp->residency==BANISHD) {
        tell_player(p, " That is a banished name.\n");
        return;
      }
      if(sp->residency==STANDARD_ROOMS) {
        tell_player(p, " That is a system room.\n");
        return;
      }
      str = page_input;
   } 
   else
   {
      if (!mail_saved_check(p))
         return;
      sp = p->saved;
      scan = sp->received_list;
   }

   if (!scan)
   {
      tell_player(p, " You have received no mail.\n");
      return;
   }

   /* how many mails? */
   for (; scan; scan=scan->next)
      mcount++;
   scan = sp->received_list;

   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;
   pages = (mcount - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;
   if (mcount == 1)
      strcpy(middle, "You have received one letter");
   else
      sprintf(middle, "You have received %s letters",
         number2string(mcount));
   titled_line(p, middle);
   
   /* how many mails to skip due to page 'x' */
   ncount = page * (TERM_LINES - 2);
   for(n=0; n<ncount; n++, scan=scan->next);

   for(n=0, ncount++; n<(TERM_LINES-1); n++, ncount++, scan=scan->next)
   {
      if (!scan)
         break;
      mail = find_note(scan->mail_received);
      if (!mail)
      {
         /* TEST */
         stack = oldstack;
         tell_player(p, " Found mail that owner had deleted ...\n");
         reconfigure_received_list(sp);
         if (sp != p->saved) /* if its an admin */
         {
            tell_player(p, " Reconfigured ... try again\n");
            return;
         }
         view_received(p, str);
         return;
      }
      if(p->residency & ADMIN)
      {
         sprintf(stack, "(%d) ", mail->id);
         stack = strchr(stack, 0);
      }
      sprintf(stack, "[%d] ", ncount);
      stack = strchr(stack, 0);
      if (ncount < 10)
         *stack++ = ' ';
      /* new/read/etc shit */
      if(scan->flags & reNEW)
        strcpy(stack, "<New>  ");
      else if(scan->flags & reREPLIED)
        strcpy(stack, "<Rpld> ");
      else if(scan->flags & reFORWARDED)
        strcpy(stack, "<Fwdd> ");
      else
        strcpy(stack, "<Read> ");
      stack = strchr(stack, 0);
      if(scan->flags & reFRIEND)
        *stack++='*';
      else
        *stack++=' ';
      strcpy(stack, mail->header);
      stack = strchr(stack, 0);
      if (mail->flags & ANONYMOUS)
      {
         if (p->residency & ADMIN)
            sprintf(stack, " <%s>\n", mail->name);
         else
            strcpy(stack, "\n");
      } 
      else
         sprintf(stack, " (%s)\n", mail->name);
      stack = strchr(stack, 0);
   }

   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   titled_line(p, middle);

   *stack++ = 0;
   tell_player(p, oldstack);

   stack = oldstack;
}


/* send a letter */


void		quit_mail(player * p)
{
  tell_player(p, " Letter NOT posted.\n");
  remove_note((note *) p->edit_info->misc);
}


/* function for the below to get names of p's friend and put them into a
   string */
char		*compose_friend_string(player *p)
{
  char *oldstack, *buffer;
  saved_player *sp;
  list_ent *l;
  int count=0, length;
  
  oldstack = stack;
  
  if(!p->saved || !p->saved->list_top) {
    tell_player(p, " You have no list, and therefore no friends.\n");
    return 0;
  }
  
  sp = p->saved;
  for(l=sp->list_top; l; l=l->next)
    if(l->flags & FRIEND && strcasecmp(l->name, "everyone")) {
      sprintf(stack, "%s,", l->name); /* dump them to the stack */
      stack = strchr(stack, 0);
      count++;
    }  
  if(!count) {
    tell_player(p, " You have no friends though!\n");
    stack = oldstack;
    return 0;
  }
  
  /* got it */
  *stack++=0;
  length = strlen(oldstack)+1;
  buffer = (char *) malloc(length+2);
  strncpy(buffer, oldstack, length);
  stack = oldstack;
  return buffer;
}


void            end_mail(player * p)
{
   note           *mail;
   char           *oldstack, *name_list, *body, *tcpy, *comp, *text, *friend_list=0;
   saved_player  **player_list, **pscan, **pfill;
   recmail	  *new;
   player         *on;
   int             n, m, receipt_count=0, friend_mail=0;
   oldstack = stack;

   mail = (note *) p->edit_info->misc;

   align(stack);
   player_list = (saved_player **) ALFIX stack;
   
   /* mail post friends crap */
   if(!strcmp(mail->text.where, "friends")) {
     friend_list = compose_friend_string(p);
     friend_mail = 1;
   }
   if(!friend_mail)
      receipt_count = saved_tag(p, mail->text.where, 0);
   else if(friend_list)
      receipt_count = saved_tag(p, friend_list, 1);
   
   if (mail->text.where)
      FREE(mail->text.where);
   mail->text.where = 0;
   if(friend_list)
     free(friend_list);   
   if (!receipt_count)
   {
      if(!friend_mail)
         tell_player(p, " No one to send the letter to !\n");
      remove_note(mail);
      stack = oldstack;
      if (p->edit_info->input_copy == mail_command)
      {
         do_prompt(p, "Mail Mode >");
         p->mode |= MAILEDIT;
      }
      return;
   }
   if (!p->saved)
   {
      tell_player(p, " Eeek, no save file !\n");
      remove_note(mail);
      stack = oldstack;
      if (p->edit_info->input_copy == mail_command)
      {
         do_prompt(p, "Mail Mode >");
         p->mode |= MAILEDIT;
      }
      return;
   }
   /* player_list is the list of people to which the mail is being sent */
   pscan = player_list;
   pfill = player_list;
   m = receipt_count; 
   for (n = 0; n < m; n++) /* n is counter below 'm' (receipt count) */
   {
      if (((*pscan)->residency == STANDARD_ROOMS) /* TEST */
          || ((*pscan)->residency & BANISHD))
      {
         text = stack;
         if((*pscan)->residency & BANISHD)
            sprintf(stack, " [%s is banished!]\n", (*pscan)->lower_name);
         else
            sprintf(stack, " [%s is a system room!]\n", (*pscan)->lower_name);
         stack = end_string(stack);
         tell_player(p, text);
         stack = text;
         receipt_count--;
         pscan++;
      } 
      else
      {
         if ((mail->flags & ANONYMOUS)
             && ((*pscan)->saved_flags & NO_ANONYMOUS))
         {
            text = stack;
            sprintf(stack, " %s is not receiving anonymous mail.\n",
                    (*pscan)->lower_name);
            stack = end_string(stack);
            tell_player(p, text);
            stack = text;
            receipt_count--;
            pscan++;
         } else
            *pfill++ = *pscan++;
      }
   }

   /* now player_list has only list of actual players to receive the mail */
   if (receipt_count > 0) /* if anyone got it */
   {
      pscan = player_list; /* ptr to start of player list */
      name_list = stack;   /* */
      if (mail->flags & SUPRESS_NAME)
      {
         strcpy(stack, "Anonymous");
         stack = end_string(stack);
      } 
      else
      {
         strcpy(stack, (*pscan)->lower_name);
         stack = strchr(stack, 0);
         if (receipt_count > 1)
         {
            pscan++;
            for (n = 2; n < receipt_count; n++, pscan++)
            {
               sprintf(stack, ", %s", (*pscan)->lower_name);
               stack = strchr(stack, 0);
            }
            sprintf(stack, " and %s", (*pscan)->lower_name);
            stack = strchr(stack, 0);
         }
         *stack++ = 0;
      }

      body = stack;
      if(!friend_mail)
        sprintf(stack, " Sending mail to %s.\n", name_list);
      else
        strcpy(stack, " Sending mail to your friends.\n");
      stack = end_string(stack);
      tell_player(p, body);
      stack = body;

      if(!friend_mail) {
         if (mail->flags & ANONYMOUS)
            sprintf(stack, " Anonymous mail dated %s.\nSubject: %s\nTo: %s\n\n",
               convert_time(mail->date), mail->header, name_list);
         else
            sprintf(stack, " Mail dated %s.\nSubject: %s\nTo: %s\nFrom: %s\n\n",
               convert_time(mail->date), mail->header, name_list, mail->name);
         stack = strchr(stack, 0);
      }
      else {
         sprintf(stack, " Mail dated %s.\nSubject: %s\nTo: Friends of %s\nFrom: %s\n\n",
               convert_time(mail->date), mail->header, mail->name, mail->name);
         stack = strchr(stack, 0);
      }
      tcpy = p->edit_info->buffer;
      
      for (n = 0; n < p->edit_info->size; n++)
         *stack++ = *tcpy++;

      comp = stack;
      stack = store_string(stack, body);
#ifdef OSF
       mail->text.length = (long) stack - (long) comp;
#else
       mail->text.length = (int) stack - (int) comp;
#endif
      mail->text.where = (char *) MALLOC(mail->text.length);
      memcpy(mail->text.where, comp, mail->text.length);

      mail->next_item = p->saved->mail_sent;
      p->saved->mail_sent = mail->id;

      text = stack;
      command_type |= (HIGHLIGHT|PERSONAL|WARNING);

      if (mail->flags & ANONYMOUS)
         sprintf(stack, "     -=>  New mail, '%s' sent anonymously.\n\n", mail->header);
      else
         sprintf(stack, "     -=>  New mail, '%s' from %s.\n\n", mail->header, mail->name);
      stack = end_string(stack);

      pscan = player_list;
      /* do each person */
      for (n = 0; n < receipt_count; n++, pscan++)
      {
         /* give them the said mail */
         new = create_recmail((*pscan));
         new->flags = reNEW;
         if(friend_mail)
           new->flags |= reFRIEND;
         new->mail_received = mail->id;
         on = find_player_absolute_quiet((*pscan)->lower_name);
         if (on && on->saved_flags & MAIL_INFORM)
         {
            tell_player(on, "\007\n");
            tell_player(on, text);
         } 
      }

      command_type &= ~(HIGHLIGHT|PERSONAL|WARNING);

      mail->read_count = receipt_count;
      mail->flags &= ~NOT_READY;
      tell_player(p, " Mail posted....\n");
   } 
   else
      tell_player(p, " No mail posted.\n");
   p->mode &= ~MAILEDIT;
   if (p->edit_info->input_copy == mail_command)
   {
      do_prompt(p, "Mail Mode >");
      p->mode |= MAILEDIT;
   }
   stack = oldstack;
}


void            send_letter(player * p, char *str)
{
   note           *mail;
   char           *subject;
   int             length;

   if (posted_count(p) >= p->max_mail)
   {
      tell_player(p, " Sorry, you have reached your mail limit.\n");
      return;
   }
   subject = next_space(str);

   if (!*subject)
   {
      tell_player(p, " Format: post <character(s)> <subject>\n");
      return;
   }
   *subject++ = 0;
   subject = tidy_article_title(subject);

   mail = create_note();
   if(!mail) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }

   if(!strcmp(p->command_used->text, "apost"))
      mail->flags |= ANONYMOUS;

   strcpy(mail->name, p->name);
   strncpy(mail->header, subject, MAX_TITLE - 1);

   tell_player(p, " Enter main text of the letter...\n");
   *stack = 0;
   start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, stack);
   if (p->edit_info)
   {
      p->edit_info->misc = (void *) mail;
      length = strlen(str) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, str, length);
   } else
      remove_note(mail);
}


/* find the note corresponing to letter number */
/* action=1, tag as read
   action=2, tag as replied */
note           *find_received(saved_player * sp, int n, int findaction)
{
   int            count = 1;
   note           *mail;
   recmail *scan;
   if(!sp->received_list)
     return 0;
   scan = sp->received_list;
   for(scan=sp->received_list; scan; count++, scan=scan->next)
     if(count==n)
     {
        mail = find_note(scan->mail_received);
        if(!mail)
        {
           reconfigure_received_list(sp);
           return find_received(sp, n, 0);
        }
        if(findaction)
          scan->flags &= ~reNEW;
        /* keep last action */
        if(findaction==2) {
          scan->flags &= ~reFORWARDED;
          scan->flags |= reREPLIED;
        }
        if(findaction==3) {
          scan->flags &= ~reREPLIED;
          scan->flags |= reFORWARDED;
        }
        return mail;
     }
   return 0;
}


/* view a letter */

void            read_letter(player * p, char *str)
{
   char           *oldstack, *which;
   saved_player   *sp;
   note           *mail;
   oldstack = stack;

   which = str;
   sp = p->saved;
   if (!mail_saved_check(p))
      return;
   mail = find_received(sp, atoi(which), 1);
   if (!mail)
   {
      vtell_player(p, " No such letter '%s'\n", which);
      return;
   }
   get_string(stack, mail->text.where);
   stack = end_string(stack);
   pager(p, oldstack, 0);
   stack = oldstack;
}


note	*find_sent(saved_player *sp, int number)
{
   note *scan;
   scan = find_note(sp->mail_sent);
   if(!scan)
     return 0;
   for (number--; number; number--)
      if (scan)
        scan = find_note(scan->next_item);
      else
        break;
   if (!scan)
     return 0;
   return scan;
}

/* view a sent letter */

void            read_sent(player * p, char *str)
{
   char           *oldstack;
   saved_player   *sp;
   note           *mail;
   oldstack = stack;

   sp = p->saved;
   if (!mail_saved_check(p))
      return;
   mail = find_sent(sp, atoi(str));
   if (!mail)
   {
      vtell_player(p, " No such letter '%s'\n", str);
      return;
   }
   get_string(stack, mail->text.where);
   stack = end_string(stack);
   pager(p, oldstack, 0);
   stack = oldstack;
}


/* reply to a letter */

void            reply_letter(player * p, char *str)
{
   note           *mail, *old;
   char           *indent, *body, *oldstack;
   int             length;

   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: reply <number>\n");
      return;
   }
   if (posted_count(p) >= p->max_mail)
   {
      tell_player(p, " Sorry, you have reached your mail limit.\n");
      return;
   }
   if (!p->saved)
   {
      tell_player(p, " Eeek, no saved bits.\n");
      return;
   }
   /* tell find_received to tag it as one thats been replied to */
   old = find_received(p->saved, atoi(str), 2);
   if (!old)
   {
      vtell_player(p, " Can't find letter '%s'\n", str);
      return;
   }
   mail = create_note();
   if(!mail) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }

   if(!strcmp(p->command_used->text, "areply"))
      mail->flags |= ANONYMOUS;

   if (old->flags & ANONYMOUS)
      mail->flags |= SUPRESS_NAME;
   strcpy(mail->name, p->name);

   if (strstr(old->header, "Re: ") == old->header)
      strcpy(mail->header, old->header);
   else
   {
      sprintf(stack, "Re: %s", old->header);
      strncpy(mail->header, stack, MAX_TITLE - 1);
   }

   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;

   sprintf(stack, "In reply to '%s'\n", old->header);
   stack = strchr(stack, 0);

   while (*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while (*indent && *indent != '\n')
    *stack++ = *indent++;
      *stack++ = '\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;

   tell_player(p, " Please trim letter as much as possible...\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, body);
   if (p->edit_info)
   {
      p->edit_info->misc = (void *) mail;
      length = strlen(old->name) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, old->name, length);
   } else
      remove_note(mail);
   stack = oldstack;
}


/* reply to an article */

void            reply_article(player * p, char *str)
{
   note           *mail, *old;
   char           *indent, *body, *oldstack;
   int             length;

   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: reply <no>\n");
      return;
   }
   if (posted_count(p) >= p->max_mail)
   {
      tell_player(p, " Sorry, you have reached your mail limit.\n");
      return;
   }
   if (!p->saved)
   {
      tell_player(p, " Eeek, no saved bits.\n");
      return;
   }
   old = find_news_article(atoi(str));
   if (!old)
   {
      vtell_player(p, " Can't find article '%s'\n", str);
      return;
   }
   mail = create_note();
   if(!mail) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }

   if(!strcmp(p->command_used->text, "areply"))
      mail->flags |= ANONYMOUS;

   if (old->flags & ANONYMOUS)
      mail->flags |= SUPRESS_NAME;
   strcpy(mail->name, p->name);

   if (strstr(old->header, "Re: ") == old->header)
      strcpy(mail->header, old->header);
   else
   {
      sprintf(stack, "Re: %s", old->header);
      strncpy(mail->header, stack, MAX_TITLE - 1);
   }

   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;

   sprintf(stack, "In your article '%s' you wrote ...\n", old->header);
   stack = strchr(stack, 0);

   while (*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while (*indent && *indent != '\n')
    *stack++ = *indent++;
      *stack++ = '\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;

   tell_player(p, " Please trim letter as much as possible...\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, body);
   if (p->edit_info)
   {
      p->edit_info->misc = (void *) mail;
      length = strlen(old->name) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, old->name, length);
   } else
      remove_note(mail);
   stack = oldstack;
}


/* reply to an article */

void            reply_sarticle(player * p, char *str)
{
   note           *mail, *old;
   char           *indent, *body, *oldstack;
   int             length;

   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: reply <no>\n");
      return;
   }
   if (posted_count(p) >= p->max_mail)
   {
      tell_player(p, " Sorry, you have reached your mail limit.\n");
      return;
   }
   if (!p->saved)
   {
      tell_player(p, " Eeek, no saved bits.\n");
      return;
   }
   old = find_snews_article(atoi(str));
   if (!old)
   {
      vtell_player(p, " Can't find article '%s'\n", str);
      return;
   }
   mail = create_note();
   if(!mail) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   strcpy(mail->name, p->name);

   if (strstr(old->header, "Re: ") == old->header)
      strcpy(mail->header, old->header);
   else
   {
      sprintf(stack, "Re: %s", old->header);
      strncpy(mail->header, stack, MAX_TITLE - 1);
   }

   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;

   sprintf(stack, "In your article '%s' you wrote ...\n", old->header);
   stack = strchr(stack, 0);

   while (*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while (*indent && *indent != '\n')
    *stack++ = *indent++;
      *stack++ = '\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;

   tell_player(p, " Please trim letter as much as possible...\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, body);
   if (p->edit_info)
   {
      p->edit_info->misc = (void *) mail;
      length = strlen(old->name) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, old->name, length);
   } else
      remove_note(mail);
   stack = oldstack;
}


/* unlink and remove a mail note */

void            unlink_mail(note * m)
{
   char           *oldstack;
   saved_player   *sp;
   int            *change;
   note           *scan, *tmp;
   if (!m)
      return;
   oldstack = stack;
   strcpy(stack, m->name);
   lower_case(stack);
   stack = end_string(stack);
   sp = find_saved_player(oldstack);
   if (!sp)
   {
      if (current_player)
      {
         vlog("mail_error", "(2) mail: %s - current: %s", m->name,
            current_player->name);
      } else
      {
         vlog("mail_error", "(2) mail, %s - no current", m->name);
      }
      unlink_mail(find_note(m->next_item));
   } else
   {
      change = &(sp->mail_sent);
      scan = find_note(sp->mail_sent);
      while (scan && scan != m)
      {
    change = &(scan->next_item);
    scan = find_note(scan->next_item);
      }
      tmp = find_note(m->next_item);
      if (tmp)
    *change = tmp->id;
      else
    *change = 0;
   }
   remove_note(m);
   stack = oldstack;
}


/* remove sent mail */

int		delete_sent_fn(player *p, int number)
{
   note *scan;
   if(!p || !p->saved) {
     log("mail_error", "delete_sent_fn called with no player or no saved data");
     return 0;
   }
   if(number<1) {
     return 0;
   }
 
   scan = find_note(p->saved->mail_sent);
   if(!scan) {
     return 0;
   }
   for (number--; number; number--)
      if (scan)
        scan = find_note(scan->next_item);
      else
        break;
   if (!scan) {
     return 0;
   }
   unlink_mail(scan);
   return 1;
}


/* remove received mail - actual fn given number, basically the old delete
   received function, minus the player interface type code, which is handled
   now by the hideous pile of spam residing below.  There -must- be a better
   way of doing it than this, but at least using the below method we are
   using a proven delete routine, and the player interface copes quite
   happily with repetition - ie mail delete 2,2,2,2,2,2,1,3 will delete
   3, 2, 1 - no repeated attempts ;) */
int            delete_received_fn(player * p, int number)
{
   note           *deleted;
   recmail        *scan;

   if(!p || !p->saved) {
     log("mail_error", "delete_received_fn given null saved player");
     return 0;
   }
   /* this should be correct.  I am paranoid, however. */
   if(number<1) {
     return 0;
   }
   scan = p->saved->received_list;
   
   for(number--;number;number--) {
     if(scan)
       scan=scan->next;
     else
       break;
   }

   if (!scan) { /* not enough mail */
      return 0;
   }

   deleted = find_note(scan->mail_received);
   destroy_recmail(p->saved, scan); /* kill the entity */
   if (deleted)
   {
      deleted->read_count--;
      if (!(deleted->read_count))
         unlink_mail(deleted);
   }
   return 1;
}


int	       mrp_check_number(player *p, int checkme, int sent)
{
  if(!p)
    return 0;
  if(checkme<=0) {
    if(sent)	tell_player(p, " Format: remove <numbers/ranges> (see help ranges)\n");
    else	tell_player(p, " Format: delete <numbers/ranges> (see help ranges)\n");
    return 0;
  }
  if(checkme>100) {
    if(sent)	tell_player(p, " I doubt you have sent more than 100 mails.\n");
    else	tell_player(p, " I doubt you have received more than 100 mails.\n");
    return 0;
  }
  return 1;
}


/* player interface to delete mail - i fucking hate parsing user input */
void	       mail_range_process(player *p, char *str, int sent)
{
   char convbuffer[IBUFFER_LENGTH+1]; /* lets see someone crash THAT ;) */
   char *nextarg, *startarg;
   int kill_list[100], count, scan;
   int	startnum, endnum;
    
   if(!p || !*str) {
     log("mail_error", "mail_range_process passed null player or string");
     return;
   }
   /* initialise array */
   for(count=0;count<100;count++)
     kill_list[count]=0;
     
   /* parse the input, dumping valid mails to delete into the array, this cant
      possibly work, surely? 8-) */
   nextarg = str;
   while(*nextarg) { /* do while there are arguments to read */
     /* set start of argument */
     startarg = nextarg;
     while(*nextarg && *nextarg!=',')
       nextarg++;
     if(*nextarg && *nextarg==',')
       *nextarg++=0; /* terminate */
       
     /* we have an argument, attempt to get the first number, this should be
        easy */
     scan = 0;
     while(scan<IBUFFER_LENGTH && *startarg && isdigit(*startarg)) {
       convbuffer[scan] = *startarg++;
       scan++;
     }
     convbuffer[scan]=0;
     startnum = atoi(convbuffer);
     if(!mrp_check_number(p, startnum, sent)) {
       return;
     }
     /* did we hit a dash? (ie a range) */
     if(*startarg && *startarg=='-') {
       startarg++; /* take it past the - */
       scan = 0;
       while(*startarg && isdigit(*startarg)) {
         convbuffer[scan] = *startarg++;
         scan++;
       }
       convbuffer[scan] = 0;
       endnum = atoi(convbuffer);
       if(!mrp_check_number(p, endnum, sent)) {
         return;
       }
       /* check to see if end<start (duuh) */
       if(endnum<startnum) {
         vtell_player(p, " Invalid range (%d-%d).\n", startnum, endnum);
         return;
       }
     }
     else /* dont want to loop, so to save on code, set end=start */
       endnum = startnum;
     
     /* dump to array */
     for(;startnum<=endnum; startnum++) 
       kill_list[startnum-1]=1; /* toggle the deletion flag */
   }
   
   /* right - we now have an array, 1 meaning delete this mail.  due to
      the nature of the (copied) delete fn, if we do things backwards, then
      they will work correctly ;) - alsi it means the loop will stop without
      damaging anything should a mail not exist */
   for(count=99;count>=0;count--) /* go back */
     if(kill_list[count]){
       if(sent) {
         if(!delete_sent_fn(p, count+1)) {
           tell_player(p, " You haven't sent that many mails.\n");
           return;
         }
       }
       else {
         if(!delete_received_fn(p, count+1)) {
           tell_player(p, " You haven't received that many mails.\n");
           return;
         }
       }
     }
   if(sent) tell_player(p, " Mail removed ...\n");
   else     tell_player(p, " Mail deleted ...\n");
}


void            delete_sent(player * p, char *str)
{
   if (!*str)
   {
      tell_player(p, " Format: remove <numbers/ranges> (see help ranges)\n");
      return;
   }
   if (!mail_saved_check(p))
      return;
   mail_range_process(p, str, 1);
}


void	delete_received(player *p, char *str)
{
   /* initial format check */
   if (!*str)
   {
      tell_player(p, " Format: delete <numbers/ranges> (see help ranges)\n");
      return;
   }
   /* lack of save info */
   if (!mail_saved_check(p))
      return;
   /* lack of received mail */
   if (!(p->saved->received_list))
   {
      tell_player(p, " You have recieved no mail to delete.\n");
      return;
   }
   mail_range_process(p, str, 0);
}



/* admin ability to view notes */

void            view_note(player * p, char *str)
{
   note           *n;
   char           *oldstack;
   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: view_note <number>\n");
      return;
   }
   n = find_note(atoi(str));
   if (!n)
   {
      tell_player(p, " Can't find note with that number.\n");
      return;
   }
   if (n->flags & NEWS_ARTICLE)
      strcpy(stack, " News Article.\n");
   else if (n->flags & SNEWS_ARTICLE)
      strcpy(stack, " SuNews Article.\n");
   else
      strcpy(stack, " Mail (?).\n");
   stack = strchr(stack, 0);
   sprintf(stack, " Posted on %s, by %s.\n Read count: %d\n",
      convert_time(n->date), n->name, n->read_count);
   stack = strchr(stack, 0);
   if (n->flags & ANONYMOUS)
   {
      strcpy(stack, " Posted anonymously.\n");
      stack = strchr(stack, 0);
   }
   if (n->flags & NOT_READY)
   {
      strcpy(stack, " Not ready flag set.\n");
      stack = strchr(stack, 0);
   }
   if (n->flags & SUPRESS_NAME)
   {
      strcpy(stack, " Name suppressed.\n");
      stack = strchr(stack, 0);
   }
   sprintf(stack, " Next link -> %d\n", n->next_item);
   stack = end_string(stack);
   pager(p, oldstack, 0);
   stack = oldstack;
}



/* remove a note from */

void            dest_note(player * p, char *str)
{
   note           *n;

   if (!*str)
   {
      tell_player(p, " Format: rm_note <number>\n");
      return;
   }
   n = find_note(atoi(str));
   if (!n)
   {
      tell_player(p, " Can't find note with that number.\n");
      return;
   }
   remove_any_note(n);
   tell_player(p, " Note removed\n");
}



/* relink emergancy command */

void            relink_note(player * p, char *str)
{
   char           *to;
   int             id1, id2;
   note           *n;
   to = next_space(str);
   *to++ = 0;
   id1 = atoi(str);
   id2 = atoi(to);
   /*
    * if (!id1 || !id2) { tell_player(p,"Format : relink <number>
    * <number>\n"); return; }
    */
   n = find_note(id1);
   if (!n)
   {
      tell_player(p, " Can't find first note\n");
      return;
   }
   n->next_item = id2;
   tell_player(p, " next_item pointer twiddled.\n");
   return;
}


/* recount emergancy command */

void            recount_news(player * p, char *str)
{
   int             article;
   note           *scan;
   news_count = 0;
   scan = find_note(news_start);
   if (scan)
   {
      article = scan->next_item;
      for (; scan; article = scan->next_item, news_count++)
      {
    scan = find_note(article);
    if (!scan)
       break;
      }
      news_count++;
   }
   snews_count = 0;
   scan = find_note(snews_start);
   if(scan)
   {
      article = scan->next_item;
      for (; scan; article = scan->next_item, snews_count++)
      {
         scan = find_note(article);
         if (!scan) break;
      }
      snews_count++;
   }
   tell_player(p, " Tis Done ...\n");
}


/* also nicked from surfers cos i wrote it - tidies up article
   headers that get spammed with re's */
char *tidy_article_title(char *str)
{
   char	*end, *start, *oldstack;
   int  reflag = 0;
   
   oldstack = stack;
   start = str;
   end = strchr(str, 0);
   if (!*str || (strlen(str)==0) || (start>=end) || (str==""))
   {
      sprintf(stack, "<Untitled>");
      return stack;
   }
   
   /* loop until either: *str stops pointing at re: or 're ' or ' '
      or until str >= end */
   do
   {
      if (!strncasecmp(str, "re:", 3) || !strncasecmp(str, "re ", 3))
      {
        reflag = 1;
        str+=3;
      }
      else if (*str == ' ')
        str++;
   }
   while ((!strncasecmp(str, "re:", 3) || !strncasecmp(str, "re ", 3) ||
          (*str == ' ')) && (str <= end));
   if ((str >= end) || !strcasecmp(str, "re"))
      sprintf(stack, "<Untitled>");
   else
   {
      if (reflag == 1)
         sprintf(stack, "Re: %s", str);
      else
         sprintf(stack, "%s", str);
   }
   return stack;
}


/* toggle whether someone gets informed of mail */

void            toggle_mail_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~MAIL_INFORM;
   else if (!strcasecmp("on", str))
      p->saved_flags |= MAIL_INFORM;
   else
      p->saved_flags ^= MAIL_INFORM;

   if (p->saved_flags & MAIL_INFORM)
      tell_player(p, " You will be informed when you receive mail.\n");
   else
      tell_player(p, " You will not be informed when you receive mail.\n");
}


/* toggle whether someone gets informed of news */
void            toggle_news_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~NEWS_INFORM;
   else if (!strcasecmp("on", str))
      p->saved_flags |= NEWS_INFORM;
   else
      p->saved_flags ^= NEWS_INFORM;

   if (p->saved_flags & NEWS_INFORM)
      tell_player(p, " You will be informed of new news.\n");
   else
      tell_player(p, " You will not be informed of new news.\n");
}


/* toggle whether someone gets informed of news */
void            toggle_snews_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~SNEWS_INFORM;
   else if (!strcasecmp("on", str))
      p->saved_flags |= SNEWS_INFORM;
   else
      p->saved_flags ^= SNEWS_INFORM;

   if (p->saved_flags & SNEWS_INFORM)
      tell_player(p, " You will be informed of new snews.\n");
   else
      tell_player(p, " You will not be informed of new snews.\n");
}


/* toggle whether someone can receive anonymous mail */
void            toggle_anonymous(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->saved_flags &= ~NO_ANONYMOUS;
   else if (!strcasecmp("on", str))
      p->saved_flags |= NO_ANONYMOUS;
   else
      p->saved_flags ^= NO_ANONYMOUS;

   if (p->saved_flags & NO_ANONYMOUS)
      tell_player(p, " You will not be able to receive anonymous mail.\n");
   else
      tell_player(p, " You will be able to receive anonymous mail.\n");
}


int	new_mail_check(player *p)
{
  recmail *scan;
  int count = 0;
  if(!p || !p->saved)
    return 0;
  for(scan=p->saved->received_list; scan; scan=scan->next)
    if(scan->flags & reNEW)
      count++;
  return count;
}


/* follow up an article to su news system */
void	followuptosunews(player *p, char *str)
{
   char		*oldstack, *body, *indent;
   note		*article, *old;
   oldstack = stack;
   
   old = find_news_article(atoi(str));
   if(!old)
   {
      vtell_player(p, " No such news article '%s'\n", str);
      return;
   }
   article = create_note();
   if(!article) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }
   if(strstr(old->header, "Re: ") == old->header)
      strcpy(article->header, old->header);
   else
   {
      sprintf(stack, "Re [News]: %s", old->header);
      strncpy(article->header, stack, MAX_TITLE - 2);
   }
   article->flags |= SNEWS_ARTICLE;
   strcpy(article->name, p->name);
   indent = stack;
   get_string(stack, old->text.where);
   stack = end_string(stack);
   body = stack;
   
   sprintf(stack, "On %s, %s wrote in the main news...\n", convert_time(old->date), old->name);
   stack = strchr(stack, 0);
   
   while(*indent)
   {
      *stack++ = '>';
      *stack++ = ' ';
      while(*indent && *indent != '\n')
         *stack++ = *indent++;
      *stack++='\n';
      indent++;
   }
   *stack++ = '\n';
   *stack++ = 0;
   
   tell_player(p, " Please trim article as much as is possible ....\n");
   start_edit(p, MAX_ARTICLE_SIZE, end_spost, quit_spost, body);
   if(p->edit_info)
     p->edit_info->misc = (void *) article;
   else
     remove_note(article);
   stack = oldstack;
}


/* time to forward mail to people - by sparky */
void forward_letter(player *p,char *str)
 {
   note *mail,*old;
   char *indent,*body,*oldstack,*name;
   int length, toflag = 0;

   oldstack=stack;

   name=next_space(str);
   *name++=0;
   if (!*str || !*name) {
     tell_player(p," Format: forward <number> <name>\n");
     return;
   }
   if (posted_count(p)>=p->max_mail) {
     tell_player(p," Sorry, you have reached your mail limit.\n");
     return;
   }
  if (!p->saved) {
     tell_player(p," Eeek, no saved bits.\n");
     return;
   }
   old=find_received(p->saved,atoi(str), 3);
   if (!old) {
     sprintf(stack," Can't find letter '%s'\n",str);
     stack=end_string(stack);
     tell_player(p,oldstack);
     stack=oldstack;
     return;
   }
   mail=create_note();
   if(!mail) {
     tell_player(p, " Sorry, notes system full!\n");
     return;
   }

   if(!strcmp(p->command_used->text, "aforward"))
      mail->flags |= ANONYMOUS;

   if (old->flags&ANONYMOUS) mail->flags |= SUPRESS_NAME;
   strcpy(mail->name,p->name);

   if (strstr(old->header,"Fd: ")==old->header)
     strcpy(mail->header,old->header);
   else {
     sprintf(stack,"Fd: %s",old->header);
     strncpy(mail->header,stack,MAX_TITLE-1);
   }

   indent=stack;
   get_string(stack,old->text.where);
   stack=end_string(stack);
   body=stack;

   sprintf(stack,"Forwarded from %s.\n",old->name);
   stack=strchr(stack,0);

   while(*indent) 
   {
     *stack++='>';
     *stack++=' ';

     if (toflag==0 && mail->flags & ANONYMOUS && !strncmp(indent, "To: ", 4))
     {
       while(*indent && *indent != '\n')
         indent++;
       toflag=1;
       strcpy(stack, "To: Anonymous sender");
       stack = strchr(stack, 0);
     }
     else
     {
       while(*indent && *indent!='\n') 
         *stack++=*indent++;
     }
     *stack++='\n';
     indent++;
   }
   *stack++='\n';
   *stack++=0;

   start_edit(p,MAX_ARTICLE_SIZE,end_mail,quit_mail,body);
   if (p->edit_info) {
     p->edit_info->misc=(void *)mail;
     length=strlen(name)+1;
     mail->text.where=(char *)MALLOC(length);
     memcpy(mail->text.where,name,length);
   }
   else
       remove_note(mail);
   stack=oldstack;
}


/* This is just INSANE! 
   A thing to let spoon newbie admin mail all their users from within the
   talker.  I pity the people who misuse this. */
void            quit_resmail(player * p)
{
   tell_player(p, " Letter NOT posted.\n");
   finish_edit(p);
}

void		end_resmail(player *p)
{
   int fd=-1;
   /* this happens after the mail is sent methinks */
   if (p->edit_info) 
   {
     /* try to send it out - first try a write to disk! */
#if defined( FREEBSD )
     fd = open("files/stuff/resmail", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
     fd = open("files/stuff/resmail", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
     if(fd<0) 
       tell_player(p, " Unable to open file for resmail.\n");
     else {
       tell_player(p, " All residents mailed.  I hope you really meant to do that ;)\n");
       write(fd, p->edit_info->buffer, strlen(p->edit_info->buffer));
       /* append the signature file */
       write(fd, sig_file.where, strlen(sig_file.where));
       close(fd);
       /* we have the files out to disk now apparently */
       system("bin/auto_mailer_script files/stuff/emails.file files/stuff/resmail files/stuff/temp_subj");
     }
   } 
   finish_edit(p);
}


void            resmail(player * p, char *str)
{
   int             fd=-1;

   if (!*str)
   {
      tell_player(p, " Format: resmail <subject>\n");
      return;
   }

   /* write out subject tempfile */
#if defined( FREEBSD )
   fd = open("files/stuff/temp_subj", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open("files/stuff/temp_subj", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
     tell_player(p, " Couldn't open temp file for subject!  Aborting.\n");
     return;
   }
   write(fd, str, strlen(str));
   close(fd);
   
   if(!dump_out_emails_fn(p, 2, 0)) {
     tell_player(p, " Failed to dump out emails!  Aborted.\n");
     return;
   }
   
   tell_player(p, " Enter main text of the letter...\n");
   *stack = 0;
   start_edit(p, MAX_ARTICLE_SIZE, end_resmail, quit_resmail, stack);
   if(!p->edit_info)
     return;
}
