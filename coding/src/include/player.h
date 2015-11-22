/*
 * player.h
 */

/* kludgy macro, there must be a better way to do this */

#define PRIVS(p) ((p)->residency & (PSU|SU|LOWER_ADMIN|ADMIN|HCADMIN))
#ifdef OSF
#define align(p) p=(void *)(((long)p+7)&-8)
#else
#define align(p) p=(void *)(((int)p+3)&-4)
#endif

/* modes that players can be in */

#define NONE 0
#define PASSWORD 1
#define CONV 2
#define ROOMEDIT 4
#define MAILEDIT 8
#define NEWSEDIT 16
#define SNEWSEDIT 32

/* gender types */

#define MALE 1
#define FEMALE 2
#define PLURAL 3
#define OTHER 0
#define VOID_GENDER -1

/* residency types */

#define STANDARD_ROOMS -2
#define NON_RESIDENT 0
#define BASE 1
#define NO_SYNC (1<<1)
#define ECHO_PRIV (1<<2)
#define NO_TIMEOUT (1<<3)
#define BANISHD (1<<4)
#define SPARE5 (1<<5)
#define MAIL (1<<6)
#define LIST (1<<7)
#define BUILD (1<<8)
#define SESSION (1<<9)
#ifdef ROBOTS
#define ROBOT_PRIV (1<<10)
#else
#define SPARE10 (1<<10)
#endif
#define SPARE11 (1<<11)
#define SPARE12 (1<<12)
#define SPARE13 (1<<13)
#define SPARE14 (1<<14)
#define SPARE15 (1<<15)
#define SPARE16 (1<<16)
#define SPARE17 (1<<17)
#define SPARE18 (1<<18)
#define SPARE19 (1<<19)
#define SPARE20 (1<<20)
#define SPARE21 (1<<21)
#define WARN (1<<22)
#define TRACE (1<<23)
#define SCRIPT (1<<24)
#define PSU (1<<25)
#define SU (1<<26)
#define LOWER_ADMIN (1<<27)
#define ADMIN (1<<28)
#define HCADMIN (1<<29)
#define SPARE30 (1<<30)

#define HCADMIN_INIT (HCADMIN + ADMIN + LOWER_ADMIN + SU \
                     + TRACE + SCRIPT + WARN + PSU + SESSION \
                     + BUILD + LIST + MAIL + NO_TIMEOUT + ECHO_PRIV + BASE )

/* #define lengths */

#define MAX_NAME 20
#define MAX_INET_ADDR 40
#define IBUFFER_LENGTH 256
#define MAX_REPEAT_STRING 512 /* this is simply paranoid */
#define MAX_PROMPT 15
#define MAX_ID 15
#define MAX_EMAIL 60
#define MAX_PASSWORD 20
#define MAX_TITLE 60
#define MAX_DESC 160
#define MAX_PLAN 160
#define MAX_PRETITLE 18
#define MAX_ENTER_MSG 60
#define MAX_IGNOREMSG 55
#define MAX_SESSION 60
#define MAX_COMMENT 59
/* an un-subtle size, but... */
#define MAX_REPLY 200
#define MAX_ROOM_CONNECT 35
#define MAX_ROOM_NAME 50
#define MAX_AUTOMESSAGE 160
#define MAX_ROOM_SIZE 1500
#define MAX_ARTICLE_SIZE 5000
#define MAX_COLS 7
#ifdef IDENT
#define MAX_REMOTE_USER 40
#endif

/* system flag definitiosn */

#define PANIC (1<<0)
#define VERBOSE (1<<1)
#define SHUTDOWN (1<<2)
/* missing flag 8 */
#define EVERYONE_TAG (1<<3)
#define FAILED_COMMAND (1<<4)
#define CLOSED_TO_NEWBIES (1<<5)
#define PIPE (1<<6)
#define ROOM_TAG (1<<7)
#define FRIEND_TAG (1<<8)
#define DO_TIMER (1<<9)
#define UPDATE (1<<10)
#define NO_PRINT_LOG (1<<11)
#define NO_PRETITLES (1<<12)
#define UPDATEROOMS (1<<13)
#define UPDATEFLAGS (1<<14)
#define NEWBIE_TAG (1<<15)
#define REPLY_TAG (1<<16)
#define SECURE_DYNAMIC (1<<17)
#define INVIS_SAVE (1<<18)
#ifdef ROBOTS
#define PROCESSING_ROBOTS (1<<19)
#endif
#ifdef CRASH_RECOVER
#define CRASH_RECOVERING (1<<20)
#endif

