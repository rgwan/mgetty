# frontends/www/Makefile
#
# RCS: $Id: Makefile,v 1.4 1998/10/07 13:05:31 gert Exp $
#
# $Log: Makefile,v $
# Revision 1.4  1998/10/07 13:05:31  gert
# added wwwgui.cfg.in/wwwgui.conf
#
# Revision 1.3  1998/09/11 13:56:57  gert
# renamed "givgif" to "nph-vf-gif" (name too long, nph- needed)
#
# Revision 1.2  1998/09/01 15:09:31  gert
# faxhist.in : show fax history (from 'acct.log')
#
# Revision 1.1  1998/08/28 14:25:24  gert
# initial release
#

CGI=faxq.cgi faxq-v.cgi faxhist.cgi viewfax.cgi nph-vf-gif.cgi
HTML=index-d.html

CGI_HREF=/cgi-bin/

all: $(CGI) $(HTML) wwwgui.conf

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
faxhist.cgi: faxhist.in ../../sedscript
viewfax.cgi: viewfax.in ../../sedscript
nph-vf-gif.cgi: nph-vf-gif.in ../../sedscript

wwwgui.conf: wwwgui.cfg.in ../../sedscript
	../../sedscript <wwwgui.cfg.in >wwwgui.conf

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

