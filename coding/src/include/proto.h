/* a list of procs that may be needed by any file */

#include <sys/time.h>

#include "player.h"

/* a couple of macros to save typing */
#define dynamic_seek_block(df,blocknum) \
  if (lseek(df->data_fd,(blocknum)*(df->granularity),SEEK_SET)<0) \
    handle_error("Failed to seek block");
#define pcp(df) printf("Current position = %d\n",lseek(df->data_fd,0,SEEK_CUR));
/* what the dfile structure looks like */
typedef struct {
  int data_fd;		/* file desc of the open data file */
  char fname[50];	/* the name of the file */
  int nkeys;		/* number of keys in the file */
  int *keylist;         /* pointer to the keys */
  int granularity;      /* size of each block in the data file */
  int first_free_block; /* start of the free block list */
  int first_free_key;   /* start of the free key list */
} dfile;


/* admin.c */
extern int		check_privs(int, int);
extern void		sneeze(player *, char *);
extern void         	soft_eject(player *, char *);

/* clist.h */
extern struct command 	check_list[];
extern struct command 	editor_list[];
extern struct command 	keyroom_list[];
extern struct command	mail_list[];
extern struct command	news_list[];
extern struct command   room_list[];
extern struct command   snews_list[];
#ifdef INTERCOM
extern struct command	intercom_list[];
#endif

/* commands.c */
extern int		count_caps(char *);
extern char         	*do_crypt(char *, player *);
extern void		quit(player *, char *);
extern void		remote(player *, char *);
extern void		tell_fn(player *, char *);
#ifdef INTERCOM
extern void		examine(player *, char *);
extern void		finger(player *, char *);
extern void		lsu(player *, char *);
#endif


/* dynamic.c */
extern void		dynamic_defrag_rooms(player *, char *);
extern void		dynamic_free(dfile *, int);
extern int		dynamic_load(dfile *, int, char *);
extern int		dynamic_save(dfile *, char *, int, int);
extern void		dynamic_validate_rooms(player *, char *);
extern dfile        	*dynamic_init(char *,int);

/* globals.c */
extern file     	idle_string_list[];
extern player  		*p_sess;
extern char     	sess_name[];
extern char     	session[];
extern int		session_reset;

/* glue.c */
extern void             free_files(void);
extern void             load_files(int);
extern char		*birthday_string(time_t);
extern char		*caps(char *);
extern char		*convert_time(time_t);
extern char         	*end_string(char *);
extern char		*full_name(player *);
extern char		*get_gender_string(player *);
extern char		*gstring(player *);
extern char    		*gstring_possessive(player *);
extern void		handle_error(char *);
extern char		*havehas(player *);
extern char		*isare(player *);
extern file		load_file(char *);
extern file		load_file_verbose(char *, int);
extern void		log(char *, char *);
extern void         	lower_case(char *);
extern char		*number2string(int);
extern int		save_file(file *, char *);
extern char		*plural_es(player *);
extern char		*single_s(player *);
extern char		*numeric_s(int);
extern char		*numeric_es(int);
extern char          	shutdown_reason[];
extern char		*sys_time(void);
extern char    		*time_diff(int);
extern char		*time_diff_sec(time_t, int);
extern char		*trailing_s(player *);
extern char    		*waswere(player *);
extern char		*word_time(int);
extern void		vlog(char *, char *, ...);
extern void		vsu_wall(char *, ...);
extern void		vsu_wall_but(player *, char *, ...);
extern void		vtell_player(player *, char *, ...);
extern void		vtell_current(char *, ...);
extern void		vtell_room(room *, char *, ...);
extern void		vtell_room_but(player *, room *, char *, ...);

/* editor.c */
extern void     	finish_edit(player *);
extern void		pager(player *, char *, int);
extern void 		start_edit(player *, int, player_func *, player_func *, char *);

/* ip.c */
extern int		do_banish(player *);

#ifdef INTERCOM
/* intercom.c */
extern void		check_idle(player *, char *);

extern void		do_intercom_examine(player *, char *);
extern void		do_intercom_finger(player *, char *);
extern void		do_intercom_idle(player *, char *);
extern void		do_intercom_lsu(player *, char *);
extern void		do_intercom_remote(player *, char *);
extern void		do_intercom_tell(player *, char *);
extern void		do_intercom_who(player *, char *);
extern int		establish_intercom_server(void);
extern void		kill_intercom(void);
extern void		parse_incoming_intercom(void);
#ifdef INTERCOM_EXT
extern void		intercom_room_look(player *);
extern void		do_intercom_room_exit_inform(player *);
extern void		do_intercom_room_enter_inform(player *);
extern void		do_intercom_say(player *, char *);
extern void		do_intercom_emote(player *, char *);
extern void		do_intercom_think(player *, char *);
#endif
#endif

/* lists.c */
extern list_scan	listscan(player *, player *);
extern list_scan	listscan_saved(saved_player *, player *);
extern char		*bit_string(int);
extern void		blocktells(player *, char *);
extern void     	check_list_resident(player *);
extern void     	check_list_newbie(char *);
extern void		construct_list_save(saved_player *);
extern void     	decompress_list(saved_player *);
extern void		do_inform(player *, char *);
extern void		earmuffs(player *, char *);
extern list_ent		*find_list_entry(player *, char *);
extern list_ent     	*fle_from_save(saved_player *, char *);
extern char		*retrieve_list_data(saved_player *, char *);
extern int      	test_receive(player *);

