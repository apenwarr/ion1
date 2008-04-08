##
## System settings
##


##
## Installation paths
##

PREFIX=/usr/local

# No need to modify these usually
BINDIR=$(PREFIX)/bin
ETCDIR=$(PREFIX)/etc
MANDIR=$(PREFIX)/man
DOCDIR=$(PREFIX)/doc
# Not used
INCDIR=$(PREFIX)/include
LIBDIR=$(PREFIX)/lib


##
## Modules
##

#MODULE_SUPPORT=-DCF_NO_MODULE_SUPPORT
MODULE_SUPPORT_LDFLAGS=-export-dynamic -ldl


##
## X libraries, includes and options
##

X11_PREFIX=/usr/X11R6

# SunOS/Solaris
#X11_PREFIX=/usr/openwin

X11_LIBS=-L$(X11_PREFIX)/lib
X11_INCLUDES=-I$(X11_PREFIX)/include
X11_DEFINES=

#EXTRA_INCLUDES = -I$(PREFIX)/include
#EXTRA_LIBS = -L$(PREFIX)/lib


##
## libc
##

# You may uncomment this if you know your system has
# asprintf and vasprintf in the c library. (gnu libc has.)
# If HAS_SYSTEM_ASPRINTF is not defined, an implementation
# in libtu/sprintf_2.2/ is used.
#HAS_SYSTEM_ASPRINTF=1


##
## C compiler
##

CC=gcc

# The POSIX_SOURCE, XOPEN_SOURCE and WARN options should not be necessary,
# they're mainly for development use. So, if they cause trouble (not
# the ones that should be used on your system or the system is broken),
# just comment them out.

# libtu/ uses POSIX_SOURCE

POSIX_SOURCE=-ansi -D_POSIX_SOURCE

# and . (ion) XOPEN_SOURCE.
# There is variation among systems what should be used and how they interpret
# it so it is perhaps better not using anything at all.

# Most systems
#XOPEN_SOURCE=-ansi -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
# sunos, (irix)
#XOPEN_SOURCE=-ansi -D__EXTENSIONS__

# Same as '-Wall -pedantic-errors' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic-errors -Wuninitialized


CFLAGS=-g -O2 $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS=-g $(LIBS) $(EXTRA_LIBS)


##
## make depend
##

DEPEND_FILE=.depend
MAKE_DEPEND=$(CC) -M $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES) > $(DEPEND_FILE)


##
## AR
##

AR=ar
ARFLAGS=cr
RANLIB=ranlib


##
## Install & strip
##

# Should work almost everywhere
INSTALL=install
# On a system with pure BSD install, -c might be preferred
#INSTALL=install -c

INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=664

STRIP=strip
