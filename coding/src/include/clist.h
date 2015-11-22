
/*
 * clist.h
 */


/* The stuff for the dynamic rooms */
extern command_func dynamic_defrag_rooms;
extern command_func dynamic_dfstats;
extern command_func dynamic_test_func_blocks;
extern command_func dynamic_test_func_keys;
extern command_func dynamic_validate_rooms;

/* this is for testing purposes only */
extern command_func	crash;
extern command_func	resmail;

/* The stuff not for the dynamic rooms ;) */
extern command_func     abort_shutdown;
extern command_func     add_auto;
extern command_func	add_ban;
extern command_func     add_exit;
extern command_func     add_lag;
extern command_func     assist_player;
extern command_func     autos_com;
extern command_func	backup_command;
extern command_func     banish_player;
extern command_func	banish_show;
extern command_func     bar;
extern command_func     barge;
extern command_func     beep;
extern command_func     blank_email;
extern command_func     blank_list;
extern command_func     blank_password;
extern command_func     blank_prefix;
extern command_func     block;
extern command_func	block_friends;
extern command_func     block_su;
extern command_func     blocktells;
extern command_func     boot_out;
extern command_func     bounce;
extern command_func     change_auto_base;
extern command_func     change_limit;
extern command_func     change_email;
extern command_func     change_password;
extern command_func     change_room_entermsg;
extern command_func     change_room_id;
extern command_func     change_room_name;
extern command_func     check;
extern command_func	check_alist;
extern command_func     check_autos;
extern command_func     check_email;
extern command_func     check_entry;
extern command_func     check_exits;
extern command_func     check_idle;
extern command_func     check_info;
extern command_func     check_room;
extern command_func     check_rooms;
extern command_func     check_updates;
extern command_func     check_wrap;
extern command_func     clear_list;
extern command_func     clear_screen;
extern command_func     close_to_newbies;
extern command_func     comments;
extern command_func     confirm_password;
extern command_func     converse_mode_off;
extern command_func     converse_mode_on;
extern command_func     create_new_room;
extern command_func     delete_received;
extern command_func     delete_room;
extern command_func     delete_sent;
extern command_func     dest_note;
extern command_func     disclaimer;
extern command_func     do_grab;
extern command_func     do_save;
extern command_func     dump_com;
extern command_func	dump_out_emails;
extern command_func     earmuffs;
extern command_func     echo;
extern command_func     echoall;
extern command_func     edit_back_line;
extern command_func     edit_delete_line;
extern command_func     edit_end;
extern command_func     edit_forward_line;
extern command_func	edit_goto_bottom;
extern command_func     edit_goto_line;
extern command_func     edit_goto_top;
extern command_func     edit_quit;
extern command_func     edit_stats;
extern command_func     edit_view;
extern command_func     edit_view_commands;
extern command_func     edit_view_line;
extern command_func     edit_wipe;
extern command_func     emergency;
extern command_func     emote;
extern command_func     ereply;
extern command_func     examine;
extern command_func     exclude;
extern command_func     exit_mail_mode;
extern command_func     exit_news_mode;
extern command_func     exit_room_mode;
extern command_func	exit_snews_mode;
extern command_func	find_email;
extern command_func     finger;
extern command_func     followup;
extern command_func	followuptosunews;
extern command_func	forward_letter;
extern command_func     friend;
extern command_func	friendblock;
extern command_func     frog;
extern command_func     gender;
extern command_func     go_colony;
extern command_func     go_comfy;
extern command_func     go_home;
extern command_func	go_main;
extern command_func     go_quiet;
extern command_func     go_room;
extern command_func     grab;
extern command_func     grabable;
extern command_func     grant;
extern command_func	grep_log;
extern command_func     help;
extern command_func     hide;
extern command_func	homeview;
extern command_func     termtype;
extern command_func     ignore;
extern command_func	ignoreemoteprefix;
extern command_func     ignoreprefix;
extern command_func     inform;
extern command_func     inform_room_enter;
extern command_func     invite;
extern command_func     invites_list;
extern command_func     join;
extern command_func     key;
extern command_func     list_all_notes;
extern command_func     list_news;
extern command_func     list_notes;
extern command_func	list_snews;
extern command_func     listfind;
extern command_func     lnew;
extern command_func     look;
extern command_func     lsu;
extern command_func     mail_command;
extern command_func	mailblock;
extern command_func     make_new_character;
extern command_func     mode;
extern command_func     motd;
extern command_func     netstat;
extern command_func     new_blankpass;
extern command_func     new_set_list;
extern command_func     new_with;
extern command_func     news_command;
extern command_func     newthink;
extern command_func     noisy;
extern command_func     nopager;
extern command_func     nuke_player;
extern command_func     on_duty;
extern command_func     pemote;
extern command_func     player_flags_verbose;
extern command_func     player_stats;
extern command_func     post_news;
extern command_func	post_snews;
extern command_func	prefer;
extern command_func     premote;
extern command_func     prison_player;
extern command_func     privs;
extern command_func     public_com;
extern command_func     pulldown;
extern command_func	purge_list;
extern command_func     quit;
extern command_func     qwho;  
extern command_func     read_article;
extern command_func     read_letter;
extern command_func	read_sarticle;
extern command_func	read_sent;
extern command_func     recap;
extern command_func     recho;
extern command_func     recount_news;
extern command_func     relink_note;
extern command_func     reload;
extern command_func     remote;
extern command_func     remote_friends;
extern command_func     remote_think;
extern command_func     remove_article;
extern command_func     remove_auto;
extern command_func	remove_ban;
extern command_func     remove_exit;
extern command_func     remove_move;
extern command_func     remove_priv;
extern command_func	remove_sarticle;
extern command_func     remove_shout;
extern command_func     rename_player;
extern command_func	repeat;
extern command_func     reply;
extern command_func     reply_article;
extern command_func     reply_letter;
extern command_func	reply_sarticle;
extern command_func     report_error;
extern command_func	report_suggestion;
extern command_func     reset_session;
extern command_func     reset_sneeze;
extern command_func     resident;
extern command_func     restore_files;
extern command_func	rfreply;
extern command_func	rm_list;
extern command_func     room_bolt;
extern command_func     room_command;
extern command_func     room_edit;
extern command_func     room_entry;
extern command_func     room_link;
extern command_func     room_linkable;
extern command_func     room_lock;
extern command_func     room_lockable;
extern command_func     room_open;
extern command_func     same_site;
extern command_func     say;
extern command_func     script;
extern command_func     see_echo;
extern command_func     send_letter;
extern command_func     set_age;
extern command_func     set_birthday;
extern command_func     set_comment;
extern command_func     set_converse_prompt;
extern command_func     set_description;
extern command_func     set_enter_msg;
extern command_func     set_home;
extern command_func     set_idle_msg;
extern command_func     set_ignore_msg;
extern command_func     set_login_room;
extern command_func     set_plan;
extern command_func     set_pretitle;
extern command_func     set_prompt;
extern command_func     set_session;
extern command_func     set_term_width;
extern command_func     set_time_delay;
extern command_func     set_title;
extern command_func     set_word_wrap;
extern command_func     set_yes_session;
extern command_func	sfollowup;
extern command_func     shout;
extern command_func     show_exits;
#ifndef BROKEN_MSTATS
 extern command_func     show_malloc;
