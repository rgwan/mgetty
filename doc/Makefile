# doc/Makefile
#
# SCCS: $Id: Makefile,v 1.4 1994/09/14 11:06:20 gert Exp $ Copyright (C) 1994 Gert Doering
#
MAN1PAGES=g3topbm.1 g3cat.1 pbmtog3.1 \
	  faxspool.1 faxrunq.1 faxq.1 faxrm.1
MAN4PAGES=mgettydefs.4
MAN5PAGES=faxqueue.5
MAN8PAGES=sendfax.8 mgetty.8
MAN1DIR=/usr/local/man/man1
MAN4DIR=/usr/local/man/man4
MAN8DIR=/usr/local/man/man8
INFODIR=/usr/local/info

#
# formatted man pages
#
MAN_FMT_PRG=nroff -man

all:

doc-all: mgetty.ms mgetty.info mgetty.dvi mgetty.ps fmt-manpages

fmt-manpages:
	for i in $(MAN1PAGES) $(MAN4PAGES) $(MAN5PAGES) $(MAN8PAGES); do \
		echo $$i ; \
		$(MAN_FMT_PRG) $$i >`expr $$i : "\([^.]*\)"`.man ; \
	done

mgetty.ms: mgetty.texi
	-texi2roff -ms <mgetty.texi >mgetty.ms

mgetty.info: mgetty.texi
	-makeinfo mgetty.texi

mgetty.ps: mgetty.dvi
	dvips -o mgetty.ps mgetty.dvi

mgetty.dvi: mgetty.texi
	texi2dvi mgetty.texi

ps: mgetty.ps

clean:
	rm -f mgetty.cp mgetty.fn mgetty.ky mgetty.log mgetty.pg \
	      mgetty.toc mgetty.tp mgetty.vr

fullclean: clean
	rm -f mgetty.ps mgetty.dvi mgetty.aux mgetty.info* mgetty.ms

install: $(MAN1PAGES) $(MAN4PAGES) mgetty.info
	for i in $(MAN1PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN1DIR) ; \
	done
	for i in $(MAN4PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN4DIR) ; \
	done
	for i in $(MAN5PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN5DIR) ; \
	done
	-for i in $(MAN8PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN8DIR) ; \
	done
	for i in mgetty.info* ; do \
		$(INSTALL) -m 644 $$i $(INFODIR) ; \
	done

doc-tar: ../mgetty0$(VS)doc.tar.gz

../mgetty0$(VS)doc.tar.gz: doc-all
	gtar cvvf ../mgetty0$(VS)doc.tar mgetty.texi mgetty.info* \
				         mgetty.dvi mgetty.ps \
				         mgetty.ms *.man
	gzip -f -9 -v ../mgetty0$(VS)doc.tar
