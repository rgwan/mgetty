# frontends/www/Makefile
#
# RCS: $Id: Makefile,v 1.2 1998/09/01 15:09:31 gert Exp $
#
# $Log: Makefile,v $
# Revision 1.2  1998/09/01 15:09:31  gert
# faxhist.in : show fax history (from 'acct.log')
#
# Revision 1.1  1998/08/28 14:25:24  gert
# initial release
#

CGI=faxq.cgi faxq-v.cgi faxhist.cgi
HTML=index-d.html

CGI_HREF=/cgi-bin/

all: $(CGI) $(HTML)

clean:
	rm -f $(CGI)
	rm -f $(HTML)

fullclean: clean

install: all
	@echo "No automatic installation possible yet"

# explicit dependencies

faxq.cgi: faxq.in ../../sedscript
faxq-v.cgi: faxq-v.in ../../sedscript
index-d.html: index-d.in ../../sedscript Makefile

../../sedscript: ../../policy.h ../../Makefile ../../mksed.c
	@cd ../.. ; $(MAKE) sedscript

#
# inference rules for creating CGI binaries / HTML pages from *.in files
#
.SUFFIXES: .in .cgi .html

.in.cgi:
	../../sedscript <$< >$*.cgi
	chmod +x $*.cgi

.in.html:
	../../sedscript <$< | \
		sed -e 's!@CGI_HREF@!$(CGI_HREF)!' >$*.html

