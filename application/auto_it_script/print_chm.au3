#cs ----------------------------------------------------------------------------

 AutoIt Version: 3.3.6.1
 Author:         Yuan Jingcheng

 Script Function:
	Template AutoIt script.

#ce ----------------------------------------------------------------------------
; 这是一个用于自动打印chm文件中每一页的script。
; 最为层循环是针对特定项目的，可以去除。
; 内层循环是针对树结构的循环，但是没有考虑到递归。
; 问题：(1)没有自动选择打印机和输出目录，需要事先手动设定好。
;       (2)很多操作通过于Send()函数实现。这样对于GUI的依赖很高，必须保证在发出
;          Send()是，焦点落在正确的窗口上。应该尽量改用Control系列的函数。
;       (3)循环打印目录树结构时，没有考虑递归。

; Script Start - Add your code below here
; AutoItSetOption("WinTitleMatchMode", 4);

For $jj = 1 To 4
;	$jj=1;
	$main_title = "小说" & String($jj);
	$main_wnd = WinActivate($main_title, "");
	if $main_wnd = 0 Then
		ContinueLoop
	EndIf
	$tree = ControlGetHandle($main_wnd, "", "[CLASS:SysTreeView32; INSTANCE:1]");
	ConsoleWrite("tree = " & $tree & @CRLF);
	$count = ControlTreeView($main_wnd, "", $tree, "GetItemCount", "");
	ConsoleWrite("count = " & $count & @CRLF);

	For $ii = 0 To $count-1
		$index = "#" & String($ii);
		ConsoleWrite("printing item" & $index & @CRLF);
		ControlTreeView($main_wnd, "", $tree, "Select", $index);
		Send("{ENTER}");
		Sleep(200);
		
		Send("!o");
		Sleep(200);
		; ControlCommand($tool_bar, "", "", "SendComnandID", 208);
		Send("p");
		Sleep(200);
		$dlg = WinWaitActive("打印主题","");
		ConsoleWrite("dlg print title = " & $dlg & @CRLF);
		ControlClick($dlg, "", 1);
		Sleep(200);
		$dlg = WinWaitActive("打印", "");
		ConsoleWrite("dlg print = " & $dlg & @CRLF);
		;ControlClick($dlg, "", 1);
		Send("!p");
		Sleep(200);
		$dlg = WinWaitActive("另存 PDF 文件为", "");
		ConsoleWrite("dlg save as = " & $dlg & @CRLF);
		Send("!s");
		Sleep(200);
		
		$dlg = WinWaitActive("另存 PDF 文件为", "", 3);
		If $dlg <> 0 Then
			ConsoleWrite("File existed " & $dlg & @CRLF);
			Send("!n");
			Sleep(200);
			Send("{ESC}");
		Else
			$adobi = WinWaitActive("Adobe Acrobat Professional", "");
			Sleep(3000);
			Send("^w");
			Sleep(10000);
		EndIf
		
		WinActivate($main_title, "");
		ConsoleWrite("Activating..." & @CRLF);	
		$main_wnd = WinWaitActive($main_title, ""); 
		ConsoleWrite("Waiting active..." & @CRLF);	
		Sleep(1000);
	Next
	WinClose($main_wnd, "");
	Sleep(2000);
Next