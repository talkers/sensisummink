/*
 * globals.c
 */

#define GLOBAL_FILE

#include "include/config.h"
#include "include/player.h"

/* boot thangs */
#ifdef ANTISPAM
int		newbie_reopen_count = -1;
#endif /* antispam */
#ifdef INTERCOM
int		intercom_fd=-1, intercom_pid, intercom_port, intercom_last;
#ifdef INTERCOM_EXT
room		*intercom_room;
#endif /* intercom_ext */
#endif /* intercom */
int		resolverFD = -1;
int             up_date;
int             logins = 0;
short int 	talker_port;

/* sizes */

int             max_players, current_players = 0;

int             in_total = 0, out_total = 0, in_current = 0, out_current = 0, in_average = 0,
                out_average = 0, net_count = 10, in_bps = 0, out_bps = 0, in_pack_total = 0,
                out_pack_total = 0, in_pack_current = 0, out_pack_current = 0, in_pps = 0,
                out_pps = 0, in_pack_average = 0, out_pack_average = 0;


/* One char for splat sites */

int             splat1 = 255, splat2 = 255, splat3 = 255, splat4 = 255;
int             splat_timeout;

/* sessions!  */

char            session[MAX_SESSION];
int             session_reset = 0;
player         *p_sess = 0;
char            sess_name[MAX_NAME] = "";

/* flags */

int             sys_flags = 0;
int             command_type = 0;

/* pointers */

char           *action;
char           *stack, *stack_start;
player         *flatlist_start;
player         *hashlist[27];
player         *current_player;
room           *current_room;
player         *stdout_player;

player        **pipe_list;
int             pipe_matches;

room           *entrance_room, *prison, *colony, *comfy, *boot_room;

/*
 * lists for use with idle times its here for want of a better place to put it
 */

file            idle_string_list[] = {
   {"has just hit return.\n", 0},
   {"is typing merrily away.\n", 10},
   {"hesitates slightly.\n", 15},
   {"is thinking about what to type next.\n", 25},
   {"appears to be stuck for words.\n", 40},
   {"ponders thoughtfully about what to say.\n", 60},
   {"stares oblivious into space.\n", 200},
   {"is on the road to idledom.\n", 300},
   {"is off to the loo ?\n", 600},
   {"appears to be doing something else.\n", 900},
   {"is slipping into unconsciousness.\n", 1200},
   {"has fallen asleep at the keyboard.\n", 1800},
   {"snores loudly.\n", 2400},
   {"moved !! .... no sorry, false alarm.\n", 3000},
   {"seems to have passed away.\n", 3600},
   {"is dead and buried.\n", 5400},
   {"is decomposing, someone drag that corpse.\n", 7200},
{0, 0}};

