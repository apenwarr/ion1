##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system.mk

######################################

SUBDIRS=libtu src
INSTALL_SUBDIRS=src

TARGETS=man/ion.1x

SCRIPTS=scripts/ion-edit scripts/ion-man scripts/ion-runinxterm \
	scripts/ion-ssh scripts/ion-view

ETC=	etc/bindings-default.conf etc/bindings-sun.conf etc/kludges.conf \
	etc/look-brownsteel.conf etc/look-greyviolet.conf \
	etc/look-simpleblue.conf etc/look-wheat.conf etc/sample.conf

DOCS=	README LICENSE ChangeLog doc/config.txt doc/functions.txt

######################################

include rules.mk

######################################

man/ion.1x: man/ion.1x.in
	sed 's#PREFIX#$(PREFIX)#g;  s#DOCDIR#$(DOCDIR)#g; s#ETCDIR#$(ETCDIR)#g' man/ion.1x.in > man/ion.1x

_install:
	$(INSTALLDIR) $(BINDIR)
	for i in $(SCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(BINDIR); \
	done

	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) man/ion.1x $(MANDIR)/man1

	$(INSTALLDIR) $(DOCDIR)/ion1
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR)/ion1; \
	done

	$(INSTALLDIR) $(ETCDIR)/ion
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR)/ion; \
	done

	@ if test -f $(ETCDIR)/ion/ion.conf ; then \
		echo "$(ETCDIR)/ion/ion.conf already exists. Not installing one."; \
	else \
		echo "Installing configuration file $(ETCDIR)/ion/ion.conf"; \
		if uname -s -p|grep "SunOS sparc" > /dev/null; then \
			sed s/bindings-default/bindings-sun/ \
			$(ETCDIR)/ion/sample.conf > $(ETCDIR)/ion/ion.conf; \
			chmod $(DATA_MODE) $(ETCDIR)/ion/ion.conf; \
		else \
			cp $(ETCDIR)/ion/sample.conf $(ETCDIR)/ion/ion.conf; \
		fi; \
	fi