/* player flag defs */

/* keep PANIC as 1 */
#define INPUT_READY (1<<1)
#define LAST_CHAR_WAS_N (1<<2)
#define LAST_CHAR_WAS_R (1<<3)
#define DO_LOCAL_ECHO (1<<4)
#define PASSWORD_MODE (1<<5)
/* keep closed to newbies at 64 */
#define PROMPT (1<<7)
#define TAGGED (1<<8)
#define LOGIN (1<<9)
#define CHUCKOUT (1<<10)
#define EOR_ON (1<<11)
#define IAC_GA_DO (1<<12)
#define SITE_LOG (1<<13)
#define DONT_CHECK_PIPE (1<<14)
#define RECONNECTION (1<<15)
#define NO_UPDATE_L_ON (1<<16)
#define BLOCK_SU (1<<17)
#define NO_SAVE_LAST_ON (1<<18)
#define FORCE_COLOUR (1<<19)
#define ASSISTED (1<<20)
#define FROGGED (1<<21)
#define SCRIPTING (1<<22)
#define ARGH_DONT_CACHE_ME (1<<23)
#define TRIED_QUIT (1<<24)
#ifdef ANTISPAM
#define SPAMMER (1<<25)
#endif
#ifdef ROBOTS
#define ROBOT (1<<26)
#endif

/* ones that get saved */

#define CONVERSE (1<<0)
#define SU_HILITED (1<<1)
#define BLOCK_TELLS (1<<2)
#define BLOCK_SHOUT (1<<3)
#define HIDING (1<<4)
#define PRIVATE_EMAIL (1<<5)
#define QUIET_EDIT (1<<6)
#define TAG_ECHO (1<<7)
#define TAG_PERSONAL (1<<8)
#define TAG_ROOM (1<<9)
#define TAG_SHOUT (1<<10)
#define TRANS_TO_HOME (1<<11)
#define SEEECHO (1<<12)
#define SNEWS_INFORM (1<<13) /* used to be NEW_MAIL */
#define NO_ANONYMOUS (1<<14)
#define MAIL_INFORM (1<<15)
#define NEWS_INFORM (1<<16)
#define TAG_AUTOS (1<<17)
#define NOEPREFIX (1<<18)
#define IAC_GA_ON (1<<19)
#define COMPRESSED_LIST (1<<20)
#define NO_PAGER (1<<21)
#define AGREED_DISCLAIMER (1<<22)
#define NOPREFIX (1<<23)
#define SAVENOSHOUT (1<<24)
#define SAVEDFROGGED (1<<25)
#define YES_SESSION (1<<26)
#define SAVEDJAIL (1<<27)
#define ROOM_ENTER (1<<28)
#define SHOW_EXITS (1<<29)
#define SPARE30 (1<<30)

/* second set of saved flags */
#define BLOCK_FRIEND (1<<0)
#define LIST_FAILS (1<<1)
#define BLOCK_TF_REPLIES (1<<2)
#ifdef ANSI_COLS
#define SEE_COLOUR (1<<3)
#endif

/* list flags */
#define NOISY 1
#define IGNORE 2
#define INFORM 4
#define GRAB 8
#define FRIEND 16
#define BAR 32
#define INVITE 64
#define BEEP 128
#define BLOCK 256
#define KEY 512
#define FIND 1024
#define FRNDBLOCK 2048
#define MAILBLOCK 4096
#define PREFERRED 8192

/* command types */
#define VOID (1<<0)
#define SEE_ERROR (1<<1)
#define PERSONAL (1<<2)
#define ROOM (1<<3)
#define EVERYONE (1<<4)
#define ECHO_COM (1<<5)
#define EMERGENCY (1<<6)
#define AUTO (1<<7)
#define HIGHLIGHT (1<<8)
#define NO_P_MATCH (1<<9)
#define TAG_INFORM (1<<10)
#define LIST_EVERYONE (1<<11)
#define ADMIN_BARGE (1<<12)
#define SORE (1<<13)
#define WARNING (1<<14)
#define EXCLUDE (1<<15)
#define FRIEND_COM (1<<16)
#define TFREPLY (1<<19)
#define MULTITELL (1<<20)
#define CHANNEL (1<<21)

