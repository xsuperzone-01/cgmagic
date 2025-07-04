cd %~dp0
set GO111MODULE=on
set GOARCH=386
go build -ldflags="-s -w -H windowsgui" -o killExe.exe