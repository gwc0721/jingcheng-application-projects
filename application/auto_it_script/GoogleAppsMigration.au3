#cs ----------------------------------------------------------------------------

 AutoIt Version: 3.3.6.1
 Author:         myName

 Script Function:
	这个脚本用于自动运行Google的同步程序Google Apps Migration. 
	运行改脚本需要手动关闭Outlook。 手动关闭是为了防止Outlook数据丢失。
	脚本运行后，会等待Outlook的关闭。


#ce ----------------------------------------------------------------------------
#include <GUIConstantsEx.au3>

; Script Start - Add your code below here
; Waiting Close Outlook
AutoItSetOption("WinTitleMatchMode", 4);

$msgwnd = GUICreate("Waiting Outlook Closing", 200, 30);
$lable = GUICtrlCreateLabel("Waiting outlook closing", -1, -1);
GUISetState(@SW_SHOW);

While 1
	$msg = GUIGetMsg();
	if $msg = $GUI_EVENT_CLOSE Then
		ConsoleWrite("User canceled." & @CRLF);
		Exit
	EndIf
	if WinExists("[REGEXPTITLE:[Microsoft Outlook]; CLASS:rctrl_renwnd32]", "") = 0 Then
		ExitLoop
	EndIf
WEnd
;WinWaitClose("[REGEXPTITLE:[Microsoft Outlook]; CLASS:rctrl_renwnd32]", "")

ControlSetText($msgwnd, "", $lable, "Processing...");

; Start Google Migration
Run("C:\Program Files\Google\Google Apps Migration\ClientMigration.exe", "C:\Program Files\Google\Google Apps Migration");
$wnd = WinWaitActive("[REGEXPTITLE:[ Google Apps Migration]; CLASS:#32770]", "", 5) ;
if (Not $wnd) Then
	ConsoleWrite("run app time out" & @CRLF);
	Exit
EndIf
ConsoleWrite("google app = " & $wnd & @CRLF);

; Input mail address
ControlSetText($wnd, "", 18915, "jcyuan78@gmail.com");
; Select have password
ControlCommand($wnd, "", 18917, "Check", "");
; Input password
ControlSetText($wnd, "", 18918, "aaddggjj");
; Click Continue
ControlClick($wnd, "", 18920);

WinWaitClose($wnd, "", 5);

$wnd = WinWaitActive("[REGEXPTITLE:[ Google Apps Migration]; CLASS:#32770]", "", 5)
if (Not $wnd) Then
	ConsoleWrite("time out while inputing account" & @CRLF);
	Exit
EndIf
ConsoleWrite("wnd = " & $wnd & @CRLF);
; Select profile "outlook" 
ControlCommand($wnd, "", 1022, "SetCurrentSelection", 0);
; Set new data only
ControlClick($wnd, "", 1024);
; Click Next
$next = ControlGetHandle($wnd, "", 12324);
; ConsoleWrite("next = " & $ctrl & @CRLF);
ControlClick($next, "", "");

Sleep(1000);
; Uncheck "Email message"
ControlCommand($wnd, "", 1004, "UnCheck", "");
; Click Migrate
ControlClick($next, "", "");

Sleep(10000);
$wnd2 = WinWaitActive("[REGEXPTITLE:[ Google Apps Migration]; CLASS:#32770; W:323; H:118]", "", 10);
ConsoleWrite("wnd2 = " & $wnd2 & @CRLF);
Send("{ENTER}");
; Click Close
WinWaitActive($wnd, "", 1);
ControlClick($wnd, "", 2);

GUIDelete($msgwnd);
