/*
 * missing_sun_headers.h
 *
 * Well - they got a little better for solaris at least.
 */
 
#ifdef SOLARIS
	extern int gethostname(char *, int);
	extern char *crypt(char *, char *);
#endif

#ifdef SUNOS
	extern char *crypt(char *, char *);
	extern char *sys_errlist[];
	extern int fclose(FILE *);
	extern void perror(char *);
	extern long strtol(char *, char **, int);
	extern int nice(int);
	extern int sscanf(char *, char *, ...);
	extern int ioctl(int, int, ...);
#ifdef VARARG_DEFINES
 		extern int vsprintf(char *, char *, va_list);
 		extern int vfprintf(FILE *, char *, va_list);
#endif
	extern int fwrite(const void *, size_t, size_t, FILE *);
	extern int printf(const char *, ...);
	extern int fprintf(FILE *, const char *, ...);
	extern int rename(const char *, const char *); 
	extern int toupper(char);
	extern int system(const char *);
	extern int tolower(char);
#ifdef RLIMIT_DEFINES
		extern int getrlimit(int, struct rlimit *);
		extern int setrlimit(int, struct rlimit *);
#endif
#ifdef TIME_DEFINES
		#include <sys/types.h>
		#include <sys/time.h>
		extern int setitimer(int, struct itimerval *, struct itimerval *);
		extern int getitimer(int, struct itimerval *);
		extern int strftime(char *, size_t, const char *, const struct tm *);
		extern time_t time(int);
#endif
#ifdef SOCKET_DEFINES
		extern int gethostname(char *, int);
		extern int setsockopt(int, int, int, char *, int);
		extern int listen(int, int);
		extern int accept(int, struct sockaddr *, int *);
		extern int bind(int, const struct sockaddr *, int);
		extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
		extern void bzero(char *, int);
		extern int connect(int, struct sockaddr *, int);
		extern int shutdown(int, int);
		extern int sigpause(int);
		extern int socket(int, int, int);
#endif
	extern int strcasecmp(char *, char *);
	extern int strncasecmp(char *, char *, int);
#endif /* SUNOS */

#ifdef LINUX
#ifdef GLIBC
		extern char *crypt(const char *, const char *);
#endif
#endif
