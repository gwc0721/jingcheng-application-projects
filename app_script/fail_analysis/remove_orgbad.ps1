# f_orgbad: input file name of original bad block list
# f_dps: input file name of dumpped list
# f_empty: input file name of empty block list

param ($f_orgbad=".\orgbad.txt", $f_dps=".\dump.txt", $f_empty=".\emptyblock.txt")


function LoadOrgBad
{
	begin
	{
	}
	process
	{
		$_.Fblock
	}
	end
	{
	}
}

function RemoveOrg
{
	begin
	{
		$empty_index = 0
		$pre_blk = 0
		[int]$cur_blk = 0
#		write-host $orgbad
	}
	process
	{
		[int]$bb = "0x" + $_.FBlock
		if ($bb -ne $cur_blk)
		{
#			write-host "change blocks to $bb"
			$pre_blk = $cur_blk
			$cur_blk = $bb
			
			while ( 1 )
			{
				$bb = "0x" + $empty[$empty_index].FBlock
#				write-host "current: $cur_blk, empty: $bb" 
				if( ($bb -gt $pre_blk) -and ($bb -lt $cur_blk) )
				{
					write-host "insert empty block $bb between $pre_blk - $cur_blk"
					$empty[$empty_index]
					$empty_index++
				}
				else
				{
					break
				}
			}
			
		}
		
		if ($orgbad -contains $_.FBlock)
		{
			write-host "find org bad:block=" $_.FBlock "page=" $_.FPage
			$_.ID = 0
		}
		$_
	}
	end
	{
	}
}

if (test-path -path $f_empty)
{
	write-host "find empty blocks"
	$empty = (import-csv $f_empty)
#	write-host $empty
}
else
{
	write-host "empty block does not exist"
}

$orgbad = (import-csv $f_orgbad | LoadOrgBad)
import-csv $f_dps | RemoveOrg 



