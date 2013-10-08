require ("PathAbs2Rel.pl");

$base0 = "";
$abs0 = "";
$expect = "";

while ( $line = <> )
{
	chomp($line);
	next if ( ($line eq "") || ($line =~ /^#/));
	
	if ( $base0 eq "")
	{
		$base0 = $line;
	}
	elsif (	$abs0 eq "")
	{
		$abs0 = $line;
	}
	elsif ( $expect eq "")
	{
		$expect = $line;
		print "Start testing...\r\n";
		print "base = $base0\r\n";
		print "abs = $abs0\r\n";
		$rel = &PathAbs2Rel($base0, $abs0);
		print "rel = $rel\r\n";
		print "exp = $expect\r\n\r\n";
		
		$base0 = "";
		$abs0 = "";
		$expect = "";
	}
}
