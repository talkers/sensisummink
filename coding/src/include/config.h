/*
 * -----------------------------------
 * START OF USER CONFIGURABLE SECTIONS
 * -----------------------------------
 */
 
/*
 * Machine options
 * Notes:
 * If your specifc revision isn't listed, choose the closest generic one.
 */
#undef	LINUX
#define	LINUX_GLIBC 1
#undef	OSF
#undef	OSF40
#undef	SOLARIS
#undef	SOLARIS24
#undef	SOLARIS25
#undef	SUNOS
#undef	SUNOS431
#undef	IRIX
#undef	IRIX52
#undef	IRIX62
#undef	IRIX63
#undef	FREEBSD
#undef	FREEBSD221
#undef	FREEBSD228

/*
 * Define this next one if passwords are failing on irix
 * Note - this will start to echo passwords whilst typing them.
 */
#undef IRIX_BUG
/*#define IRIX_BUG 1*/

/*
 * Define this next one if you have broken mstats.  A lot of ELF linux boxes
 * suffer from this - and it seems to be missing on glibc dists.
 */
#define BROKEN_MSTATS 1
/*#undef  BROKEN_MSTATS*/

/* 
 * want to debug malloc?  even know what this does?  If yes and yes, define
 * this next one
 */
#undef  MALLOC_DEBUG
/*#define MALLOC_DEBUG 1*/

/*
 * Server name and binary name (for the angel)
 */
#define TALKER_NAME "SensiSummink"
#define TALKER_BINARY "summink"

/* 
 * Server timing - don't play with this unless you know why.
 */
#define TIMER_CLICK (5)

/*
 * Memory settings - safe as they are
 * MAX_RES - maximum resident memory to be used in bytes
 * STACK_SIZE - size of ew2 stack in bytes
 * MAX_LOG_SIZE - size of log files before being wrapped
 */
#define MAX_RES (1048576)
#define STACK_SIZE (200001)
#define MAX_LOG_SIZE (5000)

/*
 * Pipe code settings - leave this
 */
#define NAME_MAX_IN_PIPE (10)
#define YOU_MAX_IN_PIPE (3)

/* 
 * How long until replies/friend replies time out
 */
#define REPLY_TIMEOUT (240)

/*
 * Command matching parser - define this to enable
 */
#define NEW_PARSER 1
/*#undef  NEW_PARSER*/

/*
 * Antipipe code - how bigger single block of text can users throw at it?
 * define ANTIPIPE to enable, and use MAX_ANTIPIPE to set (in characters)
 */
#define ANTIPIPE 1
#define MAX_ANTIPIPE (100)
/*#undef  ANTIPIPE*/

/* 
 * Antispam code - how many lines of matching text we accept before *boom*
 * define ANTISPAM to enable, and use MAX_ANTISPAM to set amount of spam
 */
#define ANTISPAM 1
#define MAX_ANTISPAM (10)
/*#undef  ANTISPAM*/

/*
 * Crash recovery code
 * define CRASH_RECOVER to enable, and use MAX_CRASH for how many crashes we
 * will recover before giving up and shutting down
 */
#define CRASH_RECOVER 1
#define MAX_CRASH (5)
/*#undef  CRASH_RECOVER*/

/*
 * Colour code - define to enable
 */
#define ANSI_COLS 1
/*#undef  ANSI_COLS*/

/*
 * Socials code (rubbish)
 */
#define SOCIALS 1
/*#undef  SOCIALS*/

/*
 * Robots code - define to enable and make sure you read the README file
 */
#undef  ROBOTS
/*#define ROBOTS 1*/

/*
 * Athanasius and Oolons Ident server.
 * define IDENT to use, DEBUG_IDENT and DEBUG_IDENT_TOO set server/client
 * side debugging.
 */ 
#define IDENT 1
#undef  DEBUG_IDENT
#undef  DEBUG_IDENT_TOO
/*
#define DEBUG_IDENT 1
#define DEBUG_IDENT_TOO 1
#undef  IDENT*/

/*
 * Grims intercom server.
 * define this to use the intercom.  define INTERCOM_EXT if you want the
 * intercom room code to function.  Remember to set the abbreviation for the
 * talker on the intercom system and the external talk room name.
 */
#define INTERCOM 1
#define INTERCOM_ABBR "sensi"
#define INTERCOM_EXT 1
#define INTERCOM_ROOM "intercom.external"
/*
#undef  INTERCOM
#undef  INTERCOM_EXT
*/
/*
 * Directory/port options
 * These should be pretty self explanatory...
 * the ROOT should be exactly where the account is on the filesystem, the
 * trailing / is important!
 */
#define CODING_PORT (4040)
#define CODING_ROOT "/home/users/someone/summink/coding/"
#define CODING_BACKUPS "/tmp/summink/coding/"
#define LIVE_PORT (4242)
#define LIVE_ROOT "/home/users/someone/summink/live/"
#define LIVE_BACKUPS "/tmp/summink/live/"
#define SOCKET_PATH "out/alive_socket"
#define INTERCOM_SOCKET "out/intercom_socket"
#define PID_FILE "out/PID"
#define ANGEL_PID "out/ANGEL_PID"

/*
 * Notes options (news and mail)
 * The timeouts and sync are in seconds.
 */
