#!/usr/bin/perl
#
# send SMS via atsms (etc)
#
# $Id: sms.cgi,v 1.3 2005/09/15 08:34:49 gert Exp $
#
# $Log: sms.cgi,v $
# Revision 1.3  2005/09/15 08:34:49  gert
# unbuffered output
#
# Revision 1.2  2005/09/13 13:47:27  gert
# fix CGI argument usage in command line ($phone, $text)
# "please wait..." message
#
# Revision 1.1  2005/09/13 13:30:56  gert
# Prototyp
#
#
use strict;

my $PIN="6062";
my $smscmd="/gnulocal/mgetty/tap/atsms -l /dev/tty3 -p $PIN";
my %args = ();
my $numberfile="/local/medat/handynr.txt";

# get variables and infos from CGI
sub get_cgi
{
    my $do_plus = shift;
    my $query_string;

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
    local $^W = 0;              # die undefs will ich nicht sehen
    %args=();
    foreach (split(/\&/,$query_string))
    {
       if ( /^(\w+)=/ && $' ne '' )
       {
          my ( $key, $value ) = ( $1, $' );
          if ($do_plus) {$value =~ s/\+/ /go;}
          $value =~ s/\%([0-9a-f]{2})/pack("C",hex($1))/eig;

          $args{"$key"}=$args{"$key"} . " " . $value if ($value!~/^\s*$/);
          $args{"$key"} =~ s/^\s*//o;
       }
    }

    return 1;
}
# end get_cgi



# main program
print <<EOF1;
Content-Type: text/html

<html>
<head><title>SMS-Frontend</title></head>
<body bgcolor="white">
<H1>SMS-Versand</H1>
EOF1

get_cgi(1);
my $do_send = 1;
my $err_msg = '';

my $phone='';
my $text='';

if ( !defined( $args{phone} ) && ! defined( $args{phone_l} ) )
    { $do_send=0; }
else
{
    $phone = defined( $args{phone_l} )? $args{phone_l}: $args{phone};
    if ( $phone !~ /^\+?[\d\s]*$/ )
    {
	$err_msg.="ung&uuml;ltige Zeichen in Telefonnummer (nur 0-9, Space, + erlaubt)<br>\n";
	$do_send=0;
    }
    $phone =~ s/\s//g;
}

if ( !defined( $args{text} ) )
    { $err_msg .= "Text-Feld leer<br>\n" if $do_send;
      $do_send=0; }
else
{
    $text=$args{text};
    $text =~ s/[\"\'\`]//g;
}

if ( !$do_send )
{
    if ( $err_msg ne '' )
    {
	print "<hr><font color=red size=+1>$err_msg</font><hr><p>\n";
    }
    print <<EOF_F1;
<form action="sms.cgi" method="GET">
<table>
<tr><td valign=top>Empf&auml;nger
    <td>
EOF_F1

    if ( open( NF, "<$numberfile" ) )
    {
	print "<select name=\"phone_l\">\n<option value=\"\" selected>--\n";
	while( <NF> )
	{
	    chomp;
	    my ( $number, $name ) = split( /;/ );
	    if ( $number eq $phone )
	    {
		print "<option selected value=\"$number\">$number $name\n";
		$phone='';
	    }
	    else
	    {
		print "<option value=\"$number\">$number $name\n";
	    }
	}
	print "</select><br>\n";
    }

    print <<EOF_F2
	<input type="text" name="phone" 
	 value="$phone" maxlength=20> (Direkteingabe)</tr>
<tr><td>Message
    <td><input type="text" name="text" 
	 value="$text" 
	 size=160 maxlength=160></tr>
<tr><td>&nbsp;<td><input type="submit" value="Abschicken"></tr>
</table>
</form>
EOF_F2

}
else
{
    $| = 1;
    print <<EOF2;
Versende SMS...<br>
Empf&auml;nger: $phone<br>
Text: $text<p>
Bitte einen Moment warten...<p>
<pre>
EOF2
    open (CMD, "$smscmd $phone '$text' 2>&1 |") ||
	die "can't run SMS command: $smscmd: $1</pre></body></html>\n";
    while( <CMD> ) { print $_; }
    close CMD;

print <<EOF3;
</pre>
SMS verschickt, Statuscode: $?<p>
EOF3
}

print <<EOF_E;
<hr>
Gert D&ouml;ring, 2005
</body>
</html>
EOF_E
