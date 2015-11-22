/*
 * ip.c - holds all the site related functions
 */
 
#include "include/config.h" 
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#ifndef FREEBSD228
#include <malloc.h>
#endif
#include <stdlib.h> 
#include <string.h>
#include <memory.h>
#include <time.h>
#include <dirent.h>

#ifdef SUNOS
#define TIME_DEFINES
#endif

#include "include/missing_headers.h"
#include "include/proto.h"



/* interns */

int	ip1=0,ip2=0,ip3=0,ip4=0;

/* SUPPORT ROUTINES */


/* another support routine: check ip string */

int check_ip_string(char *str)
{
   char *ref;
   
   for (ref = str; *ref; ref++)
      if(!isdigit(*ref) && *ref!='*' && *ref!='.')
         return 0;
         
   return 1;
}


/* routine to get the next arg from .'s not spaces */

char           *next_dot(char *str)
{
    while (*str && *str != '.')
	str++;
    if (*str == '.')
    {
	while (*str == '.')
	    str++;
	str--;
    }
    return str;
}


/* take a string and convert it into the 4 interns and return the site type :-) */
/* also do some fixing of the number if required to stop silly admin fucking things up */

int	string_to_ips(char *str)
{
   char *n1,*n2,*n3,*n4,*end;
   
   n1 = str;
   n2 = next_dot(n1);
   if (*n2) *n2++ = 0;
   n3 = next_dot(n2);
   if (*n3) *n3++ = 0;
   n4 = next_dot(n3);
   if (*n4) *n4++ = 0;
   end = n4;
   while (*end && *end!= ' ')
      end++;
   *end = 0;
   
   /* check to see they exist */
   if (!*n1) return 0;

   /* convert into numbers next */
   if (*n1=='*') ip1=255;
   else ip1=atoi(n1);
   if (!*n2 || *n2=='*') ip2=255;
   else ip2=atoi(n2);
   if (!*n3 || *n3=='*') ip3=255;
   else ip3=atoi(n3);
   if (!*n4 || *n4=='*') ip4=255;
   else ip4=atoi(n4);
   
   if (ip1<0 || ip2<0 || ip3<0 || ip4<0 || ip1>255 || ip2>255 || ip3>255 || ip4>255)
   {
      ip1=0;ip2=0;ip3=0;ip4=0;
      return 0;
   }
   /* 0-invalid, 1-class a, 2-class b, 3-class c, 4-localhost, 5-reserved, 6-all sites */
   if (ip1<1)
   {
      ip1 = 0; ip2 = 0; ip3 = 0; ip4 = 0;
      return 0;
   }
   if (ip1>0 && ip1<127)
   {
      return 1;
   }
   if (ip1==127)
   {
      ip2 = 255; ip3 = 255; ip4 = 255;
      return 4;
   }
   if (ip1>127 && ip1<192)
   {
      return 2;
   }
   if (ip1>191 && ip1<224)
   {
      return 3;
   }
   if (ip1>223 && ip1<255)
   {
      return 5;
   }
   if (ip1==255)
   {
      ip2=255;ip3=255;ip4=255;
      return 6;
   }
   ip1=0;ip2=0;ip3=0;ip4=0;
   return 0;
}

/* version with extra checks for site bans */

int	string_to_ips_ban(char *str)
{
   char *n1,*n2,*n3,*n4,*end;
   
   n1 = str;
   n2 = next_dot(n1);
   if (*n2) *n2++ = 0;
   n3 = next_dot(n2);
   if (*n3) *n3++ = 0;
   n4 = next_dot(n3);
   if (*n4) *n4++ = 0;
   end = n4;
   while (*end && *end!= ' ')
      end++;
   *end = 0;
   
   /* check to see they exist */
   if (!*n1 || !*n2 || !*n3 || !*n4) return 0;
   
   /* convert into numbers next */
   if (*n1=='*') ip1=255;
   else ip1=atoi(n1);
   if (*n2=='*') ip2=255;
   else ip2=atoi(n2);
   if (*n3=='*') ip3=255;
   else ip3=atoi(n3);
   if (*n4=='*') ip4=255;
   else ip4=atoi(n4);
   
   if (ip1<0 || ip2<0 || ip3<0 || ip4<0 || ip1>255 || ip2>255 || ip3>255 || ip4>255)
   {
      ip1=0;ip2=0;ip3=0;ip4=0;
      return 0;
   }
   /* 0-invalid, 1-class a, 2-class b, 3-class c, 4-localhost, 5-reserved, 6-all sites */
   if (ip1<1)
   {
      ip1 = 0; ip2 = 0; ip3 = 0; ip4 = 0;
      return 0;
   }
   if (ip1>0 && ip1<127)
   {
      return 1;
   }
   if (ip1==127)
   {
      ip2 = 255; ip3 = 255; ip4 = 255;
      return 4;
   }
   if (ip1>127 && ip1<192)
   {
      if (ip2 == 255) return 0;
      return 2;
   }
   if (ip1>191 && ip1<224)
   {
      if (ip2==255 || ip3==255) return 0;
      return 3;
   }
   if (ip1>223 && ip1<255)
   {
      if (ip2==255 || ip3==255 || ip4==255) return 0;
      return 5;
   }
   if (ip1==255)
   {
      ip2=255;ip3=255;ip4=255;
      return 6;
   }
   ip1=0;ip2=0;ip3=0;ip4=0;
   return 0;
}


