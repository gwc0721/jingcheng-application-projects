#cs ----------------------------------------------------------------------------

 AutoIt Version: 3.3.6.1
 Author:         Jingcheng Yuan

 Script Function:
	VNC客户端启动脚本：
	启动VNC客户端前，调用Putty启动服务器端的VNC Server。
	然后启动客户端VNC Viwer. 等待输入密码，显示VNC窗口后，
	自动设置窗口全屏。
	等VNC客户端关闭口，调用Putty关闭服务器端的VNC Server。
	TODO: 	
		1，所有的WinWaitActive都要设timeout，并且检查返回值
		2，将服务器名(svrwasabi)和用户名(bytekiller)改为输入参数

#ce ----------------------------------------------------------------------------

; Start putty	
Run("D:\Tools\putty\putty.exe", "");
$conf_wnd = WinWaitActive("PuTTY Configuration", "");
ConsoleWrite("config wnd = " & $conf_wnd & @CRLF)
$hd = ControlGetHandle($conf_wnd, "", 1055) ; Input saved config name 
ControlSend($hd, "", "", "svrwasabi");
ControlClick($conf_wnd, "", 1058) ; Click "Load"
ControlClick($conf_wnd, "", 1009) ; Click "Open"

; Start vncserver in ssh
AutoItSetOption("WinTitleMatchMode", 4);
$ssh_wnd = WinWaitActive("[CLASS:PuTTY]", ""); 
ConsoleWrite("ssh wnd = " & $ssh_wnd & @CRLF);
$passwd = InputBox("Password", "Enter your password.", "", "*")
WinActivate($ssh_wnd);
Sleep(2000);
Send("bytekiller{ENTER}");
Sleep(1000);
Send($passwd & "{ENTER}");
Sleep(3000);
Send("vncserver -geometry 1280x1024 :1{ENTER}");
Sleep(1000);
WinSetState($ssh_wnd, "", @SW_HIDE);

; Start VNC Viwer
$vnc_pid = Run("C:\Program Files\RealVNC\VNC4\vncviewer.exe");
AutoItSetOption("WinTitleMatchMode", 1);
$vnc_wnd = WinWaitActive("VNC Viewer", "");
ConsoleWrite("vnc wnd = " & $vnc_wnd & @CRLF);
$hd = ControlGetHandle($vnc_wnd, "", 1001);
ControlSetText($hd, "", "", "svrwasabi:1") ; Input server and display name
ControlClick($vnc_wnd, "", 1);

; Enter password of VNC, suppose it is same as ssh
$vnc_auth = WinWaitActive("VNC Viewer : Authentication", "");
ControlSend($vnc_auth, "", 1000, $passwd);
ControlClick($vnc_auth, "", 1);
ConsoleWrite("vnc auth = " & $vnc_auth & @CRLF);

; Set full screen of VNC Viwer
Sleep(1000);
$vnc_main = WinWaitActive("svrwasabi:", "");
ConsoleWrite("vnc main = " & $vnc_main & @CRLF);
Send("{F8}f");
ProcessWaitClose($vnc_pid);

; Stop VNC Server in ssh
WinSetState($ssh_wnd, "", @SW_SHOW);
Sleep(2000);
WinActivate($ssh_wnd);
Send("vncserver -kill :1{ENTER}");
Sleep(1000);
Send("exit{ENTER}");

ConsoleWrite("Script Finished" & @CRLF);
