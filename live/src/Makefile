# SensiSummink makefile - has lots of extra compile flags set to ensure
# as few bugs as possible during development and smoother running
# PLEASE NOTE!!!  If you change the defs etc in here, make sure you do the same 
# to the Makefile in the live directory!  Or it just won't work.

#
# COMPILER SELECTION
# If you want to use gcc, use this section
#
CC		= gcc
CFLAGS		= -ggdb -pedantic -Wall -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Winline -Wtraditional -Wnested-externs
LDFLAGS		= 
#
# On some systems, for example Irix - you get far better results from cc
# This has been tested and found to work cleanly on OSF1 versions also.
# Pity you won't get gdb debug output.  Oh, and sunos4 cc won't work.
#
#CC		= cc
#CFLAGS		=
#LDFLAGS	=


# LIBRARIES SECTION
#
# Linux - some broken versions require -lc if you get LOTS of errors!
LIBS	= -Wl,-qmagic
#
# Linux glibc (example redhat 5.x) - see above.
#LIBS	= -Wl,-qmagic -lcrypt
#
# OSF1 (Digital Unix) 4.0a-d
#LIBS	=
#
# Solaris 2.4/2.5/?
#LIBS	= -lmalloc -lsocket -lnsl
#
# SunOS
#LIBS	= 
#
# Irix 5.2/6.2/? - under gcc you may get malloc type warnings at compile time!
#LIBS	= -lmalloc
#
# FreeBSD
#LIBS	= -lcrypt -lmalloc
#
# FreeBSD 2.2.8
#LIBS	= -lcrypt

DEFS	    	= -DLIVE
SVERS		= 3.2b
BIN		= ../bin
TALKER_BIN      = summink
ANGEL_BIN       = angel
INTERCOM_BIN    = intercom
IDENT_BIN	= ident
COMMON_DEPENDS	= include/config.h include/player.h include/proto.h include/robot_proto.h include/robot_player.h Makefile include/missing_headers.h


OBJS        = admin.o \
              commands.o \
              dynamic_rooms.o \
              editor.o \
              globals.o \
              glue.o \
	      ip.o \
              lists.o \
              mail.o \
	      parse.o \
              plists.o \
              room.o \
              session.o \
              socket.o \
              tag.o 


all:            $(TALKER_BIN) $(ANGEL_BIN) $(IDENT_BIN) $(INTERCOM_BIN)
				@echo 'Made all'

ident.o:	ident.c include/player.h include/missing_headers.h include/ident.h include/config.h Makefile
		$(CC) $(CFLAGS) $(DEFS) -c ident.c -o ident.o
$(IDENT_BIN):	ident.o
		$(CC) $(LDFLAGS) -o $(IDENT_BIN) $(DEFS) ident.o $(LIBS)

intercom.o:	intercom.c include/intercom.h include/missing_headers.h include/config.h Makefile
		$(CC) $(CFLAGS) $(DEFS) -c intercom.c -o intercom.o
$(INTERCOM_BIN):	intercom.o
		$(CC) $(LDFLAGS) -o $(INTERCOM_BIN) $(DEFS) intercom.o $(LIBS)

angel.o:	angel.c include/config.h include/missing_headers.h Makefile
		$(CC) $(CFLAGS) $(DEFS) -c angel.c -o angel.o
$(ANGEL_BIN):	angel.o
		$(CC) $(LDFLAGS) -o $(ANGEL_BIN) $(DEFS) angel.o $(LIBS)

$(TALKER_BIN):	$(OBJS) Makefile
		$(CC) $(LDFLAGS) -o $(TALKER_BIN) $(DEFS) $(OBJS) $(LIBS)

clean:
	rm -f *.o $(TALKER_BIN) $(ANGEL_BIN) $(IDENT_BIN) $(INTERCOM_BIN)

vclean: clean
	rm -f ../logs/*.log ../logs/scripts/* ../logs/scripts/.??* \
		../logs/emergency/* ../logs/emergency/.??*

install.program:	$(TALKER_BIN)
	mv $(TALKER_BIN) $(BIN)
	chmod 700 $(BIN)/$(TALKER_BIN)
	@echo 'Made program install'

install.angel:	$(ANGEL_BIN)
	mv $(ANGEL_BIN) $(BIN)
	chmod 700 $(BIN)/$(ANGEL_BIN)
	@echo 'Made angel install'

install.intercom:	$(INTERCOM_BIN)
	mv $(INTERCOM_BIN) $(BIN)
	chmod 700 $(BIN)/$(INTERCOM_BIN)
	@echo 'Made intercom install'

install.ident:	$(IDENT_BIN)
	mv $(IDENT_BIN) $(BIN)
	chmod 700 $(BIN)/$(IDENT_BIN)
	@echo 'Made ident install'

install.all:     $(TALKER_BIN) $(ANGEL_BIN) $(IDENT_BIN) $(INTERCOM_BIN)
	mv $(TALKER_BIN) $(BIN)
	chmod 700 $(BIN)/$(TALKER_BIN)
	mv $(ANGEL_BIN) $(BIN)
	chmod 700 $(BIN)/$(ANGEL_BIN)
	mv $(IDENT_BIN) $(BIN)
	chmod 700 $(BIN)/$(IDENT_BIN)
	mv $(INTERCOM_BIN) $(BIN)
	chmod 700 $(BIN)/$(INTERCOM_BIN)
	@echo 'Made full install'
	@echo 'Remember to "make live" when you are happy with the new version.'

install:	install.all

program:        $(TALKER_BIN)
		@echo 'Made program'

admin.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c admin.c -o admin.o
commands.o:   	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c commands.c -o commands.o
dynamic_rooms.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c dynamic_rooms.c -o dynamic_rooms.o
editor.o:   	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c editor.c -o editor.o
globals.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c globals.c -o globals.o
glue.o:		$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c glue.c -o glue.o
ip.o:		$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c ip.c -o ip.o
lists.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c lists.c -o lists.o
mail.o:		$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c mail.c -o mail.o
parse.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c parse.c -o parse.o
plists.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c plists.c -o plists.o
room.o:		$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c room.c -o room.o
session.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c session.c -o session.o
socket.o:	$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c socket.c -o socket.o
tag.o:		$(COMMON_DEPENDS)
		$(CC) $(CFLAGS) $(DEFS) -c tag.c -o tag.o

# dependancies
glue.o: glue.c include/intercom.h include/crashrec_glue.c include/intercom_glue.c
socket.o: socket.c include/ident_socket.c include/ident.h include/ansi.h include/colour_socket.c include/nocolour_socket.c include/colours.h
globals.o: globals.c 
plists.o: plists.c include/normal_plists.c include/dynamic_plists.c include/ident.h include/robot_plists.c
parse.o: parse.c include/clist.h include/plugins.h
lists.o: lists.c 
commands.o: commands.c include/social_commands.c
admin.o: admin.c
room.o: room.c
mail.o: mail.c
tag.o: tag.c
malloc.o: malloc.c
editor.o: editor.c
session.o: session.c
dynamic_rooms.o: dynamic_rooms.c
ip.o: ip.c
