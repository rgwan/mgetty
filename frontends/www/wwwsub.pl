#
# mgetty WWW gui common "things"
#
# RCS: $Id: wwwsub.pl,v 1.1 1998/10/22 12:54:49 gert Exp $
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

1;