#define NOTE_HASH_SIZE (40)
#define NOTE_SYNC_TIME (1800)
#define NEWS_TIMEOUT (604800)
#define MAIL_TIMEOUT (604800)

/*
 * Playerfiles options
 * define DYNAMIC to use dynamic pfiles - and MAX_DYNAMIC_CACHE for the size
 * of the cache.  Note, setting the cache to the number of users you have
 * negates the whole point of dynamic files ;)  Also note setting it to zero
 * turns it off - direct disk access...
 * set TERM_LINES for the default terminal height
 * leave HASH_SIZE alone
 * set SYNC_TIME for how often in seconds you want files to be written to disk
 * automatically
 * set PLAYER_TIMEOUT to time in seconds till a user is wiped.
 * set BACKUPS_TIME to a string of the format HH:MM:SS for when you want
 * the backups to happen
 */
#define DYNAMIC 1
#define MAX_DYNAMIC_CACHE (50)
/*#undef DYNAMIC*/
#define TERM_LINES (18)
#define HASH_SIZE (64)
#define SYNC_TIME (60)
#define PLAYER_TIMEOUT (7776000)
#define BACKUPS_TIME "05:30:00"

/*
 * No, really - if you need help with this one you're a lost cause.
 */
#define ENTRANCE_ROOM "summink.main"

/*
 * ---------------------------------
 * END OF USER CONFIGURABLE SECTIONS
 * ---------------------------------
 */




/* you don't need to touch the below items */


#define VERSION "3.2b" 

/* better do this ;) */
#if defined( LINUX_GLIBC )
#define LINUX 1
#define GLIBC 1
#endif
#if defined( IRIX63 )
#define IRIX62 1
#endif
#if defined( IRIX62 )
#define IRIX52 1
#endif
#if defined( IRIX52 )
#define IRIX 1
#endif
#if defined( OSF40 )
#define OSF 1
#endif
#if defined( SOLARIS24 ) || defined( SOLARIS25 )
#define SOLARIS 1
#endif
#if defined( SUNOS431 )
#define SUNOS 1
#endif
#if defined( FREEBSD228 )
#define FREEBSD 1
#endif
#if defined( FREEBSD221 )
#define FREEBSD 1
#endif


/* what does each one have available? */
#ifdef LINUX

#define USE_SIGACTION 1
#define NO_SIGSYS 1
#ifdef GLIBC
#define USE_SIGEMPTYSET 1
#endif /* glibc */

#elif SOLARIS

#define USE_SIGACTION 1
#define USE_SIGEMPTYSET 1
#define NO_RLIMIT_RSS 1
#define HAS_MALLINFO 1
#define NONBLOCKING 1
#define HAVE_FILIOH 1
#define HAVE_BCOPY 1
#ifndef SOLARIS24
#define HAVE_BZERO 1
#endif /* solaris24 */

#elif SUNOS

#define HAS_MALLINFO 1
#define NOALARM 1
#define NONBLOCKING 1
#define HAVE_FILIOH 1
#define HAVE_BZERO 1
#define HAVE_BCOPY 1

#elif OSF

#define HAS_MALLINFO 1
#define USE_SIGACTION 1
#define USE_SIGEMPTYSET 1
#define HAVE_BCOPY 1
#define HAVE_BZERO 1
#define NOALARM 1

#elif IRIX

#define HAS_MALLINFO 1
#define USE_SIGACTION 1
#define USE_SIGEMPTYSET 1
#define HAVE_BZERO 1
#define HAVE_BCOPY 1
#define NOALARM 1
#define NONBLOCKING 1
#define DONT_USE_IOCTL 1

#elif FREEBSD

#define USE_SIGACTION 1
#define USE_SIGEMPTYSET 1
#define HAVE_BZERO 1
#define HAVE_BCOPY 1

#endif /* linux, solaris, osf, irix */

/* function line tracing for socket code debugging */
#define tostring(x)	#x
#define FLINE(x)	__FILE__":"tostring(x)
#define fline		FLINE(__LINE__) 

/* max number holdable by an int - this needs better 64 bit support. */
#define MYMAXINT 65535

/* the system equivalent of the timelocal command */
#ifdef SUNOS
#define TIMELOCAL(x) timelocal(x)
#else
#define TIMELOCAL(x) mktime(x)
#endif

/* void * fixes for aligned wrongly variables on solaris - a kludge 
   maybe sunos, hpux, and ultrix need these? */
#if defined ( SOLARIS ) || defined ( SUNOS ) || defined ( OSF ) || defined ( IRIX )
#define ALFIX	(void *)
#else
#define ALFIX	
#endif

/* default port no - set this right for coding and live versions */
/* followed by root directory for each one */
#ifdef LIVE
#define DEFAULT_PORT LIVE_PORT
#define ROOT LIVE_ROOT
#define BACKUPS_DIR LIVE_BACKUPS
#elif CODING
#define DEFAULT_PORT CODING_PORT
#define ROOT CODING_ROOT
#define BACKUPS_DIR CODING_BACKUPS
#endif

/* which malloc routines to use */
#define MALLOC malloc
#define FREE free

#ifdef INTERCOM
#define INTERCOM_PORT (DEFAULT_PORT - 1)
#endif /* intercom */

