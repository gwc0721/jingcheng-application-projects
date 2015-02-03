# parameters:
# 	%1: SRC | LIB, select pack source or lib only
param($SRC="LIB")
$package_name = "sn_update_tool.source"
if ( test-path -path $package_name )
{
	write-host "remove folder $tar_dir"
	remove-item -force -recurse -path $package_name
	remove-item -force sn_update_tool.rar
}
md $package_name > null

$ws_dir = resolve-path "..\.."
"ws dir = $ws_dir" #<DEBUG>
$tar_dir = resolve-path $package_name
"tar dir = $tar_dir"	#<DEBUG>
$tool_dir = resolve-path "..\..\tool"
"tool dir = $tool_dir"		#<DEBUG>
$app_dir = "application\sn_update_tool"

#src_dir使用相对于workspace的目录
function PackLib($src_dir, $lib_name, $SRC="LIB")
{
	write-host "PackLib, src=$src_dir, lib=$lib_name"	#<DEBUG>
	# enter lib directory
	$src_abs = "$ws_dir\$src_dir"
	"src = $src_abs"		#<DEBUG>
	$dst_abs = "$tar_dir\$src_dir"
	"dst = $dst_abs"		#<DEBUG>
	if ( !(test-path -path $dst_abs) )	{ md $dst_abs > null }
	push-location 
	cd $src_abs"\"$lib_name
	& "$tool_dir\package\pack_lib.ps1" $SRC $lib_name
	mv ".\$lib_name.package" "$dst_abs\$lib_name"
	pop-location
}

md "$tar_dir\$app_dir" > null

# create package of FerriSdk.package
PackLib "jcvos" "stdext" 
PackLib "$app_dir" "FerriSdk" $SRC
PackLib "extlib" "vld"
cp $ws_dir\jcvos\lib_output.vsprops $tar_dir\jcvos\lib_output.vsprops

#cp -recurse lib $tar_dir\$app_dir

#packing tool
push-location
cd "$tool_dir"
& ".\package\pack_tool.ps1"
mv ".\tool.package" "$tar_dir\tool\$lib_name"
pop-location

cp *.vcproj $tar_dir\$app_dir
cp *.vsprops $tar_dir\$app_dir
cp -recurse source $tar_dir\$app_dir
cp -recurse res $tar_dir\$app_dir
cp -recurse test_data $tar_dir\$app_dir
cp install.ps1 $tar_dir\$app_dir

& 'C:\Program Files\WinRAR\WinRAR.exe' a sn_update_tool.rar $package_name

