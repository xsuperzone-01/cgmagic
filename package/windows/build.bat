@echo off

cd %~dp0

set product=cgmagic
set exeName=cgmagic.exe

::9��֮ǰȡֵ���пո� winrar��������
set d=%date:~0,4%%date:~5,2%%date:~8,2%
set tt=%time:~0,2%
if %tt% LSS 10 (
	set t=%time:~1,1%%time:~3,2%%time:~6,2%
	set vertime=%date:~8,2%%time:~1,1%%time:~3,2%
) else (
	set t=%time:~0,2%%time:~3,2%%time:~6,2%
	set vertime=%date:~8,2%%time:~0,2%%time:~3,2%
)

::�ж��Ƿ����ⲿ�����ʱ��
if "%1"=="" (
	set dateAndTime=%d%-%t%
)else (
	set dateAndTime=%1
)

::�򿪿����������벽��
::goto exeOk
::��տͻ��������ļ�
call clear.bat "client\exe"
call clear.bat "update\files"
call clear.bat "output\sfx\env"
call clear.bat "client\dll\mobao"
::call clear.bat "C:\cg-client-tmp"

:: ��ȡħ��  ֻ��key  ���ã�ֱ�Ӵ�ħ������汾�� 
::git clone http://gitlab-ci-token:ERs5p-p6F2vuCKaaSK7P@gitlab.cudatec.com/xsuperzone/cg-magic/mobao.git C:\cg-client-tmp --depth=1
xcopy "C:\cg-client-tmp\%TEST%mobao" "client\dll\mobao" /e /i /h /y

::��ʼ����
call rc.bat
call env.bat
::����cgmagic.exe
call qt.bat ..\..\..\subject\client.pro %product%
::����install.exe
call qt.bat ..\..\..\install\install\install.pro install
::����uninstall.exe
call qt.bat ..\..\..\install\uninstall\uninstall.pro uninstall
::����cgmagicUpgrader.exe
call ..\..\softUpdate\build32.bat
cd %~dp0
echo f|xcopy "..\..\softUpdate\cgmagicUpgrader.exe" "client\exe\cgmagicUpgrader.exe" /e /i /h /y
::����killExe.exe
call ..\..\killExe\build32dll.bat
cd %~dp0
echo f|xcopy "..\..\killExe\killExe.dll" "client\exe\killExe.dll" /e /i /h /y
::����mbUtil.exe
call ..\..\mbUtil\build.bat
cd %~dp0
echo f|xcopy "..\..\mbUtil\mbUtil.exe" "client\exe\mbUtil.exe" /e /i /h /y
::����changeMaxSvc.dll
call ..\..\subject\changeMax\changeMaxSvc\build.bat
cd %~dp0
echo f|xcopy "..\..\subject\changeMax\changeMaxSvc\changeMaxSvc.dll" "client\exe\changeMaxSvc.dll" /e /i /h /y
::exeOk
::����ǩ��
for /f "delims=" %%i in ('dir /a-d/b/s client\exe\*.exe') do call "C:\wosign\sign.bat" %%i
for /f "delims=" %%i in ('dir /a-d/b/s client\exe\*.dll') do call "C:\wosign\sign.bat" %%i
echo f|xcopy "client\exe\mbUtil.exe" "client\dll\mobao\mbUtil.exe" /e /i /h /y

::exeOk
::����dll
xcopy "client\dll" "update\files" /e /i /h /y
xcopy "client\runtime" "update\files" /e /i /h /y
::�����ͻ���������
xcopy "client\exe\*.exe" "update\files" /e /i /h /y
xcopy "client\exe\*.dll" "update\files" /e /i /h /y
for %%i in (install, mbUtil) do del "update\files\%%i.exe"

::������װ��������������װ��
xcopy "client\exe\install.exe" "output\sfx\env" /e /i /h /y
xcopy "client\exe\killExe.dll" "output\sfx\env" /e /i /h /y
xcopy "client\public_dll" "output\sfx\env" /e /i /h /y
xcopy "client\runtime" "output\sfx\env" /e /i /h /y
::exeOk

::��ȡclient�汾
::set clientVer = 0
::for /f "tokens=3" %%i in ('findstr /c:"CLIENT_VERSION " ..\yingshi\common\version.h') do set clientVer=%%i
::��ȡclient�汾
set cur=%cd:\=\\%
for /f "skip=1 tokens=2 delims==" %%i in (
	'wmic datafile where "name='%cur%\\client\\exe\\%exeName%'" get Version /format:list'
) do for /f "delims=" %%v in ("%%i") do set "clientVer=%%v"
echo "�ͻ��˰汾��"
echo %clientVer%
echo %clientVer% > output\sfx\env\version

::��������Ŀ¼
set release=C:\%product%-client\%dateAndTime%-%clientVer%
md %release%
md %release%\exe
echo %release% > output\sfx\env\clientDir
copy "client\exe" %release%\exe
::��ÿ�ν�exe��pdbһ��Ž�clientĿ¼����ֹ��ƥ������
explorer %release%

::ѹ���ͻ������
::�˴�һ�������ѹ����ȥ��һ���ִӰ�װ���򿽱�����װĿ¼
::��Ҫ��WinRAR.exe����rar.exe quazip��������
cd update\files
for %%i in (files.zip, files.7z) do del %%i

"C:\Program Files\WinRAR\WinRAR.exe" a files.zip "*" -r
echo f|xcopy "files.zip" "..\..\output\sfx\env\files.zip" /e /i /h /y
del "files.zip"

::ѹ�����°�
rd mobao /s /q
md mobao
echo f|xcopy "..\..\client\exe\mbUtil.exe" "mobao\mbUtil.exe" /e /i /h /y
set updategzname=%product%-update-%dateAndTime%-%clientVer%.7z
"C:\\Program Files (x86)\7-Zip\7z.exe" a -t7z files.7z ./
echo f|xcopy "files.7z" %release%\%updategzname% /e /i /h /y
cd ..\..


::X:CG OS:���⤖
for %%i in (X, OS) do call NSIS.bat %release% %product% %clientVer%.%vertime% %%i

