#
# mgetty WWW gui common "things"
#
# RCS: $Id: wwwsub.pl,v 1.2 1998/11/20 10:06:47 gert Exp $
#
# $Log

sub errormessage
{
    my $message = shift;
    print <<EOF;
<html><head><title>Configuration Error</title></head><body bgcolor="#ffffff">
<h1><p><b><blink>Configuration Error</blink> - - $message</b></p></h1>
</body></html>
EOF
    exit 1;
}

sub check_outgoing
{
    # check, if fax-outgoing is defined
    if ($fax_outgoing eq "")
    {
	errormessage("\$fax_outgoing : not configured - please look at the configuration-Files and configure the Directory where you store your outgoing faxes (\$fax_outgoing)");
    }
    if (! -d $fax_outgoing)
    { errormessage("\$fax_outgoing : no such directory $fax_outgoing");}
    if (! -r $fax_outgoing)
    { errormessage("\$fax_outgoing : no read-permission for $fax_outgoing");}
}

1;
