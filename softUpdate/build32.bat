cd %~dp0
set GO111MODULE=on
set GOARCH=386
::不能有main.syso,main.exe.manifset文件
set CGO_ENABLED=1
::当GOARCH=386且-o文件名带update时默认带上了管理员图标？？？
::远程桌面中明明是32位GCC，居然命名64位
set PATH=C:\TDM-GCC-64\bin;%PATH%
go build -o cgmagicUpgrader.exe -ldflags "-s -w -H windowsgui"