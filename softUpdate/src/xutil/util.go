package xutil

import (
	"crypto/md5"
	"encoding/hex"
	"github.com/shirou/gopsutil/process"
	"io"
	"log"
	"os"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"time"
)

type CmdLine struct {
	ExeDir       string // 关闭该目录下所有运行的exe
	RequireAdmin bool
}

func KillExeByDir(dir string) error {
	dir = ToSingleFSlash(dir)

	pros, err := process.Processes()
	if err != nil {
		return err
	}
	//最好是管理员权限
	for _, pro := range pros {
		if int32(os.Getpid()) == pro.Pid {
			continue
		}
		exe, err := pro.Exe()
		if err != nil {
			log.Println("pro.Exe", err.Error())
			continue
		}
		if strings.HasSuffix(exe, "uninstall.exe") || strings.HasSuffix(exe, "uninstall") {
			continue
		}
		exe = ToSingleFSlash(exe)
		if strings.Contains(exe, dir) {
			err := pro.Terminate()
			log.Println("pro.Terminate", exe, err)
		}
	}
	return nil
}

func SelfKill(pid int) {
	tk := time.NewTicker(3 * time.Second)
	defer tk.Stop()
	for {
		select {
		case <-tk.C:
			pros, err := process.Processes()
			if err != nil {
				continue
			}

			toKill := true
			for _, pro := range pros {
				if int(pro.Pid) == pid {
					toKill = false
				}
			}
			if toKill {
				os.Exit(0)
			}
		}
	}
}

//斜杠分正斜杠（forward slash'/'）和反斜杠（back slash'\'）
func ToSingleFSlash(path string) string {
	for strings.Contains(path, "\\") {
		path = strings.ReplaceAll(path, "\\", "/")
	}
	for strings.Contains(path, "//") {
		path = strings.ReplaceAll(path, "//", "/")
	}
	return path
}

func ToInt(i interface{}) int {
	//log.Println(i, reflect.TypeOf(i))
	switch i.(type) {
	case int:
		return i.(int)
	case float64:
		return int(i.(float64))
	case string:
		bb, err := strconv.ParseInt(i.(string), 10, 64)
		if err == nil {
			return int(bb)
		}
	default:
	}
	return 0
}
func ToString(i interface{}) string {
	//log.Println(i, reflect.TypeOf(i))
	switch i.(type) {
	case int:
		return strconv.Itoa(i.(int))
	case float64:
		return strconv.Itoa(int(i.(float64)))
	case int64:
		return strconv.Itoa(int(i.(int64)))
	case string:
		return i.(string)
	default:
	}
	return ""
}

func IsWindows() bool {
	sys := runtime.GOOS
	return "windows" == sys
}

func IsMac() bool {
	sys := runtime.GOOS
	return "darwin" == sys
}

func SysAppData() (path string) {
	if IsWindows() {
		path = os.Getenv("AppData")
	} else {
		path = os.Getenv("HOME")
	}
	return
}

func IsExist(path string) bool {
	_, err := os.Stat(path)
	return err == nil || os.IsExist(err)
}

func FileMd5(fileName string) (string, error) {
	file, err := os.Open(fileName)
	if err != nil {
		return "", err
	}
	defer file.Close()
	m5 := md5.New()
	_, err = io.Copy(m5, file)
	if err != nil {
		return "", err
	}
	md5Str := hex.EncodeToString(m5.Sum(nil))
	return md5Str, nil
}

func AppDirPath() string {
	dir, _ := filepath.Abs(filepath.Dir(os.Args[0]))
	//dir = strings.ReplaceAll(dir, "\\", "/")
	return dir
}
