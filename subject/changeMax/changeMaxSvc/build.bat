cd %~dp0

set GO111MODULE=on
set GOARCH=amd64
set CGO_ENABLED=1

go build -buildmode=c-shared -ldflags "-s -w" -o changeMaxSvc.dll main.go

::lib读取无效
::go build -buildmode=c-shared -ldflags "-s -w" -o changeMaxSvc.lib main.go


::另一个教程 用go+gcc编译::https://www.cnblogs.com/majianguo/p/7486812.html
::go build -buildmode=c-archive
::安装TDM-gcc
::gcc -m32 -shared -o changeMaxSvc.dll changeMaxSvc.def changeMaxSvc.a -static -lwinmm -lWs2_32

::echo f|xcopy "changeMaxSvc.dll" "D:\xsuperzone\cgmagic\dev\build-client-Desktop_Qt_5_14_2_MSVC2017_32bit-Release\release" /e /i /h /y
::echo f|xcopy "changeMaxSvc.lib" "D:\" /e /i /h /y

::pause