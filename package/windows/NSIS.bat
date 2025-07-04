@echo off

cd %cd%

set exePath=%1
set product=%2
set version=%3
set third=%4
set RAR="C:\Program Files\WinRAR\WinRAR.exe"

echo f|xcopy "client\ini\XR-%third%.ico" "output\sfx\env\XR.ico" /e /i /h /y
echo f|xcopy "client\ini\XR-%third%.ini" "output\sfx\env\XR.ini" /e /i /h /y
if exist "client\ini\XR-%third%_ins.ico" (
echo f|xcopy "client\ini\XR-%third%_ins.ico" "output\sfx\sfx_ins.ico" /e /i /h /y
) else (
echo f|xcopy "client\ini\XR-%third%.ico" "output\sfx\sfx_ins.ico" /e /i /h /y
)

if %4 equ OS goto OS_0
goto OS_1
:OS_0
cd output/sfx/env
%RAR% a "files.zip" "XR.ini"
cd ../../..
:OS_1

cd output/sfx
del sfx.exe
set nsisPath=C:\Program Files (x86)\NSIS
set path=%path%;%nsisPath%
set nsi=nsi.nsi
if exist "nsi-%third%.nsi" (
set nsi=nsi-%third%.nsi
)

makensis.exe /DPRODUCT_VERSION=%version% %nsi%
cd ../..

set woexe="%exePath%\%third%%product%_%version%.exe"
copy "output\sfx\sfx.exe" %woexe%

call "C:\wosign\sign.bat" %woexe%
