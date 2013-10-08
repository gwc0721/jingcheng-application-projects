#cs ----------------------------------------------------------------------------

 AutoIt Version: 3.3.6.1
 Author:         myName

 Script Function:
	Template AutoIt script.

#ce ----------------------------------------------------------------------------

#cs
这是一个用于自动发送邮件的脚本。邮件的标题存放在当前目录的"subject.txt"中，
内容和bcc分别存放在"body.txt"和"bcc_asano.txt"中。to保存在剪切板中。按下<ctrl>+<alt>+m
自动发送邮件。
技术要点：HotKeySet()
ShellExecute()
mailto:语法
#ce

#include <file.au3>
; Script Start - Add your code below here

$subject = FileRead("subject.txt");
$body = FileRead("body.txt");
$bcc_asano = FileRead("bcc_asano.txt");

;$bcc_asano = "bcc=asano116@crest.ocn.ne.jp&"
;$bcc_asano = ""

$ir = HotKeySet("+!m", "AutoSendMail");
ConsoleWrite("ir = " & $ir & @CRLF);

While 1
	ConsoleWrite("Running..." & @CRLF);
	Sleep(100000);
Wend

Func AutoSendMail($mail_add_in = "")
	Local $mail_add;
	Local $wnd_mail;
	Local $ctrl_body;
	
	$mail_add = Eval("mail_add_in");
	If ($mail_add = "" ) Then
		ConsoleWrite("Get mail addres from clip" & @CRLF);
		$mail_add = ClipGet() ;
	EndIf
	ConsoleWrite("Mail addres = " & $mail_add & @CRLF);
	If ($mail_add = "" Or StringInStr($mail_add, "@") =0 ) Then
		Return
	EndIf
	
	$mail_to = "mailto:" & $mail_add & "?" & $bcc_asano & "subject=" & $subject
	ShellExecute($mail_to);
	$wnd_mail = WinWaitActive($subject);

	ConsoleWrite("wnd_mail = " & $wnd_mail & @CRLF);
	$ctrl_body = ControlGetHandle($wnd_mail, "", "[CLASS:Internet Explorer_Server]") ;
	ConsoleWrite("ctrl_body = " & $ctrl_body & @CRLF);

	ClipPut($body);
	ControlClick($ctrl_body, "", "");
	Send("^v");
	Sleep(100);
	Send("!s");
;	Sleep(100);
	ClipPut("");
EndFunc
;SendKeepActive($wnd_mail, $body);
;ControlSetText($ctrl_body, "", "", $body);

		   
		   