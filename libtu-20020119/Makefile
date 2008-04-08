##
## libtu Makefile
##

# System-specific configuration is in system.mk
include system.mk

######################################

INCLUDES += -I./include
CFLAGS += $(POSIX_SOURCE)

OBJS= misc.o output.o util.o optparser.o parser.o tokenizer.o map.o

ifdef LIBTU_NO_ERRMSG
DEFINES += -DLIBTU_NO_ERRMSG
else
ifndef HAS_SYSTEM_ASPRINTF
OBJS += snprintf_2.2/snprintf.o
else
DEFINES += -DHAS_SYSTEM_ASPRINTF
endif
endif

TARGETS=libtu.a
TESTERS=tester tester2 tester3

######################################

include rules.mk

######################################

testers: $(TESTERS)

libtu.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@
	
tester: tester.c libtu.a
	$(CC) $(CFLAGS) $< -L. -ltu -lm -o $@

tester2: tester2.c libtu.a
	$(CC) $(CFLAGS) $< -L. -ltu -lm -o $@

tester3: tester3.c libtu.a
	$(CC) $(CFLAGS) $< -L. -ltu -lm -o $@

_install:
	$(INSTALLDIR) $(LIBDIR)
	$(INSTALLDIR) $(INCDIR)/libtu
	$(INSTALL) -m $(DATA_MODE) libtu.a $(LIBDIR)
	for i in include/libtu/*.h; do \
		$(INSTALL) -m $(DATA_MODE) $$i $(INCDIR)/libtu; \
	done
