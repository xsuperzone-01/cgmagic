@echo off

cd ..\..\

for %%i in (softUpdate, mbUtil, killExe) do (
	xcopy "subject\version.h" "%%i\" /y
	xcopy "subject\exe.rc" "%%i\" /y
	xcopy "subject\exe.ico" "%%i\" /y
	windres -F pe-i386 -o %%i\exe.syso %%i\exe.rc
	
	del %%i\version.h
	del %%i\exe.rc
	del %%i\exe.ico
)

cd %~dp0