/* string to convert the ips into a string and return a pointer to it */

char *ips_to_string(void)
{
   char address[MAX_INET_ADDR], *ref;
   
   memset(address, 0, MAX_INET_ADDR);
   if (ip1==255)
      return "*.*.*.*";
   sprintf(address, "%d.", ip1);
   ref = strchr(address, 0);
   if (ip2==255)
      strcpy(ref, "*.");
   else
      sprintf(ref, "%d.", ip2);
   ref = strchr(ref, 0);
   if (ip3==255)
      strcpy(ref, "*.");
   else
      sprintf(ref, "%d.", ip3);
   ref = strchr(ref, 0);
   if (ip4==255)
      strcpy(ref, "*");
   else
      sprintf(ref, "%d", ip4);
   ref = end_string(ref);
   ref = address;
   return ref;
}

/* return the subnet class of a site */
/* returns 0 if no match, 1=a, 2=b, 3=c, 4=localhost, 5=reserved */

int return_subnet_class(char *str)
{
   int no1,no2,no3,no4;
   char address[MAX_INET_ADDR];
   memset(address, 0, MAX_INET_ADDR);
   if (!*str)
      return 0;
   strcpy(address, str);
   sscanf(address, "%d.%d.%d.%d", &no1,&no2,&no3,&no4);
   if (*str=='*')
      return 6;
   if (no1<1)
      return 0;
   if (no1>0 && no1<127)
      return 1;
   if (no1==127)
      return 4;
   if (no1>127 && no1<192)
      return 2;
   if (no1>191 && no1<224)
      return 3;
   if (no1>223 && no1<255)
      return 5;
   return 0;
}


/* STUFF TO DO WITH SITE BANS */


/* read the banish file in from disk (*/