/* do_inform flags */
#define iINF_SU (1<<0)
#define iINF_FRIEND (1<<1)
#define iINF_NEWBIE (1<<2)
#define iBEEP_FRIEND (1<<3)
#ifdef ROBOTS
#define iINF_ROBOT (1<<4)
#endif

/* files'n'things */
typedef struct
{
   char           *where;
   int             length;
}               file;

#ifdef DYNAMIC 
/* dynamic playerfiles cache struct */
struct c_struct
{
   file		data;	/* where their data goes */
   int		last_access; /* used for caching checks */
   struct s_struct *sp; /* for backwards reference */
};
typedef struct c_struct cache;
#endif

/* room definitions */

#define HOME_ROOM 1
#define COMPRESSED 2
#define AUTO_MESSAGE 4
#define AUTOS_TAG 8
#define LOCKABLE 16
#define LOCKED 32
#define OPEN 64
#define LINKABLE 128
#define KEYLOCKED 256
#define CONFERENCE 512
#define ROOM_UPDATED 1024

struct r_struct
{
   char            name[MAX_ROOM_NAME];
   char            id[MAX_ID];
   int             flags;
   int             data_key;
   int             auto_count;
   int             auto_base;
   file            text;
   file            exits;
   file            automessage;
   struct s_struct *owner;
   struct p_struct *players_top;
   struct r_struct *next;
   char            enter_msg[MAX_ENTER_MSG];
};

typedef struct r_struct room;

/* note defs */

#define NEWS_ARTICLE 1
#define ANONYMOUS 2
#define NOT_READY 4
#define SUPRESS_NAME 8
#define SNEWS_ARTICLE 16

struct n_struct
{
   int             id;
   int             flags;
   int             date;
   file            text;
   int             next_item;
   int             read_count;
   struct n_struct *hash_next;
   char            header[MAX_TITLE];
   char            name[MAX_NAME];
};
typedef struct n_struct note;

/* flags for received mail - NEW */
#define reNEW (1<<0)
#define reFRIEND (1<<1)
#define reREPLIED (1<<2)
#define reFORWARDED (1<<3)

/* structure for received mail */
struct re_struct
{
   int		mail_received; /* actual note id */
   int		flags;
   struct re_struct *next;
};
typedef struct re_struct recmail;

/* list defs */

struct l_struct
{
   char            name[MAX_NAME];
   int             flags;
   struct l_struct *next;
};
typedef struct l_struct list_ent;

/* ok, in -so- many places in the code, l and lf are sought, in a big global
   scan routine.  this is code replication==bad, so we have a new struct.
   All it needs are two pointers. */
struct l_scan_struct
{
   list_ent *l;
   list_ent *lf;
};
typedef struct l_scan_struct list_scan;


/* saved player defs */
struct s_struct
{
   char            lower_name[MAX_NAME];
   char            last_host[MAX_INET_ADDR];
   char		   saved_email[MAX_EMAIL];
   int             last_on;
   int             residency;
   int             saved_flags;
#ifdef DYNAMIC
   int		   cache;
#else
   file		   data;
#endif
   struct l_struct *list_top;
   struct r_struct *rooms;
   int             mail_sent;
   struct re_struct *received_list;
   struct s_struct *next;
};
typedef struct s_struct saved_player;


/* terminal defs */

struct terminal
{
   char           *name;
   char           *bold;
   char           *off;
   char           *cls;
};



/* flag list def */

typedef struct
{
   /* changed to const for Wwrite-string */
   char           *text;
   int             change;
}               flag_list;

struct p_struct;
typedef void player_func(struct p_struct *);
typedef void command_func(struct p_struct *, char *);


/* robots - insert decl's here if required ;) */
#ifdef ROBOTS
#include "robot_player.h"
#endif


/* flag for command types */
#define cCOMMS (1<<0)
#define cCUST (1<<1)
#define cINFO (1<<2)
#define cMOVE (1<<3)
#define cROOM (1<<4)
#define cLIST (1<<5)
#define cSYS (1<<6)
#define cMISC (1<<7)
#define cSU (1<<8)
#define cADM (1<<9)
#define cNO_MATCH (1<<10)
#define cTAGGED (1<<11)
#define cNO_SPACE (1<<12)
#ifdef SOCIALS
#define cSOC (1<<13)
#endif

/* structure for commands */
struct command
{
   char           *text;
   command_func   *function;
   int             level;
   int             andlevel;
   char           *help;
   int             type;
};