/* mail.c */
extern int		new_mail_check(player *);
extern void		construct_mail_save(saved_player *);
extern int		delete_received_fn(player *, int);
extern int		delete_sent_fn(player *, int);
extern note         	*find_note(int);
extern void		init_notes(void);
extern int      	nhash_update[];
extern char		*retrieve_mail_data(saved_player *, char *);
extern void		sync_notes(int);

/* parse.c */
extern void             divider_line(player *);
extern void             titled_line(player *, char *);
extern void		actual_timer(int);
extern char		*first_char(player *);
extern int          	get_flag(flag_list *, char *);
extern void		init_help(void);
extern void		input_for_one(player *);
extern void		init_parser(void);
extern void		match_commands(player *, char *);
extern char 		*next_space(char *);
extern void		process_players(void);
extern int        	shutdown_count;
extern void		sub_command(player *, char *, struct command *);
extern void		timer_function(void);
extern void		view_sub_commands(player *, struct command *);

/* plists.c */
extern int		dump_out_emails_fn(player *, int, int);
extern char 		*get_int(int *,char *);
extern char		*get_int_safe(int *, char *, file);
extern char    		*get_string(char *, char *);
extern char		*get_string_safe(char *, char *, file);
extern char    		*get_word(int *, char *);
extern char 		*store_int(char *,int);
extern char    		*store_string(char *, char *);
extern char    		*store_word(char *, int);
extern int      	check_password(char *, char *, player *);
extern void     	connect_to_prog(player *);
extern void		create_banish_file(char *);
extern void		destroy_player(player *);
extern void		do_birthdays(void);
extern void		do_update(int);
extern saved_player 	*find_saved_player(char *);
extern saved_player 	*find_top_player(char, int);
extern void		fork_the_thing_and_sync_the_playerfiles(void);
extern void         	hard_load_one_file(char);
extern void		init_plist(void);
extern int		load_player(player *);
extern int		match_player(char *, char *);
extern file         	newban_msg, nonewbies_msg, connect_msg, motd_msg,
                    	newpage1_msg, newpage2_msg, disclaimer_msg;
extern void     	player_flags(player *);
extern void		player_flags_verbose(player *, char *);
extern void		remove_entire_list(char);
extern int          	remove_player_file(char *);
extern int		restore_player(player *, char *);
extern int      	restore_player_title(player *, char *, char *);
extern void		save_player(player *);
extern saved_player 	**saved_hash[];
extern void		set_update(char);
extern void		sync_all(void);
extern void		sync_to_file(char, int);
extern int		update[];
#ifdef DYNAMIC
extern int	     	find_cached_player(char *);
extern int		sync_cache_item_to_disk(int);
extern void		empty_cache_item(int);
extern void		sync_cache_letter(char);
#endif

/* room.c */
extern void         	all_players_out(saved_player *);
extern room		*boot_room;
extern room    		*colony;
extern room         	*comfy;
#ifdef INTERCOM
extern room		*intercom_room;
#endif
extern void		compress_room(room *);
extern void     	construct_room_save(saved_player *);
extern room		*convert_room(player *, char *);
extern room    		*create_room(player *);
extern int		decompress_room(room *);
extern void     	free_room_data(saved_player *);
extern void		init_rooms(void);
extern int		move_to(player *, char *, int);
extern int      	possible_move(player *, room *, int);
extern char		*retrieve_room_data(saved_player *, char *);
extern dfile 		*room_df;
extern void         	to_room1(room *, char *, player *);
extern void         	to_room2(room *, char *, player *, player *);
extern void		trans_to(player *, char *);

/* session.c */
extern void		make_reply_list(player *, player **, int);

/* socket.c */
extern struct terminal 	terms[];
extern void     	alive_connect(void);
extern file  		hitells_msg, banned_msg, banish_file, sig_file, banish_scan, banish_msg, full_msg, newbie_msg, splat_msg;
extern void     	close_down_socket(void);
extern void		do_alive_ping(void);
extern void		do_prompt(player *, char *);
extern void		init_socket(void);
extern void		non_block_tell(player *, char *);
extern void		scan_sockets(void);
extern void		tell_current(char *);
extern void		tell_player(player *, char *);
#ifdef IDENT
 extern int		init_ident_server(void);
#endif
#ifdef INTERCOM
 extern void		close_only_main_fd(void);
#endif

/* tag.c */
extern void		cleanup_tag(player **, int);
extern void		construct_name_list(player **, int);
extern player		*create_player(void);
extern char		*do_pipe(player *, char *);
extern void     	extract_pipe_global(char *);
extern void   		extract_pipe_local(char *);
extern player       	*find_player_absolute_quiet(char *);
extern player   	*find_player_global(char *);
extern player   	*find_player_global_quiet(char *);
extern int		friend_tag_but(player *, player *);
extern int		global_tag(player *, char *, int);
extern int		global_tag_no_p(player *p, char *);
extern char		*idlingstring(player *, player **, int);
extern int		local_tag(player *, char *);
extern char         	*name_string(player *, player *);
extern void		news_wall_but(player *, char *);
extern void		snews_wall_but(player *, char *);
extern void		password_mode_off(player *);
extern void		password_mode_on(player *);
extern void		pstack_mid(char *);
extern void		raw_wall(char *);
extern int		saved_tag(player *, char *, int);
extern char          	*self_string(player *);
extern void     	swho(player *, char *);
extern void		su_wall(char *);
extern void		su_wall_but(player *, char *);
extern char 		*tag_string(player *, player **, int);
extern void		tell_room(room *, char *);
extern void		tell_room_but(player *, room *, char *);
extern char		*their_player(player *);

#ifdef ROBOTS
#include "robot_proto.h"
#endif
