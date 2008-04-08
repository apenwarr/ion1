##
## Ion Makefile
##

# System-specific configuration is in system.mk
include ../system.mk

######################################

LIBS += -L../libtu -ltu -lm $(X11_LIBS) -lX11
#-lefence
INCLUDES += -I../libtu/include $(X11_INCLUDES)
DEFINES += -DETCDIR=\"$(ETCDIR)\"

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

OBJS=	binding.o client.o clientwin.o complete.o complete_file.o confws.o \
	cursor.o draw.o edln.o event.o exec.o focus.o font.o frame.o readfds.o \
	frameid.o function.o key.o listing.o main.o obj.o pointer.o \
	property.o query.o readconfig.o resize.o screen.o selection.o \
	signal.o sizehint.o split.o thing.o wedln.o window.o winprops.o \
	workspace.o xic.o input.o wmessage.o splitmisc.o modules.o \
	command.o shortcut.o grab.o

TARGETS=ion

######################################

include ../rules.mk

######################################

ion: $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ion $(BINDIR)
	$(STRIP) $(BINDIR)/ion
