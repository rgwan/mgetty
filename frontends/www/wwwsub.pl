#
# mgetty WWW gui common "things"
#
# RCS: $Id: wwwsub.pl,v 1.3 1998/11/20 16:38:49 gert Exp $
#
# $Log

# common HTML error handler - print error message, end program
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
# end errormessage


#
# all necessary checks for $fax_outgoing
#
sub check_outgoing
{

    # check, if fax-outgoing is defined
    if ($fax_outgoing eq "")
    {
	errormessage("\$fax_outgoing : not configured - please look at the configuration-Files and configure the Directory where you store your outgoing faxes (\$fax_outgoing)");
    }

    # check, if fax-outgoing exists
    if (! -d $fax_outgoing)
    { errormessage("\$fax_outgoing : no such directory $fax_outgoing");}

    # check, if fax_outgoing has read-permissions
    if (! -r $fax_outgoing)
    { errormessage("\$fax_outgoing : no read-permission for $fax_outgoing (running with UID: $<)");}
}
# end check_outgoing


# get variables and infos from CGI
sub get_cgi
{
    my $do_plus = shift;

    ### GET or POST?
    if ($ENV{'REQUEST_METHOD'} eq "GET")
    {
       $query_string=$ENV{'QUERY_STRING'};
    }
    elsif (($ENV{'REQUEST_METHOD'} eq "POST") &&
	  ($ENV{'CONTENT_TYPE'} eq "application/x-www-form-urlencoded"))
    {
       read(STDIN,$query_string,$ENV{'CONTENT_LENGTH'});
    }
    else
    {
        $query_string="";
	return 0;
    }

    ### parse arguments
    %args=();
    foreach (split(/\&/,$query_string))
    {
       if ( /^(\w+)=/ && ($key=$1) && ($value=$') )
       {
	  if ($do_plus) {$value =~ s/\+/ /go;}
	  $value =~ s/\%([0-9a-f]{2})/pack(C,hex($1))/eig;
     
	  $args{"$key"}=$args{"$key"} . " " . $value if ($value!~/^\s*$/);
	  $args{"$key"} =~ s/^\s*//o;
       }
    }

    return 1;
}
# end get_cgi

1;
