/* ansi.h */
#ifndef _ANSI_H_
#define _ANSI_H_

#define ESC     "\033"
#define CSI     ESC"["

/* Note, these are Esc codes for VT100 terminals, and emmulators */
/*       and they may not all work within the talker               */

#define WRAP ESC"[?7h"         /* Set autowrap */
#define CLR ESC"[2J"           /* Clear the screen  */
#define HOME ESC"[H"           /* Send cursor to home position */
#define REF CLR HOME            /* Clear screen and home cursor */
#define REVINDEX ESC"M"        /* Scroll screen in opposite direction */
#define SINGW ESC"#5"          /* Normal, single-width characters */
#define DBL ESC"#6"            /* Creates double-width characters */
#define FRTOP ESC"[2;25r"      /* Freeze top line */
#define FRBOT ESC"[1;24r"      /* Freeze bottom line */
#define UNFR ESC"[r"           /* Unfreeze top and bottom lines */
#define U ESC"[4m"             /* Initialize underscore mode */
#define REV ESC"[7m"           /* Turns reverse video mode on */
#define HIREV ESC"[1,7m"       /* Hi intensity reverse video  */
#define FIX     ESC"c"         /* Repair character set */
#define ERASEL  ESC"[1K"        /* Erase entire line */


#endif /* _ANSI_H_ */

