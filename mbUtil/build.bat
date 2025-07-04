cd %~dp0
set GO111MODULE=on
set GOARCH=amd64
go build -ldflags="-s -w" -o mbUtil.exe