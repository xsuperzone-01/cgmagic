package main

import "C"
import (
	"flag"
	"github.com/shirou/gopsutil/process"
	"golang.org/x/text/encoding/simplifiedchinese"
	"log"
	"os"
	"path/filepath"
	"strings"
)

// 最好是管理员权限调用此exe
// 编成exe，火绒会报后门病毒，应该是github.com/shirou/gopsutil/process里面导致
func main()  {
	flag.CommandLine = flag.NewFlagSet(os.Args[0], 10)
	dir := flag.String("dir", "", "杀掉dir下的exe进程")
	white := flag.String("white", "", "免杀白名单，用|分割")
	flag.Parse()

	if *dir == "" {
		os.Exit(0)
	}

	killExeByDir(*dir, *white)
}

//export KillExeByDir
func KillExeByDir(dirC *C.char, whiteC *C.char) {
	dir := C.GoString(dirC)
	white := C.GoString(whiteC)
	killExeByDir(dir, white)
}

func killExeByDir(dir string, white string) error {
	dir = filepath.Clean(filepath.ToSlash(dir))
	var whiteList []string
	for _, white := range strings.Split(white, "|") {
		if white != "" {
			whiteList = append(whiteList, white)
		}
	}
	log.Println(dir, whiteList)

	pros, err := process.Processes()
	if err != nil {
		return err
	}

	for _, pro := range pros {
		// 禁止杀调用者
		if int32(os.Getpid()) == pro.Pid {
			continue
		}

		exe, err := pro.Exe()
		if err != nil {
			log.Println("pro.Exe", err.Error())
			continue
		}

		// qt的32位程序load此dll时，中文会乱码
		ds, err := simplifiedchinese.GBK.NewEncoder().String(exe)
		if err == nil {
			exe = ds
		}

		if inWhiteList(exe, whiteList) {
			continue
		}

		exe = filepath.Clean(filepath.ToSlash(exe))
		//log.Println(strings.Contains(exe, dir), dir, exe)
		if strings.Contains(exe, dir) {
			err := pro.Terminate()
			log.Println("pro.Terminate", exe, err)
		}
	}
	return nil
}

func inWhiteList(exe string, whiteList []string) bool {
	for _, white := range whiteList {
		if strings.HasSuffix(exe, white) {
			return true
		}
	}
	return false
}