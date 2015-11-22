
--------------------------- Terminal Type ----------------------------
If you want hilighted text to indicate text aimed specifically at
you, then you will need to set a valid terminal type.  Also note,
if you want colour output, only the types 'xterm' and 'ansi' will
allow it.

vt100   : Base setting - most machines are OK with this.
ansi    : Many colour PC's, and some higher end terminals.
vt102   : Basically the same as vt100, only minor differences.
vt220   : Enhanced version of vt100, good for vt200, and vt320 also.
sun     : For a sun workstation.
xterm   : For someone running xwindows.
adm     : \
hp2392  : \\
wyse-30 : // You probably don't have one of these.
tvi912  : /

The default is vt100, which will work for most people, and you can
change the setting later with the 'termtype' command.
----------------------------------------------------------------------
