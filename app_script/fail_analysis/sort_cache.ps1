param ($f_dps)
import-csv $f_dps | where-object { ($_.ID -match "4.+") -and ($_.FPage -eq "000")} |
 sort-object -property sn,FPage | export-csv cache_blocks.txt
#| 