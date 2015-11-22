/*
 * file included into parse.c to give externs for functions
 */
 
#ifdef ANSI_COLS
 extern void ansi_cols_version(void);
#endif
#ifdef DYNAMIC
 extern void dynamic_version(void);
#endif
#ifdef SOCIALS
 extern void socials_version(void);
#endif
#ifdef IDENT
 extern void ident_version(void);
#endif
#ifdef INTERCOM
 extern void ver_intercom_version(void);
#endif
#ifdef CRASH_RECOVER
 extern void crashrec_version(void);
#endif
#ifdef ROBOTS
 extern void robot_version(void);
#endif

