# create property

param($path_vld, $path_boost, $path_winddk)

$prop_dir = resolve-path "../application"
$prop_file = "$prop_dir/external_libs.vsprops"
if (test-path -path $prop_file)
{
	remove-item -force $prop_file
}

if (!($path_vld) )
{
	$path_vld = resolve-path "../extlib/vld"
}

$prop_list = @()
$prop_list += @{name="VLDDIR"; val=$path_vld}
$prop_list += @{name="BOOST_DIR"; val=$path_boost}
$prop_list += @{name="WINDDK"; val=$path_winddk}


'<?xml version="1.0"?>' >> $prop_file
"<VisualStudioPropertySheet"  >> $prop_file
"	ProjectType=""Visual C++"" " >> $prop_file
"	Version=""8.00"" "  >> $prop_file
"	Name=""external_libs"" "  >> $prop_file
"	>"  >> $prop_file

foreach ($p in $prop_list)
{
	if ($p.val)
	{
		"	<UserMacro" >> $prop_file
		"		Name=""" + $p.name + '"'	>> $prop_file
		"		Value=""" + $p.val + '"'	>> $prop_file
		"	/>" >> $prop_file
	}
}	
"</VisualStudioPropertySheet>"  >> $prop_file

