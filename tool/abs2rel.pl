# 代码管理工具
# 将.vcproj文件中的绝对路径变换成以工程文件目录为基准的相对路径。
# 变换前的文件以.bak作为备份。
# 在promote .vcproj文件以前，必须执行该步骤。
# 用法: abs2rel.pl <workspace>
# 参数: <workspace> workspace所在的目录。转换再次目录及子目录下的所有工程文件     

use Cwd;
use File::Find;
require ("PathAbs2Rel.pl");

$cwd = $ARGV[0];
print "cwd = $cwd \r\n";
find(\&OnFileFind, $cwd);

sub OnFileFind
{
	$filename = $File::Find::name;
	if (-f $filename)
	{
#		return 

#		$pos = index ($filename, "extlib/");
	#	print "file = $filename pos = $pos\r\n";
		
		if ( index ($filename, "extlib/") >= 0 )
		{
#			print "$filename is excluded \r\n";
			return;
		}
		if ( 	($filename =~ /\.vcproj$/) )
#				|| ($filename =~ /\.sln$/) )
		{
#			print "$File::Find::name \r\n";
			# 使用File::Find得到的文件名可能包含\或者/
			$filename =~ s/\//\\/g;
			print "file = $filename\r\n";
			&ConvertFile($filename);
		}
	}
}

sub ConvertFile
{
	my ($filename ) = @_;
	
	# backup file
#	print "file = $filename\r\n";
	
	# 从文件名中提取目录名作为转换的base
	local ($dir_len) = rindex($filename, "\\");
	return if ($dir_len <= 0);
	local($dir) = substr($filename, 0,  $dir_len);
#	print "dir = $dir \r\n";
	$filename_back = $filename.".bak";
	
	rename($filename, $filename_back);
	open(DstFile, ">$filename");
	open(SrcFile, $filename_back);
	
	while ($line = <SrcFile>)
	{
		chomp($line);
		next if ($line eq "");
		if ( ($line =~ s/(\s*AdditionalIncludeDirectories=")(.*)/$2/)
			|| ($line =~ s/(\s*AdditionalLibraryDirectories=")(.*)/$2/) )
		{
			print DstFile ($1);
			print "line = $line\r\n";
			while ( $line =~ s/([^;]*)[;|"](.*)/$2/ )
			{
				next if ($1 eq "");
				$path = $1;
				# check if $path is a absolute path
				local($rel_path) = PathAbs2Rel($dir, $path);
				if ( $path != $rel_path)
				{
					print "$path => $rel_path\r\n";
				}
				print DstFile ("$rel_path;");
			}
			print DstFile ("\"$line\r\n");
			
		}
		else 
		{
			print DstFile ("$line\r\n");
		}
	}
	close(SrcFile);
	close(DstFile);
}

