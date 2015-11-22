/*
 * anti crash code, by subtle
 */

#define REC_VERS "1.1"

#include <setjmp.h>

/* somewhere near the top of glue.c  */
jmp_buf recover_jmp_env;
int total_recovers;

void mid_program_error(int dummy)
{
  int meep;
  
  /* if we reach here, command_type SHOULD BE ZERO, since it's going to go
     back out into the initial main loop */
  command_type=0;
  sys_flags |= CRASH_RECOVERING;
  
  stack = stack_start;
  
  total_recovers++;
  if(MAX_CRASH==0) 
    meep=0;
  else
    meep = MAX_CRASH - total_recovers;
  
        if(meep==1)
           raw_wall( "-=> Situation critical, the floors and walls fall away.\n" );
        else if(meep==2)
           raw_wall ( "-=> The walls crumble relentlessly.\n" );
	else if(meep==3)
	   raw_wall ( "-=> There is a loud groan as the floor splits in two.\n" );
	else if(meep==4)
	   raw_wall ( "-=> A small crack appears in the wall.\n" );
	else if(meep>0)
	   raw_wall ("-=> The floor vibrates slightly.\n");
        
  if(!meep)
    handle_error("Too many recovered crashes..");

  if( current_player )
  {
     vsu_wall("-=> %s just caused a program error, recovering...\n", current_player->name);
     if(*(current_player->ibuffer))
        vlog("recover", "%s caused program error using: %s", current_player->name, current_player->ibuffer);
     else
        vlog("recover", "%s caused program error with no buffer", current_player->name);
     current_player->flags |= CHUCKOUT;
     tell_player( current_player, "\n\n\nThis command has caused a system error to occur.  Connection Terminated.\007\n" );
     quit( current_player, 0 );
  }
  else
  {
     su_wall("-=> Program error occurred, recovering...\n");
     vlog("recover", "Program error recovered, action: %s", action);
  }
  sys_flags &= ~CRASH_RECOVERING;
  longjmp (recover_jmp_env, 0);
}


void crashrec_version(void)
{
  sprintf(stack, " -=> Anti-crash code V%s by subtle enabled.\n", REC_VERS);
  stack = strchr(stack, 0);
}
