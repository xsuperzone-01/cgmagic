@echo off

cd client\dll\mobao

"C:\Program Files\WinRAR\WinRAR.exe" a plugin.zip "*" -r
::更新包不打素材
::"C:\Program Files\WinRAR\WinRAR.exe" d plugin.zip Data
echo f|xcopy "plugin.zip" "..\..\..\plugin\plugin.zip" /e /i /h /y
del "plugin.zip"

cd ..\..\..\plugin
del plugin.7z
"C:\Program Files\7-Zip\7z.exe" a -t7z plugin.7z ./