package xutil

import (
	"log"
	"os"
	"syscall"
)

//条件编译https://blog.csdn.net/varding/article/details/12675971

func SoftwareInstallPath(year string) (string, error) {
	return "", nil
}
func Restart(exe string, arg ...string) {

}
func AdminExec(se ShellExecute) error {
	return nil
}

func RegisterContextMenu(admin bool, sfxjson string, en2 bool) {

}

func UnregisterContextMenu() {

}

func SysProcessByPPID(parentPID int) (ret []ProcessStruct) {
	return
}

func UnregisterPluginMenu() {

}

func RegisterPluginMenu(admin bool) {

}

func RedirectStderr(f *os.File) {
	err := syscall.Dup2(int(f.Fd()), int(os.Stderr.Fd()))
	if err != nil {
		log.Println("Failed to redirect stderr to file:", err)
	}
	// SetStdHandle does not affect prior references to stderr
	os.Stderr = f
}