/* editor info structure */

typedef struct
{
   char           *buffer;
   char           *current;
   int             max_size;
   int             size;
   player_func    *finish_func;
   player_func    *quit_func;
   int             flag_copy;
   int             sflag_copy;
   int		   sflags_copy;
   command_func   *input_copy;
   void           *misc;
}               ed_info;

/* the player structure */

struct p_struct
{
   int             fd;
   int             performance;
   int             hash_top;
   int             flags;
   int             term;
   int             saved_flags;
   int		   sflags;
   int             anticrash;
   int             residency;
   int             saved_residency;
   int             term_width;
   int             column;
   int             word_wrap;
   int             idle;
   int             gender;
   int             no_shout;
   int             shout_index;
   int             jail_timeout;
   int             no_move;
   int             lagged;
   int             script;
   int             jetlag;	/* This has just become time zone difference */
   int             sneezed;	/* wibble! */
   int             birthday;
   int             age;
   struct p_struct *hash_next;
   struct p_struct *flat_next;
   struct p_struct *flat_previous;
   struct p_struct *room_next;
   saved_player   *saved;
   room           *location;
   int             max_rooms;
   int             max_exits;
   int             max_autos;
   int             max_list;
   int             max_mail;
   int             on_since;
   int             total_login;
   ed_info        *edit_info;
   char            inet_addr[MAX_INET_ADDR];
   char            num_addr[MAX_INET_ADDR];
   char            name[MAX_NAME];
   char            title[MAX_TITLE];
   char            pretitle[MAX_PRETITLE];
   char            description[MAX_DESC];
   char            plan[MAX_PLAN];
   char            lower_name[MAX_NAME];
   char            idle_msg[MAX_TITLE];

   char            ignore_msg[MAX_IGNOREMSG];
   char            comment[MAX_COMMENT];
   char            reply[MAX_REPLY];
   char            room_connect[MAX_ROOM_CONNECT];
   int             reply_time;
   int		   tfreplytime;

   struct command *command_used;
   command_func   *input_to_fn;
   player_func    *timer_fn;
   int             timer_count;
   char            ibuffer[IBUFFER_LENGTH];
   int             ibuff_pointer;
   char            prompt[MAX_PROMPT];
   char            converse_prompt[MAX_PROMPT];
   char            email[MAX_EMAIL];
   char            password[MAX_PASSWORD];
   char            password_cpy[MAX_PASSWORD];
   char            enter_msg[MAX_ENTER_MSG];
   char            script_file[MAX_NAME + 16];
   char            assisted_by[MAX_NAME];
   int             logged_in;
   int             mode;
   
/* new for sensisummink */
   command_func	  *repeat_command;
   char		   repeat_string[MAX_REPEAT_STRING];
   int		   failpasswd;

/* antispam code */
#ifdef ANTISPAM
   char		   ibuffer_copy[IBUFFER_LENGTH];
   int		   matched_ibuffer_count;
#endif

/* ident server information */
#ifdef IDENT
   char		   userID[MAX_REMOTE_USER+1];
   int		   ident_id;
#endif

/* colours */
#ifdef ANSI_COLS
   int		   colours[MAX_COLS]; 
#endif
};

typedef struct p_struct player;

/* global definitions */

#ifndef GLOBAL_FILE

extern int            backup;
extern char           *action;
extern char    *stack, *stack_start;
extern int      sys_flags, max_players, current_players, command_type, up_time,
                up_date, logins;
extern short int talker_port;
extern player  *flatlist_start, *hashlist[], *current_player, *stdout_player, *input_player;
extern room    *current_room, *entrance_room, *prison;
extern player **pipe_list;
extern int      pipe_matches;
extern int      splat1, splat2, splat3, splat4, splat_timeout;
extern int	resolverFD;
#ifdef ANTISPAM
extern int newbie_reopen_count;
#endif
#ifdef INTERCOM
extern int intercom_fd,intercom_pid,intercom_port,intercom_last;
#ifdef INTERCOM_EXT
extern room *intercom_room;
#endif /* intercom_ext */
#endif /* intercom */
#endif


extern int      in_total, out_total, in_current, out_current, in_average,
                out_average, net_count, in_bps, out_bps, in_pack_total,
                out_pack_total, in_pack_current, out_pack_current, in_pps,
                out_pps, in_pack_average, out_pack_average;

