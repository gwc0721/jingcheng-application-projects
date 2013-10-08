
# 这个脚本将emule地址中的以%xx形式表示的非ascii字符转换成字符。
# 在URL中，常用%xx表示字符串中的非ascii（> 0x7F）字符，例如中文，UTF8等。
# "ed2k://|file|%5BBBC.%E4%B8%98%E5%90%89%E5%B0%94%E7%9A%84%E4%BF%9D%E9%95%96%5D.BBC...." ;
# 表示 "ed2k://|file|[BBC.丘吉尔的保镖].BBC.Churchills...."
# 本脚本用于还原上述emule地址。
# 脚本中的if段其实是一个通用的转换程序

while ( $line = <> )
{
	if ( $line =~ /ed2k:\/\/\|file\|([^\|]*)/ )
	{
		$remain = $1;
		$title = "";
		while ( $remain )
		{
			if ($remain =~ /([^%]*)(.*)/ )
			{
				# pickup for non transcode substrings
				$title = $title.$1;
				$remain = $2;
			}
			if ($remain =~ /\%([0-9A-Z]{2})(.*)/)
			{
				# transcode one byte (%xx)
				$title = $title.chr(hex($1));
				$remain = $2;
			}
		}
		print "$title\r";
		print "$line\r\r";
	}
}