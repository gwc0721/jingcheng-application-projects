#参数：
param($tar, $DBG)

if ( test-path -path $tar )
{
	remove-item -force -recurse -path $tar
}
md -force $tar
cp -recurse test_data\* $tar\.
cp FerriSdk\SDK\* $tar\.
if ($DBG -eq "DEBUG")
{
	cp DEBUG_STATIC_MFC\UpdateSnTool.exe $tar\.
}
else
{
	cp RELEASE_STATIC_MFC\UpdateSnTool.exe $tar\.
}
