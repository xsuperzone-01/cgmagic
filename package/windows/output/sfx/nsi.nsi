!include exe.nsh

icon "sfx_ins.ico"
OutFile "sfx.exe"
SilentInstall silent


Var suffix
Var suffix_str

Function Random
Exch $0
Push $1
System::Call 'kernel32::QueryPerformanceCounter(*l.r1)'
System::Int64Op $1 % $0
Pop $0
Pop $1
Exch $0
FunctionEnd


Function .onInit #NSIS程序安装准备工作

Push "1000" 
Call Random
Pop $suffix

; 将随机数转换为字符串
StrCpy $suffix_str $suffix

InitPluginsDir #创建临时目录（内存）用于保存一些必要的库
SetOutPath $TEMP\xcgmagic_ins$suffix_str #将文件保存在临时目录中
	RMDir /r $TEMP\xcgmagic_ins$suffix_str
	File /r "env\*"
FunctionEnd

Function .onInstSuccess #安装成功后调用
call fun
FunctionEnd

Function fun
   ExecWait "$TEMP\xcgmagic_ins$suffix_str\install.exe $CMDLINE" #调用QT安装程序，等待其运行完后运行下一条语句
   RMDir /r $TEMP\xcgmagic_ins$suffix_str                #释放临时目录（内存）

FunctionEnd

section
sectionend