# 根据参考路径，将绝对路径转换成相对路径
# 参数1 [in] base : 参考路径 （必须以绝对路的形式给出），
#					Win32中，必须和绝对路径使用相同volume
# 参数2 [in] abs : 需要转换的绝对路径
# 返回值 : 如果匹配成功，返回转换后的相对路径，否则返回绝对路径

sub PathAbs2Rel
{
	local ($base_head);

	my ($base, $abs) = @_;
	
	# 在所有的路径后面加上\\，所有相对路径都直接一字符开头，\\结尾。
	# 这样可以简化末端处理，并且防止路径名的部分匹配。
	$base = $base."\\";
	$abs = $abs."\\";
	local ($len) = length($abs);
	# 检查参考路径和绝对路径是否合法。
	return ($abs) if ($base !~ /^([a-zA-Z]:\\)/i);
	$base_head = $1;
	return (substr($abs, 0, $len-1)) if (index($abs, $base_head) != 0);
	$abs = substr($abs, length($base_head));
	$base = substr($base, length($base_head));

	# 查找并且出去参考路径和绝对路径头部的重复部分
	while ( ( $base ne "" ) && ($abs ne "") )
	{
		last if ($base !~ /([^\\]*\\).*/);
		$base_head = $1;

		last if (index($abs, $base_head) != 0);
		$abs = substr($abs, length($base_head));
		$base = substr($base, length($base_head));
	}
	
	# 如果参考路径还有剩余，计算退回的目录层数
	local($ii) = 0;	
	while ($base ne "")
	{
		$ii ++;
		$base =~ s/[^\\]*\\(.*)/$1/;
	}
	
	# 合成相对路径
	local($rel) = "";
	
	$rel = $rel."..\\"  while ($ii-- > 0); 
	$rel = $rel.$abs;
	# 删除添加到末尾的\\
	$len = length($rel);
	return (substr($rel, 0, $_len -1));
}

1;