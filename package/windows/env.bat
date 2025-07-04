@echo off

if %USERNAME% equ 0968zbl goto local
goto remote

:local
set qt=D:\Qt6
set vs=D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.29.30133
set kitRoot=D:\Windows Kits\10
set kitVer=10.0.17763.0
goto end

:remote
set qt=C:\Qt6
set vs=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133
set kitRoot=C:\Program Files (x86)\Windows Kits\10
set kitVer=10.0.17763.0
goto end

:end
set kitInclude=%kitRoot%\include\%kitVer%
set kitLib=%kitRoot%\lib\%kitVer%
set kitBin=%kitRoot%\bin\%kitVer%
::qmake
set path=%qt%\6.6.3\msvc2019_64\bin;%path%
::make
set path=%qt%\Tools\QtCreator\bin;%path%
::cl
set path=%vs%\bin\Hostx64\x64;%path%
::从Qt Creator的左侧菜单，项目->Build->构建环境，复制环境变量INCLUDE
set INCLUDE=%vs%\atlmfc\include;%vs%\include;%kitInclude%\ucrt;%kitInclude%\shared;%kitInclude%\um;%kitInclude%\winrt;%kitInclude%\cppwinrt
::rc
set path=%kitBin%\x64;%path%
::从Qt Creator的左侧菜单，项目->Build->构建环境，复制环境变量LIB
set LIB=%vs%\ATLMFC\lib\x64;%vs%\lib\x64;%kitLib%\ucrt\x64;%kitLib%\um\x64

echo "设置env结束"
