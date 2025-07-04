@echo off

rd tmp /s /q
md tmp
cd tmp
qmake %1 -spec win32-msvc CONFIG+=qtquickcompiler
jom -j4
cd ..

xcopy "tmp\release\%2.exe" "client\exe" /e /i /h /y
xcopy "tmp\release\%2.pdb" "client\exe" /e /i /h /y