#endif
extern command_func	sitecheck;
extern command_func     sneeze;
extern command_func	snews_command;
extern command_func     soft_eject;
extern command_func     splat_player;
extern command_func     straight_home;
extern command_func     su;
extern command_func     su_hilited;
extern command_func     suemote;
extern command_func     super_help;
extern command_func     suthink;
extern command_func     swho;
extern command_func     sync_all_by_user;
extern command_func     sync_files;
extern command_func     tell_fn;
extern command_func     tell_friends;
extern command_func	tf_reply_block;
extern command_func	tfreply;
extern command_func     think;
extern command_func     toggle_anonymous;
extern command_func     toggle_iacga;
extern command_func	toggle_listing_of_tf;
extern command_func     toggle_mail_inform;
extern command_func     toggle_news_inform;
extern command_func     toggle_quiet_edit;
extern command_func	toggle_snews_inform;
extern command_func     toggle_tags;
extern command_func     trace;
extern command_func     trans_fn;
extern command_func     transfer_room;
extern command_func     twho;
extern command_func     unbanish;
extern command_func     unconverse;
extern command_func     unfrog;
extern command_func     unjail;
extern command_func     unlink_from_room;
extern command_func     unsplat;
extern command_func     validate_email;
extern command_func     view_check_commands;
extern command_func     view_commands;
extern command_func     view_ip;
extern command_func     view_list;
extern command_func     view_log;
extern command_func     view_mail_commands;
extern command_func     view_news_commands;
extern command_func     view_note;
extern command_func     view_others_list;
extern command_func     view_player_email;
extern command_func     view_received;
extern command_func     view_room_commands;
extern command_func     view_room_key_commands;
extern command_func     view_saved_lists;
extern command_func     view_sent;
extern command_func     view_session;
extern command_func	view_snews_commands;
extern command_func     view_time;
extern command_func     visit;
extern command_func     wake;
extern command_func     wall;
extern command_func     warn;
extern command_func     where;
extern command_func     whisper;
extern command_func     who;
extern command_func     with;
extern command_func     yoyo;

/* This is NOT to be removed, or you violate the terms of the license! */
extern command_func     summink_version;

/* colours plugin stuff */
#ifdef ANSI_COLS
 extern command_func	test_colours;
 extern command_func	toggle_colours;
 extern command_func	set_colours;
#endif

/* dynamic pfiles stuff */
#ifdef DYNAMIC
 extern command_func	view_dynamic_cache;
 extern command_func	sync_all_cache;
 extern command_func	nuke_cache;
 extern command_func	scan_players_for_caching;
#endif

/* socials */
#ifdef SOCIALS
 extern command_func	social_command;
#endif

/* robots */
#ifdef ROBOTS
 extern command_func	list_robots;
 extern command_func	store_robot;
 extern command_func	unstore_robot;
#endif

/* dummy commands for stack checks */

struct command  input_to = {"input_to fn", 0, 0, 0, 0, 0};
struct command  timer = {"timer fn", 0, 0, 0, 0, 0};

