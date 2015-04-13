这个文档介绍解析cache info block的脚本：
1，脚本：
	sort_cache_info -f_dps <dps文件名> -start_id <启示ID>
	作用： 将id为0xFF的cache info page，id设置为该block的cache info id
			并且将id小于start_id的page +0x40绕回到最后。
			
2，用例：
	(1).\sort_cache_info.ps1 .\dump_1.txt -start_id 0x2900 | where-object { $_.FPage -eq "000"} | sort-
object hblock | export-csv cache-info-block.txt
		输出cache info block，并且按照生成顺序排序
	(2) .\sort_cache_info.ps1 -f_dps .\dump_1.txt -start_id 0x2900 | sort-object -property hblock，FPage
 | export-csv cache-info-page.txt
		输出所有的cache info page，并且按照生成顺序排序，用于LBA -> 物理地址解析

	(3) import-csv <dps> | where-object { ($_.ID -match "4.+") -and ($_.FPage -eq "000")} |
 sort-object -property sn,FPage | export-csv cache_blocks.txt
		输出cache block并且按照序列号排序
		
	(4) import-csv <dps> | where-object { ($_.ID -match "4.+")} |sort-object -property sn,FPage | export-csv .\dumpdata\cache-page.txt
		输出cache pages并且序列号排序
		
	(5) .\remove_orgbad.ps1 -f_dps <dps.txt> -f_orgbad <orgbad.txt> | export-csv .\dumpdata\dps.txt
		在dps中排出original bad block
