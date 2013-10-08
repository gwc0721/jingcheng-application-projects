
while ( $line = <> )
{
	if ( $line =~ /block size\s*= (\d*)/ )
	{
		$block_size = $1 / 2;
	}
	elsif ( $line =~ /\(delta_tick =(\d*)\)/ )
	{
		print "$1, $block_size\r";
		$block_size = 0;
	}
}