#Get-Content $args[0] | & ".\lba2file2.ps1" | sort-object start | format-table * -autosize > lba_map.txt
param(
	[string]$drive, 
	[int64]$offset = 0)
#get disk size

switch (get-wmiobject win32_logicaldisk)
{
	{$_.DeviceID -eq $drive} {$cap=$_.Size / 512}
}
#write-host "$drive size=$cap"	
& ".\nfi.exe" $drive | & ".\lba2file.ps1" -offset $offset -cap $cap | sort-object start | & ".\fill_unused.ps1" |
	format-table  start, len, fid, attr, fn -autosize 
	
#	| out-file lba_map3.txt -width 1000