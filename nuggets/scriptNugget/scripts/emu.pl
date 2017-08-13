#!/usr/bin/perl

my $nugget_id = "cf52e719-91de-456c-be4c-9c758f4389ca";
my $application_type = "2d96bc81-1171-447f-9b95-4fc155aaa528";
my $data_type = "SHELL_CODE";


if ($ARGV[0] ne '--register') {
    my $output = `/razorback/bin/sctest -vv -s -1 -S < $ARGV[0]`;
	
	my $str1 = $output;
    $str1 =~ s/.*stepcount//sm;
	$str1 =~ s/.*\x0a//;
    $str1 =~ s/^\x5bemu[^\x5d]*\x5d/\x5bDEBUG\x5d/mg;

	my $str2 = $output;
	$str2 =~ s/stepcount.*//sm;
	$str2 =~ s/^[^\x5b].*$//mg;
	$str2 =~ s/^\s*$//mg;
    $str2 =~ s/^\x5bemu[^\x5d]*\x5d/\x5bDEBUG\x5d/mg;

	
	if ($str1) {
		#create report
	
        if ($str2) {
			$str2 = "<entry><type>REPORT</type><data><![CDATA[\"$str2\"]]></data></entry>";
		}

		print "<?xml version=\"1.0\"?><razorback>".
		      "<response>".
			  "<log level=\"1\"><message>Analyzing $ARGV[0]</message></log>".
			  "<verdict priority=\"1\" gid=\"7\" sid=\"1\">".
			  "<flags><sourcefire><set>2</set><unset>1</unset></sourcefire>".
			  "<enterprise><set>0</set><unset>0</unset></enterprise></flags>".
			  "<message>Possible shellcode found.</message>".
			  "<metadata><entry>".
			  "<type>REPORT</type>".
			  "<data><![CDATA[\"$str1\"]]></data>".
			  "</entry>".
			  $str2.
			  "</metadata>".
		      "</verdict></response></razorback>";
	}
	else {
		#don't create report
        print "<?xml version=\"1.0\"?><razorback></razorback>";
	}

}
else {
    print "<?xml version=\"1.0\"?><razorback><registration>" .
          "<nugget_id>$nugget_id</nugget_id>" .
		  "<application_type>$application_type</application_type>" .
		  "<data_types>".
		  "<data_type>$data_type</data_type>" .
          "</data_types>".
		  "</registration></razorback>";
}