void		read_banish_file_from_disk(void)
{
   char *oldstack, *ref;
   int fd;

   oldstack = stack;

   free_files();
   
   banish_file = load_file("files/stuff/banish");
   ref = banish_file.where;
   stack = oldstack;
   while (*ref)
   {
      /* take it to the site info */
      while(*ref!='@' && *ref && *ref!='\n')
         ref++;
      ref++;
      /* ok we got to the @, start copying until after the type */
      while(*ref!=' ')
         *stack++ = *ref++;
      /* now we hit the first space, do the type */
      *stack++ = *ref++;
      *stack++ = *ref++; /* add the type */
      *stack++ = '\n';   /* add the newline */
      /* take ref to the end of the line */
      while(*ref!='\n')
         ref++;
      ref++; /* to the next line */
   }
   *stack++ = 0;
#if defined( FREEBSD )
   fd = open("files/stuff/banish_scan", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open("files/stuff/banish_scan", O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
     stack = oldstack;
     return;
   }
   write(fd, oldstack, strlen(oldstack));
   close(fd);/* end the stack data */
   stack = oldstack;
   
   load_files(0);
}


/* write the banish file out to disk */

void		write_banish_file_to_disk(void)
{
    int             fd;
#if defined( FREEBSD )
    fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
    fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
    if(fd<0) 
      return;
    write(fd, banish_file.where, banish_file.length);
    close(fd);
}



/* work out whether a player is site banished or not */
/* line is the bit of the banish file, char is the input */
int             match_banish(char *str, char *line)
{
    char           *addr;
    
    for (addr = str; *addr; addr++, line++)
	if (*line == '*')
	{
	    while (isdigit(*addr))
		addr++;
	    line++;
	} 
	else if (*addr != *line)
	    return 0;
    /* step through till end of line */
    while(*line && *line!='N' && *line!='C' && *line!='L') line++;
    if (*line=='N') return 1;
    if (*line=='L') return 3;
    return 2;
}

/* version to do approve file */
int             match_approve(char *str, char *line)
{
    char           *addr;
    
    for (addr = str; *addr; addr++, line++)
	if (*line == '*')
	{
	    while (isdigit(*addr))
		addr++;
	    line++;
	} 
	else if (*addr != *line)
	    return 0;
    return 2;
}

/* version of the above to allow double wildcards */

int             match_banish_wild(char *str, char *line)
{
    char           *addr;
    
    for (addr=str; *addr; addr++, line++)
    {
       if (*addr=='*')
       {
          while(isdigit(*line) || *line=='*') line++;
          addr++;
       }
       else if (*line=='*')
       {
          while(isdigit(*addr) || *addr=='*') addr++;
          line++;
       }
       else if (*line!=*addr)
          return 0;
    }
    
    /* step through till end of line */
    while(*line && *line!='N' && *line!='C' && *line!='L') line++;
    if (*line=='N') return 1;
    if (*line=='L') return 3;
    return 2;
}

/* version of the above to not check type of ban */
int             match_site(player *p, char *line)
{
    char           *addr;
    
    for (addr = p->num_addr; *addr; addr++, line++)
	if (*line == '*')
	{
	    while (isdigit(*addr))
		addr++;
	    line++;
	} else if (*addr != *line)
	    return 0;
    return 1;
}


int             do_banish(player * p)
{
    char           *scan;
    int             i, t;

    scan = banish_scan.where;
    for (i = banish_scan.length; i;) 
    {
        t = match_banish(p->num_addr, scan);
        if(t==2) return 1;
        if(t==1) p->flags |= CLOSED_TO_NEWBIES;
        if(t==3) p->flags |= SITE_LOG;

	while (i && *scan != '\n') {
	    scan++;
	    i--;
	}
	if (i) 
	{
	    scan++;
	    i--;
	}
    }
    return 0;
}


/* view the banish file */

void		banish_show(player * p, char *str)
{
   char *oldstack;
   
   oldstack = stack;
   if (*str=='-')
      pager(p, banish_scan.where, 0);
   else
      pager(p, banish_file.where, 0);
   stack = oldstack;
}


/* support routine for sitecheck command */

void site_to_stack(char *text, int type)
{
   while (isdigit(*text)) 
      *stack++ = *text++;
   *stack++ = '.';
   text++;
   switch(type)
   {
	   case 1:
	        strcpy(stack, "*.*.*");
	        stack = strchr(stack, 0);
	        break;
	   case 2:
	   	while (isdigit(*text))
	   	   *stack++ = *text++;
	   	strcpy(stack, ".*.*");
	   	stack = strchr(stack, 0);
	   	break;
	   case 3:
	   	while (isdigit(*text))
	   	   *stack++ = *text++;
	   	*stack++ = '.';
	   	text++;
	   	while (isdigit(*text))
	   	   *stack++ = *text++;
	   	*stack++ = '.';
	   	*stack++ = '*';
	   	break;
	   default:
	        while (isdigit(*text))
	           *stack++ = *text++;
	        *stack++ = '.';
	        text++;
	        while (isdigit(*text))
	           *stack++ = *text++;
	        *stack++ = '.';
	        text++;
	        while (isdigit(*text))
	           *stack++ = *text++;
   }
   *stack++ = 0;
}


/* unsplat a site *wibble* */

void unsplat(player * p, char *str)
{
    time_t t;
    int number = -1;
    
    if (p->flags & BLOCK_SU)
    {
	tell_player (p," You need to be on_duty to do that.\n");
	return;
    }
    
    t = time(0);
    if (str)
	    number = atoi(str);
	else
	    number = 0;
    if (!*str || number<0)
    {
	number = splat_timeout - (int) t;
	if (number <= 0)
	{
		tell_player(p, " Format: unsplat <new time> <site>\n");
	    return;
	}
	if (splat2 == 255)
		vtell_player(p, " Site %d.*.*.* is splatted for %d more seconds.\n",
		splat1, number);
	else if (splat3 ==255)
		vtell_player(p, " Site %d.%d.*.* is splatted for %d more seconds.\n",
		splat1, splat2, number);
	else if (splat4 ==255)
		vtell_player(p, " Site %d.%d.%d.* is splatted for %d more seconds.\n",
		splat1, splat2, splat3, number);
	else
		vtell_player(p, " Site %d.%d.%d.%d is splatted for %d more seconds.\n",
		splat1, splat2, splat3, splat4, number);
	return;
    }
    if (splat1 == 255)
    {
	tell_player(p, " No site splatted atm.\n");
	return;
    }
    if (number == 0)
    {
        if (splat2==255)
        vsu_wall("-=> %s unsplat%s site %d.*.*.*\n", p->name, single_s(p), splat1);
        else if (splat3==255)
	vsu_wall("-=> %s unsplat%s site %d.%d.*.*\n", p->name, single_s(p), splat1, splat2);
	else if (splat4==255)
	vsu_wall("-=> %s unsplat%s site %d.%d.%d.*\n", p->name, single_s(p), splat1, splat2, splat3);
	else
	vsu_wall("-=> %s unsplat%s site %d.%d.%d.%d\n", p->name, single_s(p), splat1, splat2,splat3,splat4);
	splat_timeout = (int) t;
	return;
    }
    if (number > 600)
    {
	tell_player(p, " That's not a very nice time,"
		    " resetting to 10 minutes...\n");
	number = 600;
    }
    vsu_wall("-=> %s change%s the splat time on site %d.%d.*.*"
	    " to a further %d seconds.\n",
	    p->name, single_s(p), splat1, splat2, number);
    splat_timeout = (int) t + number;
}


/* STUFF TO DO WITH CHECKING SOMEONES SITE */

/* command to check IP addresses */

void view_ip(player * p, char *str)
{
    player *scan;
    char *oldstack, middle[80];
    int page, pages, count;
    
    oldstack = stack;
    if (isalpha(*str))
    {
	scan = find_player_global(str);
	if (!scan)
	    return;
#ifdef IDENT
	vtell_player(p, "%s %s logged in from %s@%s.\n", scan->name, isare(scan), scan->userID, scan->inet_addr);
#else
        vtell_player(p, "%s %s logged in from %s.\n", scan->name, isare(scan), scan->inet_addr);
#endif
    }
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
	    if (scan->flags & SITE_LOG)
		*stack++ = '*';
	    else
		*stack++ = ' ';
#ifdef IDENT
            sprintf(stack, "%s %s logged in from %s@%s.\n", scan->name,
			isare(scan), scan->userID, scan->inet_addr);
#else
            sprintf(stack, "%s %s logged in from %s.\n", scan->name,
			isare(scan), scan->inet_addr);
#endif
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


/* list people who are from the same site */
void same_site(player * p, char *str)
{
    char *oldstack, *text, *ref=0;
    char address[MAX_INET_ADDR];
    player *p2;
    list_ent	*l;
    int type, site_type = 0, ctype = 0, count = 0;
    
    memset(address, 0, MAX_INET_ADDR);
    oldstack = stack;
    if (*str && *str=='@' && *(str+1))
    {
	str++; /* take away from @ */
	if (isdigit(*str) || *str=='*')
	{
           if (!check_ip_string(str))
           {
              tell_player(p, " Sites must be in the form: n.n.n.n (*'s acceptable)\n");
              stack = oldstack;
              return;
           }
           site_type = string_to_ips(str);
           if (site_type==0)
           {
              tell_player(p, " Sorry, invalid site to check (read help sites)\n");
              stack = oldstack;
              return;
           }
           stack = ips_to_string();
           ref = address;
           while(*stack)
              *ref++ = *stack++;
           stack = oldstack;   
        }
        else /* address in string form */
           ctype=1;
        if (ctype==0)
	   sprintf(stack,"People from %s.\n",address);
	else
	   sprintf(stack, "People from %s.\n",str);
	stack=strchr(stack,0);
	
	for (p2 = flatlist_start; p2; p2 = p2->flat_next)
	{
	    /* there used to be a p2->location check here too */
	    /* get enough of the partial address into 'address' */
	    if (ctype==1)
	    {
	       strcpy(address, p2->inet_addr); 
	       for(ref=address; (int)strlen(ref)>(int)strlen(str); ref++);
	    }
	    
	    if ((ctype==0 && match_site(p2, address)) || (ctype==1 && !strcasecmp(str, ref)))
	    {
	        count++;
#ifdef IDENT
	         if(p2->name[0]!=0)
		   sprintf(stack, "(%s) %s@%s : %s ", p2->num_addr, p2->userID, p2->inet_addr, p2->name);
		 else
		   sprintf(stack, "(%s) %s@%s : <incoming> ", p2->num_addr, p2->userID, p2->inet_addr);
#else
	         if(p2->name[0]!=0)
	           sprintf(stack, "(%s) %s : %s ", p2->num_addr, p2->inet_addr, p2->name);
	         else
	           sprintf(stack, "(%s) %s : <incoming> ", p2->num_addr, p2->inet_addr);
#endif
		stack = strchr(stack, 0);
		if (p2->residency == NON_RESIDENT)
		{
		    strcpy(stack, "non resident.\n");
		    stack = strchr(stack, 0);
		} 
		else if (p2->email[0])
		{
		    if (p2->email[0] == ' ' && p2->email[1]==0)
			strcpy(stack, "Email validated set.");
		    else
		    {
		        l = find_list_entry(p2, p->name);
			if ((p2->saved_flags & PRIVATE_EMAIL && p->residency & ADMIN) ||
			   !(p2->saved_flags & PRIVATE_EMAIL))
			{
			    sprintf(stack, "[%s]", p2->email);
			    stack = strchr(stack, 0);
			}
			if (p2->saved_flags & PRIVATE_EMAIL)
			{
			    strcpy(stack, " (private)");
			    stack = strchr(stack, 0);
			}
		    }
		    *stack++ = '\n';
		} 
		else
		{
		    strcpy(stack, "Email not set.\n");
		    stack = strchr(stack, 0);
		}
	    }
	}
	*stack++=0;
	if (count==0)
	{
	    stack = oldstack;
	    if (ctype==0)
	       sprintf(stack, "No people on from %s.\n", address);
	    else
	       sprintf(stack, "No people on from %s.\n", str);
	    stack = end_string(stack);
	    tell_player(p, oldstack);
	    stack = oldstack;
	    return;
	}
        if (p->saved_flags & NO_PAGER)
	    tell_player(p, oldstack);
	else
	    pager(p, oldstack, 0);
	stack=oldstack;
	return;
    }
    else if (*str && isalpha(*str))
    {
	if (!strcasecmp(str, "me"))
	{
	    p2 = p;
	} else
	{
	    p2 = find_player_global(str);
	}
	if (!p2)
	{
	    stack = oldstack;
	    return;
	}
	str = stack;
	type = return_subnet_class(p2->num_addr); 
	text = p2->num_addr;
	site_to_stack(text, type);
        text = stack;
        sprintf(stack, "People from .. %s\n", str);
        stack = strchr(stack, 0);
        for (p2 = flatlist_start; p2; p2 = p2->flat_next)
        {
	    if (match_site(p2, str))
	    {
#ifdef IDENT
	        if (p2->name[0]!=0)
		   sprintf(stack, "(%s) %s@%s : %s ", p2->num_addr, p2->userID, p2->inet_addr, p2->name);
		else
		   sprintf(stack, "(%s) %s@%s : <incoming> ", p2->num_addr, p2->userID, p2->inet_addr);
#else
	        if (p2->name[0]!=0)
		   sprintf(stack, "(%s) %s : %s ", p2->num_addr, p2->inet_addr, p2->name);
		else
		   sprintf(stack, "(%s) %s : <incoming> ", p2->num_addr, p2->inet_addr);
#endif
	        stack = strchr(stack, 0);
	        if (p2->residency == NON_RESIDENT)
	        { 
		    strcpy(stack, "non resident.\n");
		    stack = strchr(stack, 0);
	        } 
	        else if (p2->email[0])
	        {
		    if (p2->email[0] == ' ' && p2->email[1]==0)
		        strcpy(stack, "Email validated set.");
		    else
		    {
		        l = find_list_entry(p2, p->name);
		        if ((p2->saved_flags & PRIVATE_EMAIL && p->residency & ADMIN) ||
			   !(p2->saved_flags & PRIVATE_EMAIL))
		        {
			    sprintf(stack, "[%s]", p2->email);
			    stack = strchr(stack, 0);
		        }
		        if (p2->saved_flags & PRIVATE_EMAIL)
		        {
			    strcpy(stack, " (private)");
			    stack = strchr(stack, 0);
		        }
		    }
		    *stack++ = '\n';
	        } 
	        else
	        {
		    strcpy(stack, "Email not set.\n");
		    stack = strchr(stack, 0);
	        }
	    }
        }
        *stack++ = 0;
        tell_player(p, text);
        stack = oldstack;
    }
    else
    {
	tell_player(p, " Format: site @<inet_number> or site <person>\n");
	stack = oldstack;
	return;
    }
    stack = oldstack;
}


/* trace someone and check against email */
void trace(player * p, char *str)
{
    char *oldstack;
    player *p2, dummy;
    
    oldstack = stack;
    if (!*str)
    {
	tell_player(p, " Format: trace <person>\n");
	return;
    }
    p2 = find_player_absolute_quiet(str);
    if (!p2)
    {
	vtell_player(p, " \'%s\' not logged on, checking saved files...\n",
		str);
	strcpy(dummy.lower_name, str);
	lower_case(dummy.lower_name);
	dummy.fd = p->fd;
	if (!load_player(&dummy))
	{
	    tell_player(p, " Not found.\n");
	    return;
	}
	if (dummy.residency == BANISHD)
	{
	    tell_player(p, " That is a banished name.\n");
	    return;
	}
	if ( dummy.email[0] )
	{
	    if ( dummy.email[0] == ' ' && dummy.email[1]==0 ) {
	        strcpy(stack, " Email validated set.\n");
	        stack = strchr(stack, 0);
	    }
	    else if (p->residency & ADMIN)
	    {
		sprintf(stack, " %s [%s]\n", dummy.name, dummy.email);
		if (dummy.saved_flags & PRIVATE_EMAIL)
		{
		    while (*stack != '\n')
			stack++;
		    strcpy(stack, " (private)\n");
		}
		stack = strchr(stack, 0);
	    }
	}
	sprintf(stack, " %s last connected from %s\n   and disconnected at ",
		dummy.name, dummy.saved->last_host);
	stack = strchr(stack, 0);
	if (p->jetlag)
	    sprintf(stack, "%s\n", convert_time(dummy.saved->last_on
						+ (p->jetlag * 3600)));
	else
	    sprintf(stack, "%s\n", convert_time(dummy.saved->last_on));
	stack = end_string(stack);
	tell_player(p, oldstack);
	stack = oldstack;
	return;
    }
    
    if (p2->residency == NON_RESIDENT)
    {
	sprintf(stack, " %s is non resident.\n", p2->name);
	stack = strchr(stack, 0);
    }
    else if (p2->email[0])
    {
	if(p2->email[0]==' ' && p2->email[1]==0) {
	    strcpy(stack, " Email validated set.\n");
	    stack = strchr(stack, 0);
	} 
	else if (p->residency & ADMIN)
	{
	    sprintf(stack, " %s [%s]\n", p2->name, p2->email);
	    if (p2->saved_flags & PRIVATE_EMAIL)
	    {
		while (*stack != '\n')
		    stack++;
		strcpy(stack, " (private)\n");
	    }
	    stack = strchr(stack, 0);
	}
    } else
    {
	sprintf(stack, " %s has not set an email address.\n", p2->name);
	stack = strchr(stack, 0);
    }
#ifdef IDENT
     sprintf(stack," Connected from %s@%s (%s).\n", p2->userID, p2->inet_addr, p2->num_addr);
#else
     sprintf(stack," Connected from %s (%s).\n", p2->inet_addr, p2->num_addr);
#endif
    stack =end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
}


/* ok, time to have something to add a ban to the banish file
   lets remember to reset it afterwards though (reload for now) */
void add_ban(player *p, char *str)
{
   char *oldstack, *site, *type, *reason, *end=NULL;
   char address[MAX_INET_ADDR];

   int site_type, fail = 0, fd;
   
   if (p->flags & BLOCK_SU)
   {
      tell_player(p, " Try doing that on duty.\n");
      return;
   }
   oldstack = stack;
   /* break line down into arguments */
   site = str;
   type = next_space(str);
   if (*type) *type++ = 0;
   reason = next_space(type);
   if (*reason) *reason++ = 0;
   end = end_string(reason);
   
   /* validity checks */
   if (!*str || !*reason) fail = 1;
   if (((int)strlen(type) > 1) || (*type!='L' && *type!='N' && *type!='C')) fail = 1;
   /* check the site specified */   
   /* first break the site down into seperate bits */
   
   /* now report an invalid format type if necessary */
   if (fail==1)
   {
      tell_player(p, " Format: ban <n.n.n.n> <N/C/L> <reason>\n");
      return;
   }
   if (!check_ip_string(str))
        {
            tell_player(p, " Sites must be in the form: n.n.n.n (*'s acceptable)\n");
            return;
        }
   site_type = string_to_ips_ban(site);
   if (site_type==0)
   {
      tell_player(p, " Sorry, invalid site to ban (read help sites)\n");
      return;
   }
   if (site_type==6 && *type=='C')
   {
      tell_player(p, " You may not totally close all sites!\n");
      return;
   }
   stack = ips_to_string();
   end = address;
   memset(address, 0, MAX_INET_ADDR);
   while (*stack)
      *end++ = *stack++;
   stack = oldstack;
      
   /* time to actually add the site to the banish file */
#if defined( FREEBSD )
   fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
     stack = oldstack;
     return;
   }
   /* move the current banish file to the stack */
   strcpy(stack, banish_file.where);
   stack = strchr(stack,0);
   /* add the new data to the stack */
   sprintf(stack, "%s @%s %s %s %s\n", sys_time(), address, type, p->name, reason);
   stack = strchr(stack, 0);
   write(fd, oldstack, strlen(oldstack));
   close(fd);
   stack = oldstack;   
   
   /* tell the sus */
   if (*type=='L')
      sprintf(stack, "-=> %s place%s site logging on site %s with reason: %s.\n", p->name, single_s(p), address, reason);
   else
   {
      sprintf(stack, "-=> %s ban%s site %s ", p->name, single_s(p), address);
      stack = strchr(stack, 0);
      if (*type=='C')
         sprintf(stack, "to all users, with reason: %s.\n", reason);
      else
         sprintf(stack, "to newbies, with reason: %s.\n", reason);
   }
   stack = end_string(stack);   
   su_wall(oldstack);
   stack = oldstack;
   /* log it */
   vlog("ban", "%s %s banned %s with reason: %s", p->name, type, address, reason);
   /* now do a call to the reload banish file thing */
   read_banish_file_from_disk();
}


/* if we are going to ban people we may aswell be able to unban them */

void remove_ban(player *p, char *str)
{
   char *oldstack, *ref, *ref2, *reason;
   char address[MAX_INET_ADDR];
   int site_type = 0, success = 0, fd;
   
   if (p->flags & BLOCK_SU)
   {
      tell_player(p, " Try doing that on duty.\n");
      return;
   }
   oldstack = stack;
   /* break up command args */
   reason=next_space(str);
   if (*reason) *reason++=0;
   ref=end_string(reason);
   
   if (!*str || !*reason)
   {
      tell_player(p, " Format: unban <site> <reason>\n");
      return;
   }    
   if (!check_ip_string(str))
        {
            tell_player(p, " Sites must be in the form: n.n.n.n (*'s acceptable)\n");
            return;
        }
   site_type = string_to_ips_ban(str);
   if (site_type==0)
   {
      tell_player(p, " Sorry, invalid site to unban (read help sites)\n");
      return;
   }
   stack = ips_to_string();
   ref = address;
   memset(address, 0, MAX_INET_ADDR);
   while (*stack)
      *ref++ = *stack++;
   stack = oldstack;
   
   /* scan thru files */
   ref = banish_file.where;
   while (*ref)
   {
      /* remember beginning of line */
      ref2 = ref;
      /* move to the @ */
      while (*ref!='@') ref++;
      ref++; /* move past the @ */
      
      if (!strncmp(address, ref, strlen(address)))
      {
         /* ok, omit the line */
         success = 1;
         /* move to the \n */
         while (*ref && *ref!='\n') ref++;
         ref++;
      }
      else
      {
         /* copy the line down cos its not a match */
         while(*ref2 && *ref2!='\n') 
            *stack++ = *ref2++;
         *stack++ = '\n';
         ref2++;
         ref = ref2;
      }
   }
   *stack++ = 0;
   if (success==0)
   {
      tell_player(p, " Sorry, no match.\n");
      stack = oldstack;
      return;
   }
   /* write this to disk now */
#if defined( FREEBSD )
   fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
   fd = open("files/stuff/banish", O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#endif
   if(fd<0) {
     stack = oldstack;
     return;
   }
   write(fd, oldstack, strlen(oldstack));
   close(fd);
   stack = oldstack;   
   /* tell the su channel */
   vsu_wall("-=> %s unban%s site %s.\n", p->name, single_s(p), address);
   /* log it */
   vlog("unban", " %s unbans %s with reason: %s", p->name, address, reason);
   read_banish_file_from_disk();   
}


/* -new- sitecheck command */

void sitecheck(player *p, char *str)
{
   char *oldstack, *ref, *ref2, *addref;
   char address[MAX_INET_ADDR];
   int site_type = 0, count = 0, t = 0, number;
   
   /* check for duff arguments */
   if (!*str)
   {
      tell_player(p, " Format: sitecheck <ip number>\n");
      return;
   }
   oldstack = stack;
   if (!check_ip_string(str))
        {
            tell_player(p, " Sites must be in the form: n.n.n.n (*'s acceptable)\n");
            return;
        }
   site_type = string_to_ips(str);
   if (site_type==0)
   {
      tell_player(p, " Sorry, invalid site to check (read help sites)\n");
      return;
   }
   stack = ips_to_string();
   memset(address, 0, MAX_INET_ADDR);
   ref = address;
   while(*stack)
      *ref++ = *stack++;
   stack = oldstack;
   addref = address;
   sprintf(stack, "\n Site %s is ", address);
   stack = strchr(stack, 0);
   switch(site_type)
   {
      case 1:
         strcpy(stack, "a Class A address (1-126.*.*.*).\n");
         break;
      case 2:
         strcpy(stack, "a Class B address (128-191.n.*.*).\n");
         break;
      case 3:
         strcpy(stack, "a Class C address (192-223.n.n.*).\n");
         break;
      case 4:
         strcpy(stack, "localhost (127.*.*.*).\n");
         break;
      case 5:
         strcpy(stack, "a reserved address (224-254.n.n.n).\n");
         break;
      case 6:
         strcpy(stack, "a full wildcard (*.*.*.*).\n");
   }      
   stack = strchr(stack, 0);
   
   /* scan through the banish file for matches */
   ref = banish_file.where;
   while(*ref) /* while file exists */
   {
      ref2 = ref; /* remember start of line */
      while(*ref!='@') ref++; /* find @ */
      ref++; /* move to site */
      t = match_banish_wild(addref, ref);
      if (t) /* a match */
      {
         count++;
         /* write out the site */
         strcpy(stack, "\n Address: ");
         stack = strchr(stack, 0);
         while(isdigit(*ref) || *ref=='.' || *ref=='*')
            *stack++ = *ref++;
         ref+=3; /* get it to the name */
         switch(t)
         {
            case 1:
               strcpy(stack, " - (New users ban)\n");
               break;
            case 2:
               strcpy(stack, " - (Complete close)\n");
               break;
            case 3:
               strcpy(stack, " - (Logged connections)\n");
         }
         stack = strchr(stack, 0);
         strcpy(stack, " Placed by: ");
         stack = strchr(stack, 0);
         while(*ref!=' ')
            *stack++ = *ref++;
         ref++; /* to the reason */
         strcpy(stack, " on ");
         stack = strchr(stack, 0);
         /* copy the time */
         while(*ref2!='@')
            *stack++ = *ref2++;
         *stack++ = '\n';
         /* do the reason */
         strcpy(stack, " Reason: ");
         stack = strchr(stack, 0);
         while(*ref!='\n')
            *stack++ = *ref++;
         *stack++='\n';
         ref++; /* next line */
      }
      else /* skip line */
      {
         while(*ref!='\n') ref++;
         ref++;
      }
   }
   if (count==0)
   {
      strcpy(stack, " Sorry, no matches.\n");
      stack = strchr(stack, 0);
   }
   number = splat_timeout - (int)time(0);
   /* check splat */
   if (((ip1==255) || (ip1==splat1 && ip2==255) || (ip1==splat1 && ip2==splat2 && ip3==255) ||
       (ip1==splat1 && ip2==splat2 && ip3==splat3 && ip4==255) ||
       (ip1==splat1 && ip2==splat3 && ip3==splat3 && ip4==splat4)) && (number > 0))
   {
        *stack++='\n';
	if (splat2 == 255)
		sprintf(stack, " Site %d.*.*.* is splatted for %d more seconds.\n",
		splat1, number);
	else if (splat3 ==255)
		sprintf(stack, " Site %d.%d.*.* is splatted for %d more seconds.\n",
		splat1, splat2, number);
	else if (splat4 ==255)
		sprintf(stack, " Site %d.%d.%d.* is splatted for %d more seconds.\n",
		splat1, splat2, splat3, number);
	else
		sprintf(stack, " Site %d.%d.%d.%d is splatted for %d more seconds.\n",
		splat1, splat2, splat3, splat4, number);
	stack = strchr(stack, 0);
   }
   *stack++='\n';
   *stack++=0;
   pager(p, oldstack, 1);
   stack = oldstack;
}


void splat_player(player * p, char *str)
{
    time_t t;
    char *space, *oldstack;
    player *dummy;
    int no1, no2, no3, no4, tme = 0, type;
    
    tme=0;
    
    oldstack = stack;
    
    if (!*str)
    {
	tell_player(p, " Format: splat <person> <time>\n");
	return;
    }
    if (p->flags & BLOCK_SU)
    {
       tell_player(p, " You need to be on duty to do that!\n");
       return;
    }
    if ((space = strchr(str, ' ')))
    {
	*space++ = 0;
	tme = atoi(space);
    }
    dummy = find_player_global(str);
    if (!dummy)
	return;
    if (tme>10000000) tme=10000000;
    if ((p->residency & SU && !(p->residency & ADMIN) && (tme < 0 || tme > 30)) ||
	(p->residency & ADMIN && (tme < 0)))
    {
	tell_player(p, " That's not a very nice amount of time.  Set to 30 "
		    "minutes ...\n");
	tme = 30;
    }
    else
    {
	/* when no time specified */
	if (!tme)
	{
	    tell_player(p, "Time set to 5 minutes.\n");
	    tme = 5;
	}
    }
    
    
    sneeze(p, dummy->lower_name);
    if (!(dummy->flags & CHUCKOUT))
	return;
    t = time(0);
    splat_timeout = t + (tme * 60);
    type = return_subnet_class(dummy->num_addr);
    sscanf(dummy->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
    splat1 = no1;
    splat2 = no2;
    splat3 = no3;
    splat4 = no4;
    switch(type)
    {
       	case 1:
          vsu_wall("-=> Site %d.*.*.* banned for %d minutes because of %s\n",
	    no1, tme, dummy->name);
	  splat2 = 255;
	  splat3 = 255;
	  splat4 = 255;
	  break;
	case 2:
	  vsu_wall("-=> Site %d.%d.*.* banned for %d minutes because of %s\n",
	    no1, no2, tme, dummy->name);
	  splat3 = 255;
	  splat4 = 255;
	  break;
	case 3:
	  vsu_wall("-=> Site %d.%d.%d.* banned for %d minutes because of %s\n",
	    no1, no2, no3, tme, dummy->name);
	  splat4 = 255;
	  break;
	default:
	  vsu_wall("-=> Site %d.%d.%d.%d banned for %d minutes because of %s\n",
	    no1, no2, no3, no4, tme, dummy->name);
    }
    stack = oldstack;
}
