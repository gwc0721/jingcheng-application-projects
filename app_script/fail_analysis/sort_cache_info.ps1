param ($f_dps, [int]$start_id = 0)

function SetCacheInfoId
{
	begin
	{
		[int]$cur_fblock = 0
		$cur_id = "0xFF00"
		$buffer = @()
	}
	process
	{
		[int]$fblock = "0x"+$_.FBlock
		[int]$cache_info_id = "0x" + $_.hblock
		if ($cache_info_id -lt $start_id)
		{
			$_.hblock = "{0:X}" -f ($cache_info_id + "0x4000")
		}
		
		if ( $cache_info_id -eq "0xFF00")
		{	# unknow cache info id
			if ($fblock -eq $cur_fblock)
			{	# set known cache info id
				$_.hblock = $cur_id
				$_
			}
			else
			{	#push to buffer
				$buffer += $_
			}
		}
		else
		{
			if ($fblock -ne $cur_fblock)
			{	# set id for f-block
				$cur_fblock = $fblock
				$cur_id = $_.hblock
				# output buffer
				foreach ($x in $buffer)
				{
					$x.hblock = $cur_id
					$x
				}
				$buffer = @()
			}
			$_
		}
	}
	end
	{
	}
}


import-csv $f_dps | where-object { ($_.ID -eq "E8") } | SetCacheInfoId

# export-csv cache-info-id.txt
# SetCacheInfoId | sort-object hblock | export-csv cache_info.txt