#ifdef INTERCOM
extern command_func add_intercom_server;
extern command_func bar_talker;
extern command_func close_intercom;
extern command_func delete_intercom_server;
extern command_func intercom_banish;
extern command_func intercom_banish_name;
extern command_func intercom_bar_name;
extern command_func intercom_change_address;
extern command_func intercom_change_alias;
extern command_func intercom_change_name;
extern command_func intercom_change_port;
extern command_func intercom_command;
extern command_func intercom_hide;
extern command_func intercom_locate_name;
extern command_func intercom_ping;
extern command_func intercom_reboot;
extern command_func intercom_request_stats;
extern command_func intercom_unbanish_name;
extern command_func intercom_site_move;
extern command_func intercom_slist;
extern command_func intercom_unbar_name;
extern command_func intercom_unhide;
extern command_func intercom_update_servers;
extern command_func intercom_version;
extern command_func list_intercom_servers;
extern command_func open_intercom;
extern command_func unbar_talker;
extern command_func view_intercom_commands;
#ifdef INTERCOM_EXT
extern command_func intercom_home;
#endif

struct command intercom_list[] = {
   {"add_server",add_intercom_server,LOWER_ADMIN, 0, 0, 0},
   {"announce_move",intercom_site_move,ADMIN, 0, 0, 0},
   {"banish", intercom_banish, SU,  0, 0, 0},
   {"banish_name", intercom_banish_name, SU,  0, 0, 0},
   {"bar_name", intercom_bar_name, SU,  0, 0, 0},
   {"bar", bar_talker, SU,  0, 0, 0},
   {"change_alias",intercom_change_alias,LOWER_ADMIN, 0, 0, 0},
   {"change_address",intercom_change_address,LOWER_ADMIN, 0, 0, 0},
   {"change_name",intercom_change_name,LOWER_ADMIN, 0, 0, 0},
   {"change_port",intercom_change_port,LOWER_ADMIN, 0, 0, 0},
   {"close",close_intercom,SU, 0, 0, 0},
   {"commands", view_intercom_commands, BASE,  0, 0, 0},
   {"delete_server",delete_intercom_server,LOWER_ADMIN, 0, 0, 0},
   {"hide", intercom_hide, SU,  0, 0, 0},
#ifdef INTERCOM_EXT
   {"home", intercom_home, 0,  0, 0, 0},
#endif
   {"list",list_intercom_servers,BASE, 0, 0, 0},
   {"locate",intercom_locate_name,BASE, 0, 0, 0},
   {"open",open_intercom,SU, 0, 0, 0},
   {"ping",intercom_ping,SU,  0, 0, 0},
   {"reboot",intercom_reboot,LOWER_ADMIN, 0, 0, 0},
   {"slist",intercom_slist,BASE, 0, 0, 0},
   {"stats",intercom_request_stats,SU, 0, 0, 0},
   {"unbanish_name", intercom_unbanish_name, SU,  0, 0, 0},
   {"unbar_name", intercom_unbar_name, SU,  0, 0, 0},
   {"unbar",unbar_talker,SU, 0, 0, 0},
   {"unhide", intercom_unhide, SU,  0, 0, 0},
   {"update_servers",intercom_update_servers,ADMIN, 0, 0, 0},
   {"version",intercom_version,BASE, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};


#endif

/* command list for editor */

struct command  editor_list[] = {
   {"del", edit_delete_line, 0, 0, 0, 0},
   {"-", edit_back_line, 0, 0, 0, 0},
   {"+", edit_forward_line, 0, 0, 0, 0},
   {"view", edit_view, 0, 0, 0, 0},
   {"l", edit_view_line, 0, 0, 0, 0},
   {"g", edit_goto_line, 0, 0, 0, 0},
   {"top", edit_goto_top, 0, 0, 0, 0},
   {"bot", edit_goto_bottom, 0, 0, 0, 0},
   {"end", edit_end, 0, 0, 0, 0},
   {"quit", edit_quit, 0, 0, 0, 0},
   {"wipe", edit_wipe, 0, 0, 0, 0},
   {"stats", edit_stats, 0, 0, 0, 0},
   {"quiet", toggle_quiet_edit, 0, 0, 0, 0},
   {"commands", edit_view_commands, 0, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};

/* command list for the room function */

struct command  keyroom_list[] = {
   {"check", check_rooms, BUILD, 0, 0, 0},
   {"commands", view_room_key_commands, BUILD, 0, 0, 0},
   {"end", exit_room_mode, 0, 0, 0, 0},
   {"entermsg", change_room_entermsg, BUILD, 0, 0, 0},
   {"exits", check_exits, BUILD, 0, 0, 0},
   {"+exit", add_exit, BUILD, 0, 0, 0},
   {"-exit", remove_exit, BUILD, 0, 0, 0},
   {"go", go_room, BUILD, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"info", check_room, BUILD, 0, 0, 0},
   {"lock", room_lock, BUILD, 0, 0, 0},
   {"lockable", room_lockable, BUILD, 0, 0, 0},
   {"look", look, BUILD, 0, 0, 0},
   {"linkable", room_linkable, BUILD, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {"name", change_room_name, BUILD, 0, 0, 0},
   {"open", room_open, BUILD, 0, 0, 0},
   {"trans", trans_fn, BUILD, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};


struct command  room_list[] = {
   {"bolt", room_bolt, BUILD, 0, 0, 0},
   {"edit", room_edit, BUILD, 0, 0, 0},
   {"sethome", set_home, BUILD, 0, 0, 0},
   {"lock", room_lock, BUILD, 0, 0, 0},
   {"lockable", room_lockable, BUILD, 0, 0, 0},
   {"linkable", room_linkable, BUILD, 0, 0, 0},
   {"open", room_open, BUILD, 0, 0, 0},
   {"entrance", room_entry, BUILD, 0, 0, 0},
   {"entermsg", change_room_entermsg, BUILD, 0, 0, 0},
   {"+exit", add_exit, BUILD, 0, 0, 0},
   {"-exit", remove_exit, BUILD, 0, 0, 0},
   {"link", room_link, BUILD, 0, 0, 0},
   {"exits", check_exits, BUILD, 0, 0, 0},
   {"id", change_room_id, BUILD, 0, 0, 0},
   {"name", change_room_name, BUILD, 0, 0, 0},
   {"notify", inform_room_enter, BUILD, 0, 0, 0},
   {"end", exit_room_mode, 0, 0, 0, 0},
   {"info", check_room, BUILD, 0, 0, 0},
   {"check", check_rooms, BUILD, 0, 0, 0},
   {"+auto", add_auto, BUILD, 0, 0, 0},
   {"-auto", remove_auto, BUILD, 0, 0, 0},
   {"speed", change_auto_base, BUILD, 0, 0, 0},
   {"autos", autos_com, BUILD, 0, 0, 0},
   {"delete", delete_room, BUILD, 0, 0, 0},
   {"create", create_new_room, BUILD, 0, 0, 0},
   {"go", go_room, BUILD, 0, 0, 0},
   {"look", look, BUILD, 0, 0, 0},
   {"trans", trans_fn, BUILD, 0, 0, 0},
   {"home", go_home, BUILD, 0, 0, 0},
   {"commands", view_room_commands, BUILD, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {"transfer", transfer_room, ADMIN, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};


/* command list for the check function */

struct command  check_list[] = {
   {"flags", player_flags_verbose, 0, 0, 0, 0},
   {"mail", view_received, MAIL, 0, 0, 0},
   {"sent", view_sent, MAIL, 0, 0, 0},
   {"news", list_news, 0, 0, 0, 0},
   {"exits", check_exits, 0, 0, 0, 0},
   {"entry", check_entry, 0, 0, 0, 0},
   {"list", check_alist, LIST, 0, 0, 0},
   {"autos", check_autos, BUILD, 0, 0, 0},
   {"room", check_room, 0, 0, 0, 0},
   {"rooms", check_rooms, BUILD, 0, 0, 0},
   {"email", check_email, BASE, 0, 0, 0},
   {"wrap", check_wrap, 0, 0, 0, 0},
   {"res_list", view_saved_lists, ADMIN, 0, 0, 0},
   {"updates", check_updates, (LOWER_ADMIN | ADMIN), 0, 0, 0},
   {"info", check_info, ADMIN, 0, 0, 0},
   {"commands", view_check_commands, 0, 0, 0, 0},
   {"ip", view_ip, (SU | ADMIN), 0, 0, 0},
   {"mails", view_player_email, ADMIN, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};

/* command list for the news sub command */

struct command  news_list[] = {
   {"check", list_news, 0, 0, 0, 0},
   {"read", read_article, 0, 0, 0, 0},
   {"view", list_news, 0, 0, 0, 0},
   {"reply", reply_article, MAIL, 0, 0, 0},
   {"areply", reply_article, MAIL, 0, 0, 0},
   {"post", post_news, MAIL, 0, 0, 0},
   {"apost", post_news, MAIL, 0, 0, 0},
   {"followup", followup, MAIL, 0, 0, 0},
   {"afollowup", followup, MAIL, 0, 0, 0},
   {"sfollowup", followuptosunews, SU, 0, 0, 0},
   {"remove", remove_article, MAIL, 0, 0, 0},
   {"commands", view_news_commands, 0, 0, 0, 0},
   {"inform", toggle_news_inform, 0, 0, 0, 0},
   {"end", exit_news_mode, 0, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};

/* command list for snews */
struct command	snews_list[] = {
   {"check", list_snews, MAIL, 0, 0, 0},
   {"end", exit_snews_mode, 0, 0, 0, 0},
   {"followup", sfollowup, MAIL, 0, 0, 0},
   {"inform", toggle_snews_inform, MAIL, 0, 0, 0},
   {"post", post_snews, MAIL, 0, 0, 0},
   {"read", read_sarticle, MAIL, 0, 0, 0},
   {"remove", remove_sarticle, MAIL, 0, 0, 0},
   {"reply", reply_sarticle, MAIL, 0, 0, 0},
   {"view", list_snews, MAIL, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"commands", view_snews_commands, MAIL, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};
/* command list for the mail sub command */

struct command  mail_list[] = {
   {"check", view_received, MAIL, 0, 0, 0},
   {"commands", view_mail_commands, MAIL, 0, 0, 0},
   {"end", exit_mail_mode, 0, 0, 0, 0},
   {"forward", forward_letter, MAIL, 0, 0, 0},
   {"aforward", forward_letter, MAIL, 0, 0, 0},
   {"inform", toggle_mail_inform, MAIL, 0, 0, 0},
   {"noanon", toggle_anonymous, MAIL, 0, 0, 0},
   {"post", send_letter, MAIL, 0, 0, 0},
   {"apost", send_letter, MAIL, 0, 0, 0},
   {"read", read_letter, MAIL, 0, 0, 0},
   {"reply", reply_letter, MAIL, 0, 0, 0},
   {"areply", reply_letter, MAIL, 0, 0, 0},
   {"delete", delete_received, MAIL, 0, 0, 0},
   {"remove", delete_sent, MAIL, 0, 0, 0},
   {"sent", view_sent, MAIL, 0, 0, 0},
   {"sread", read_sent, MAIL, 0, 0, 0},
   {"view", view_received, MAIL, 0, 0, 0},
   {"?", help, 0, 0, 0, cNO_SPACE},
   {"help", help, 0, 0, 0, 0},
   {"man", help, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0}
};


/* restricted command list for naughty peoples */

struct command  restricted_list[] = {
   {"'", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {"`", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {"\"", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {"::", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {":;", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {";;", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {";:", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {";", emote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {":", emote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {"=", whisper, 0, 0, 0, (cCOMMS|cNO_SPACE)},
   {"emote", emote, 0, 0, 0, cCOMMS},
   {"say", say, 0, 0, 0, cCOMMS},
   {"pemote", pemote, 0, 0, 0, cCOMMS},
   {"whisper", whisper, 0, 0, 0, cCOMMS},
   {"look", look, 0, 0, 0, cROOM},
   {"l", look, 0, 0, 0, cROOM},
   {"?", help, 0, 0, 0, (cINFO|cNO_SPACE)},
   {"help", help, 0, 0, 0, cINFO},
   {"man", help, 0, 0, 0, cINFO},
   {0, 0, 0, 0, 0, 0}
};

/* this is the main command list */

struct command  complete_list[] = {	/* non alphabetic */
{"'", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"`", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"\"", say, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"::", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{":;", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{";;", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{";:", pemote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{";", emote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{":", emote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{">", tell_fn, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{".", tell_fn, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"<:", premote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{",:", premote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"<;", premote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{",;", premote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"<", remote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{",", remote, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"=", whisper, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"!", shout, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"?", help, 0, 0, 0, (cINFO|cNO_SPACE)},
{"+", echo, ECHO_PRIV, 0, 0, (cCOMMS|cNO_SPACE)},
{"-", recho, ECHO_PRIV, 0, 0, (cCOMMS|cNO_SPACE)},
{"]", reply, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"[", ereply, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{"}", tfreply, LIST, 0, 0, (cCOMMS|cNO_SPACE)},
{"{", rfreply, LIST, 0, 0, (cCOMMS|cNO_SPACE)},
{"~", newthink, 0, 0, 0, (cCOMMS|cNO_SPACE)},
{0, 0, 0, 0, 0, 0},

{"abort", abort_shutdown, ADMIN, 0, 0, cADM},
{"age", set_age, 0, 0, 0, cCUST},	/* A */
{"assist", assist_player, SU, 0, 0, cSU},
{0, 0, 0, 0, 0, 0},

{"backup", backup_command, ADMIN, 0, 0, cADM},
{"ban", add_ban, ADMIN, 0, 0, cADM},
{"banish", banish_player, SU, 0, 0, cSU},
{"bar", bar, LIST, 0, 0, cLIST},	/* B */
{"barge", barge, ADMIN, 0, 0, cADM},
{"beep", beep, LIST, 0, 0, cLIST},
{"birthday", set_birthday, 0, 0, 0, cCUST},
{"blank_email", blank_email, ADMIN, 0, 0, cADM},
{"blank_own_list", blank_list, LIST, 0, 0, cLIST},
{"blank_prefix", blank_prefix, SU, 0, 0, cSU},
{"blankpass", new_blankpass, SU, 0, 0, cSU},
{"block", block, LIST, 0, 0, cLIST},
{"blockfriends", block_friends, 0, 0, 0, cSYS},
{"blocklisting", toggle_listing_of_tf, LIST, 0, 0, cSYS},
{"blockreplies", tf_reply_block, 0, 0, 0, cSYS},
{"blocktells", blocktells, 0, 0, 0, cSYS},
{"boot", boot_out, BUILD, 0, 0, (cROOM|cSU)},
#ifdef SOCIALS
{"bop", social_command, 0, 0, 0, cSOC},
#endif
{"bounce", bounce, 0, 0, 0, cMOVE},
{"bug", report_error, BASE, 0, 0, cMISC},
{0, 0, 0, 0, 0, 0},

{"check", check, 0, 0, 0, cINFO},	/* C */
{"chlim", change_limit, ADMIN, 0, 0, cADM},
{"clist", clear_list, LIST, 0, 0, cLIST},
{"cls", clear_screen, 0, 0, 0, cMISC},
{"colony", go_colony, 0, 0, 0, (cMOVE|cSU)},
#ifdef ANSI_COLS
{"colour", toggle_colours, 0, 0, 0, cSYS},
#endif
{"comfy", go_comfy, SU, 0, 0, (cMOVE|cSU)},
{"commands", view_commands, 0, 0, 0, cINFO},
{"comment", set_comment, 0, 0, 0, cMISC},
{"comments",comments, SESSION, 0, 0, cINFO},
{"confirm", confirm_password, PSU, 0, 0, cSU},
{"connect_room", set_login_room, BASE, 0, 0, cROOM},
{"converse", converse_mode_on, 0, 0, 0, cMISC},
{"cprompt", set_converse_prompt, 0, 0, 0, cCUST},
{"crash", crash, ADMIN, 0, 0, cADM},
{0, 0, 0, 0, 0, 0},

/* Stuff for dynamic rooms */
/* These seems to crash it by putting too much in the pager.
   {"dtb", dynamic_test_func_blocks, ADMIN, 0, 0, 0},
   {"dtk", dynamic_test_func_keys, ADMIN, 0, 0, 0},
   */
{"defrag", dynamic_defrag_rooms, ADMIN, 0, 0, cADM},
{"description", set_description, 0, 0, 0, cCUST},	/* D */
{"dfcheck", dynamic_validate_rooms, ADMIN, 0, 0, cADM},
{"dfstats", dynamic_dfstats, ADMIN, 0, 0, cADM},
{"drag", soft_eject, SU, 0, 0, cSU},
{"dump", dump_com, ADMIN, 0, 0, cADM},
{"dump_emails", dump_out_emails, ADMIN, 0, 0, cADM},
{0, 0, 0, 0, 0, 0},

{"earmuffs", earmuffs, 0, 0, 0, cSYS},
{"echo", echo, ECHO_PRIV, 0, 0, cCOMMS},
{"echoall", echoall, ADMIN, 0, 0, cADM},
{"emergency", emergency, 0, 0, 0, cMISC},
{"emote", emote, 0, 0, 0, cCOMMS},	/* E */
{"end", converse_mode_off, 0, 0, 0, cMISC},
{"entermsg", set_enter_msg, 0, 0, 0, cCUST},
{"ereply", ereply, 0, 0, 0, cCOMMS},
{"ereplytf", rfreply, LIST, 0, 0, cCOMMS},
{"email", change_email, BASE, 0, 0, cCUST},
{"etf", rfreply, LIST, 0, 0, cCOMMS},
{"examine", examine, 0, 0, 0, cINFO},
{"exclude", exclude, 0, 0, 0, cCOMMS},
{"exits", check_exits, 0, 0, 0, cROOM},
{0, 0, 0, 0, 0, 0},

{"find", listfind, LIST, 0, 0, cLIST},
{"finger", finger, 0, 0, 0, cINFO},
{"f", finger, 0, 0, 0, cINFO},
{"find_email", find_email, ADMIN, 0, 0, cADM},
{"flist", new_set_list, LIST, 0, 0, cLIST},
{"friend", friend, LIST, 0, 0, cLIST},	/* F */
{"friendblock", friendblock, LIST, 0, 0, cLIST},
{"frog", frog, SU, 0, 0, cSU},
{"fwho", qwho, LIST, 0, 0, cINFO},
{0, 0, 0, 0, 0, 0},

{"gender", gender, 0, 0, 0, cCUST},
{"ghome", straight_home, BUILD, 0, 0, cSYS},
{"go", go_room, 0, 0, 0, cMOVE},	/* G */
{"grab", do_grab, 0, 0, 0, cROOM},
{"grabable", grabable, 0, 0, 0, cINFO},
{"grabme", grab, LIST, 0, 0, cLIST},
{"grant", grant, ADMIN, 0, 0, cADM},
{"grep", grep_log, LOWER_ADMIN, 0, 0, cADM},
#ifdef SOCIALS
{"grin", social_command, 0, 0, 0, cSOC},
#endif
{0, 0, 0, 0, 0, 0},

{"help", help, 0, 0, 0, cINFO},	/* H */
{"hide", hide, 0, 0, 0, cSYS},
{"hitells", termtype, 0, 0, 0, cSYS},
{"home", go_home, BUILD, 0, 0, cMOVE},
{"homeview", homeview, BUILD, 0, 0, (cINFO|cROOM)},
{0, 0, 0, 0, 0, 0},

{"iacga", toggle_iacga, 0, 0, 0, cSYS},
{"idle", check_idle, 0, 0, 0, cINFO},	/* I */
{"idlemsg", set_idle_msg, 0, 0, 0, cCUST},
{"ignore", ignore, LIST, 0, 0, cLIST},
{"ignoremsg", set_ignore_msg, LIST, 0, 0, (cLIST|cCUST)},
{"inform", inform, LIST, 0, 0, cLIST},
#ifdef INTERCOM
{"intercom", intercom_command, BASE, 0, 0, cCOMMS},
#endif
{"invite", invite, LIST, 0, 0, cLIST},
{"invites", invites_list, 0, 0, 0, (cLIST|cINFO)},
{0, 0, 0, 0, 0, 0},

{"jail", prison_player, SU, 0, 0, cSU},
{"jetlag", set_time_delay, 0, 0, 0, cCUST},
{"join", join, 0, 0, 0, cMOVE},	/* J */
{0, 0, 0, 0, 0, 0},

{"key", key, LIST, 0, 0, cLIST},
{0, 0, 0, 0, 0, 0},		/* K */

{"l", look, 0, 0, 0, cROOM},
{"lag", add_lag, ADMIN, 0, 0, cADM},
{"leave", go_main, 0, 0, 0, cMOVE},
{"linewrap", set_term_width, 0, 0, 0, cCUST},
{"list", view_list, LIST, 0, 0, cLIST},
{"list_new", lnew, PSU, 0, 0, (cSU|cINFO)},
{"list_notes", list_notes, ADMIN, 0, 0, cADM},
{"list_res", view_saved_lists, SU, 0, 0, (cINFO|cSU)},
#ifdef ROBOTS
 {"list_robots", list_robots, LOWER_ADMIN, 0, 0, (cINFO|cADM)},
#endif
{"list_su", lsu, 0, 0, 0, cINFO},
{"lock", room_lock, 0, 0, 0, cROOM},
{"look", look, 0, 0, 0, cROOM},	/* L */
{"lsn", lnew, PSU, 0, 0, (cINFO|cSU)},
#ifdef ROBOTS
 {"lsr", list_robots, LOWER_ADMIN, 0, 0, (cINFO|cADM)},
#endif
{"lsu", lsu, 0, 0, 0, cINFO},
{0, 0, 0, 0, 0, 0},

{"mail", mail_command, MAIL, 0, 0, cMISC},	/* M */
{"mailblock", mailblock, MAIL, 0, 0, cLIST},
{"main", go_main, 0, 0, 0, cMOVE},
{"make", make_new_character, ADMIN, 0, 0, cADM},
#ifndef BROKEN_MSTATS
{"malloc", show_malloc, ADMIN, 0, 0, cADM},
#endif
{"man", help, 0, 0, 0, cINFO},
{"mode", mode, PSU, 0, 0, (cINFO|cSU)},
{"motd", motd, 0, 0, 0, cINFO},
{0, 0, 0, 0, 0, 0},

{"netstat", netstat, ADMIN, 0, 0, cADM},	/* N */
{"newbies", close_to_newbies, SU, 0, 0, cSU},
{"news", news_command, 0, 0, 0, cMISC},
{"noisy", noisy, LIST, 0, 0, cLIST},
{"noeprefix", ignoreemoteprefix, 0, 0, 0, cSYS},
{"nopager", nopager, 0, 0, 0, cSYS},
{"noprefix", ignoreprefix, 0, 0, 0, cSYS},
{"nuke", nuke_player, SU, 0, 0, cSU},
{0, 0, 0, 0, 0, 0},

{"off_duty", block_su, PSU, 0, 0, cSU},
{"on_duty", on_duty, PSU, 0, 0, cSU},
{0, 0, 0, 0, 0, 0},		/* O */

{"password", change_password, BASE, 0, 0, cCUST},	/* P */
{"pemote", pemote, 0, 0, 0, cCOMMS},
{"plan", set_plan, 0, 0, 0, cCUST},
#ifdef SOCIALS
{"ponder", social_command, 0, 0, 0, cSOC},
#endif
{"prefer", prefer, LIST, 0, 0, cLIST},
{"prefix", set_pretitle, BASE, 0, 0, cCUST},
{"premote", premote, 0, 0, 0, cCOMMS},
{"prompt", set_prompt, 0, 0, 0, cCUST},
{"privs", privs, BASE, 0, 0, cINFO},
{"pstats", player_stats, PSU, 0, 0, (cSU|cINFO)},
{"public", public_com, BASE, 0, 0, cSYS},
{"purge_list", purge_list, LIST, 0, 0, cLIST},
{0, 0, 0, 0, 0, 0},

{"quiet", go_quiet, 0, 0, 0, cSYS},
{"quit", quit, 0, 0, 0, cMISC},	/* Q */
{"qwho", qwho, 0, 0, 0, cINFO},
{0, 0, 0, 0, 0, 0},

{"re", repeat, 0, 0, 0, cCOMMS},
{"recap", recap, 0, 0, 0, cCUST},	/* R */
{"recho", recho, ECHO_PRIV, 0, 0, cCOMMS},
{"recount", recount_news, ADMIN, 0, 0, cADM},
{"relink", relink_note, ADMIN, 0, 0, cADM},
{"reload", reload, ADMIN, 0, 0, cADM},
{"remote", remote, 0, 0, 0, cCOMMS},
{"remove", remove_priv, ADMIN, 0, 0, cADM},
{"rename", rename_player, SU, 0, 0, cSU},
{"repeat", repeat, 0, 0, 0, cCOMMS},
{"reply", reply, 0, 0, 0, cCOMMS},
{"replytf", tfreply, LIST, 0, 0, cCOMMS},
{"reset_session", reset_session, SU, 0, 0, cSU},
{"reset_sneeze", reset_sneeze, SU, 0, 0, cSU},
{"resident", resident, SU, 0, 0, cSU},
{"resmail", resmail, ADMIN, 0, 0, cADM},
{"restore", restore_files, ADMIN, 0, 0, (cADM|cNO_MATCH)},
{"rf", remote_friends, LIST, 0, 0, cCOMMS},
{"rlist", new_set_list, LIST, 0, 0, cLIST},
{"rm_list", rm_list, 0, 0, 0, cLIST},
{"rm_move", remove_move, LOWER_ADMIN, 0, 0, cADM},
{"rm_note", dest_note, ADMIN, 0, 0, cADM},
{"rm_shout", remove_shout, SU, 0, 0, cSU},
{"room", room_command, BUILD, 0, 0, cROOM},
{"rt", remote_think, 0, 0, 0, cCOMMS},
{"rtf", tfreply, LIST, 0, 0, cCOMMS},
{0, 0, 0, 0, 0, 0},

{"save", do_save, BASE, 0, 0, cMISC},
{"say", say, 0, 0, 0, cCOMMS},	/* S */
#ifdef DYNAMIC
{"scan_cache", scan_players_for_caching, ADMIN, 0, 0, cADM},
#endif
{"script", script, SCRIPT, 0, 0, cMISC},
{"se", suemote, PSU, 0, 0, (cSU|cCOMMS)},
{"seeecho", see_echo, 0, 0, 0, cSYS},
{"seesess", view_session, 0, 0, 0, cINFO},
{"seetitle", set_yes_session, 0, 0, 0, cSYS},
{"session", set_session, SESSION, 0, 0, cMISC},
#ifdef ANSI_COLS
 {"setcolour", set_colours, 0, 0, 0, cSYS},
#endif
{"shelp", super_help, PSU, 0, 0, (cSU|cINFO)},
{"shout", shout, 0, 0, 0, cCOMMS},
{"show", toggle_tags, 0, 0, 0, cSYS},
{"showexits", show_exits, BASE, 0, 0, cSYS},
{"shutdown", pulldown, SU, 0, 0, (cSU|cNO_MATCH)},
{"site", same_site, (TRACE | SU), 0, 0, (cINFO|cSU)},
{"sitecheck", sitecheck, SU, 0, 0, (cINFO|cSU)},
{"slist", new_set_list, LIST, 0, 0, cLIST},
#ifdef SOCIALS
 {"smile", social_command, 0, 0, 0, cSOC},
#endif
{"sneeze", sneeze, SU, 0, 0, cSU},
{"snews", snews_command, SU, 0, 0, cSU},
{"splat", splat_player, SU, 0, 0, cSU},
{"st", suthink, PSU, 0, 0, (cSU|cCOMMS)},
#ifdef ROBOTS
 {"store", store_robot, ADMIN, 0, 0, cADM},
#endif
{"su", su, PSU, 0, 0, (cSU|cCOMMS)},
{"su_hi", su_hilited, PSU, 0, 0, (cSU|cSYS)},
{"su:", suemote, PSU, 0, 0, (cSU|cCOMMS)},
{"suggest", report_suggestion, 0, 0, 0, cMISC},
/* Do NOT remove summink_version from the command list, if you do so you
 * will be violating the terms of the License
 */
{"summink_version", summink_version, 0, 0, 0, cINFO},
{"swho", swho, 0, 0, 0, cINFO},
{"sync", sync_files, ADMIN, 0, 0, cADM},
{"sync_all", sync_all_by_user, ADMIN, 0, 0, cADM},
#ifdef DYNAMIC
 {"sync_cache", sync_all_cache, ADMIN, 0, 0, cADM},
#endif
{0, 0, 0, 0, 0, 0},

{"tell", tell_fn, 0, 0, 0, cCOMMS},	/* T */
{"termtype", termtype, 0, 0, 0, cSYS},
#ifdef ANSI_COLS
 {"testcolour", test_colours, 0, 0, 0, cMISC},
#endif
{"tf", tell_friends, LIST, 0, 0, cCOMMS},
{"think", newthink, 0, 0, 0, cCOMMS},
{"tlist", new_set_list, LIST, 0, 0, cLIST},
{"time", view_time, 0, 0, 0, cINFO},
{"title", set_title, 0, 0, 0, cCUST},
{"trace", trace, (TRACE | SU), 0, 0, (cSU|cINFO)},
{"trans", trans_fn, 0, 0, 0, cMOVE},
{"twho", twho, 0, 0, 0, cINFO},
{0, 0, 0, 0, 0, 0},

/* {"unlink",unlink_from_room,ADMIN,1,0, 0},            U     */
{"unban", remove_ban, ADMIN, 0, 0, cADM},
{"unbanish", unbanish, SU, 0, 0, cSU},
{"unconverse", unconverse, SU, 0, 0, cSU},
{"unfrog", unfrog, SU, 0, 0, cSU},
{"unjail", unjail, SU, 0, 0, cSU},
{"unsplat", unsplat, SU, 0, 0, cSU},
#ifdef ROBOTS
 {"unstore", unstore_robot, ADMIN, 0, 0, cADM},
#endif
{0, 0, 0, 0, 0, 0},

{"validate_email", validate_email, SU, 0, 0, cSU},
{"vbanish", banish_show, SU, 0, 0, (cSU|cINFO)},
#ifdef DYNAMIC
{"view_cache", view_dynamic_cache, ADMIN, 0, 0, cADM},
#endif
{"view_note", view_note, ADMIN, 0, 0, cADM},
{"visit", visit, 0, 0, 0, cMOVE},	/* V */
{"vlist", view_others_list, LOWER_ADMIN, 0, 0, (cLIST|cADM)},
{"vlog", view_log, (LOWER_ADMIN|ADMIN), 0, 0, cADM},
{0, 0, 0, 0, 0, 0},

{"wake", wake, 0, 0, 0, cMISC},
{"wall", wall, LOWER_ADMIN, 0, 0, cADM},
{"warn", warn, (WARN | SU ), 0, 0, (cSU|cMISC)},
{"where", where, 0, 0, 0, cINFO},
{"whisper", whisper, 0, 0, 0, cCOMMS},
{"who", who, 0, 0, 0, cINFO},	/* W */
#ifdef DYNAMIC
{"wipe_cache", nuke_cache, ADMIN, 0, 0, cADM},
#endif
{"with", with, 0, 0, 0, cINFO},
{"wordwrap", set_word_wrap, 0, 0, 0, cCUST},
{0, 0, 0, 0, 0, 0},

{"x", examine, 0, 0, 0, cINFO},	/* X */
{0, 0, 0, 0, 0, 0},

{"yoyo", yoyo, SU, 0, 0, cSU}, /* Y */
{0, 0, 0, 0, 0, 0},

{0, 0, 0, 0, 0, 0}		/* Z */
};

struct command *coms[27];
