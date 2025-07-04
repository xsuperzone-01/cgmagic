cd %~dp0
set GO111MODULE=on
set GOARCH=amd64
set CGO_ENABLED=1
go build -buildmode=c-shared -ldflags "-s -w" -o killExe.dll main.go
set CGO_ENABLED=0