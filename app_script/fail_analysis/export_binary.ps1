Param($f_dps = ".\dps.txt", $f_bin=".\dump.txt.bin", $f_out, [int]$block, [int]$page=0, [int]$page_no=512)

#$raw_data = get-content $f_bin -encoding byte 

#$f_out = "data.bin"
# $fo = new-object io.filestream $f_out,createnew
[int]$end_page = $page + $page_no
[int]$copied = 0
write-host "search block:$block, from $page to $end_page"

foreach ($pp in (import-csv $f_dps) )
{
#	write-host "block:"$pp.FBlock" page:"$pp.FPage
	[int]$block_pp = "0x" + $pp.FBlock
	[int]$page_pp = "0x" + $pp.FPage
	if ( ($block -eq $block_pp) -and ($page_pp -ge $page) -and ($page_pp -lt $end_page) )
	{
		write-host "found block:$block_pp, page:" $page_pp" data:" ($pp.Data)
		if ($pp.Data -match "<#([0-9A-F]+);([0-9A-F]+)>")
		{	#
			[int]$offset = "0x" + $matches[1]
#			$offset *= 512
			[int]$len = "0x" + $matches[2]
#			$len *= 512
			write-host "copy data from $offset, len:$len" 
			& dd bs=512 count=$len if=$f_bin of=$f_out skip=$offset seek=$copied > null.txt 2>&1
			$copied += $len
#			$raw_data[$offset..($offset+$len-1)] | set-content -path $f_out
#			write-host "offset: $offset"
#			$ff = new-object io.filestream $f_bin,open;
#			$ff.seek($offset,0)
#			$ff.read($data, $offset, $count)
#			$fo.write($data, 0, $count)
		}
		else
		{
			write-host "warning: there is no data for block:$block page:"$pp.FPage
		}
	}
}