# doc/Makefile
#
# SCCS: $Id: Makefile,v 1.8 1994/11/04 14:35:03 gert Exp $ Copyright (C) 1994 Gert Doering
#

NROFF=nroff
#
#
MAN1PAGES=g3topbm.1 g3cat.1 pbmtog3.1 fax.1 \
	  faxspool.1 faxrunq.1 faxq.1 faxrm.1 coverpg.1
MAN4PAGES=mgettydefs.4
MAN5PAGES=faxqueue.5
MAN8PAGES=sendfax.8 mgetty.8
MANSRC=$(MAN1PAGES) $(MAN4PAGES) $(MAN5PAGES) $(MAN8PAGES)
MAN1DIR=/usr/local/man/man1
MAN4DIR=/usr/local/man/man4
MAN5DIR=/usr/local/man/man5
MAN8DIR=/usr/local/man/man8
INFODIR=/usr/local/info

#
# formatted man pages
#
MAN_FMT_PRG=$(NROFF) -man


all:

doc-all: mgetty.asc mgetty.info mgetty.dvi mgetty.ps fmt-manpages

manpages: $(MANSRC)

fmt-manpages:
	$(MAKE) `for i in $(MANSRC) ; do echo \`expr $$i : "\([^.]*\)"\`.man ; done `

mgetty.asc: mgetty.ms
	-$(NROFF) -ms mgetty.ms >mgetty.asc

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
	rm -f mgetty.ps mgetty.dvi mgetty.aux mgetty.info* mgetty.ms \
	      *.man *.[1-9]

install: $(MANSRC) mgetty.info
	-mkdir $(MAN1DIR) $(MAN4DIR) $(MAN5DIR) $(MAN8DIR)
	for i in $(MAN1PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN1DIR) ; \
	done
	for i in $(MAN4PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN4DIR) ; \
	done
	for i in $(MAN5PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN5DIR) ; \
	done
	for i in $(MAN8PAGES) ; do \
		$(INSTALL) -m 644 $$i $(MAN8DIR) ; \
	done
	if [ -f mgetty.info ] ; then\
	    for i in mgetty.info* ; do \
		$(INSTALL) -m 644 $$i $(INFODIR) ; \
	    done ; \
	else \
	    echo "mgetty.info not made, skipping" ; \
	fi

doc-tar: ../mgetty0$(VS)doc.tar.gz

../mgetty0$(VS)doc.tar.gz: doc-all
	gtar cvvf ../mgetty0$(VS)doc.tar mgetty.texi mgetty.info* \
				         mgetty.dvi mgetty.ps \
				         mgetty.ms mgetty.asc *.man
	gzip -f -9 -v ../mgetty0$(VS)doc.tar

# explicit man page dependencies

faxq.1: faxq.1in ../sedscript
faxqueue.5: faxqueue.5in ../sedscript
faxrm.1: faxrm.1in ../sedscript
faxrunq.1: faxrunq.1in ../sedscript
faxspool.1: faxspool.1in ../sedscript
coverpg.1: coverpg.1in ../sedscript
g3cat.1: g3cat.1in ../sedscript
g3topbm.1: g3topbm.1in ../sedscript
mgetty.8: mgetty.8in ../sedscript
mgettydefs.4: mgettydefs.4in ../sedscript
pbmtog3.1: pbmtog3.1in ../sedscript
sendfax.8: sendfax.8in ../sedscript

../sedscript: ../policy.h ../Makefile ../mksed.c
	@cd .. ; $(MAKE) sedscript

#
# inference rules for creating manpages from .in pages
#
.SUFFIXES: .1in .4in .5in .8in

.1in.1:
	../sedscript <$< >$*.1

.4in.4:
	../sedscript <$< >$*.4

.5in.5:
	../sedscript <$< >$*.5

.8in.8:
	../sedscript <$< >$*.8

#
# inference rules for building formatted manpages
#
.SUFFIXES: .1 .4 .5 .8 .man

.1.man:
	$(MAN_FMT_PRG) $< >$*.man

.4.man:
	$(MAN_FMT_PRG) $< >$*.man

.5.man:
	$(MAN_FMT_PRG) $< >$*.man

.8.man:
	$(MAN_FMT_PRG) $< >$*.man

