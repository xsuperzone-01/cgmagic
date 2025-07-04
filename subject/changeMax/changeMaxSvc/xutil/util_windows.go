package xutil

import (
	"encoding/json"
	"errors"
	"fmt"
	"golang.org/x/sys/windows/registry"
	"golang.org/x/text/encoding/simplifiedchinese"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"unsafe"
)

//条件编译https://blog.csdn.net/varding/article/details/12675971

func SoftwareInstallPath(tag string) (exe string, err error) {
	tag = strings.ToLower(tag)
	if strings.Contains(tag, "maya") {
		year := SoftwareVer(tag)
		path := "SOFTWARE\\Autodesk\\Maya\\" + year + "\\Setup\\InstallPath"
		key := "MAYA_INSTALL_LOCATION"
		val, err2 := lm64(path, key)

		// 全局没找到就找当前用户注册表
		if val == "" || err2 != nil {
			log.Println("SoftwareInstallPath maya lmCurrentUser")
			val, err2 = lmCurrentUser(path, key)
		}
		if val == "" || err2 != nil {
			return "", err2
		}
		exe = filepath.Join(val, "bin", "mayabatch.exe")
	}
	if strings.Contains(tag, "max") {
		year := SoftwareVer(tag)
		key := "Installdir"
		ss := maxTag[year]
		for _, max := range ss {
			path := "SOFTWARE\\Autodesk\\3dsMax\\" + max
			exe, err = lm64(path, key)
			if exe != "" {
				break
			}
		}

		// 全局没找到就找当前用户注册表
		if exe == "" {
			for _, max := range ss {
				path := "SOFTWARE\\Autodesk\\3dsMax\\" + max
				exe, err = lmCurrentUser(path, key)
				if exe != "" {
					break
				}
			}
		}

		if exe != "" {
			exe = filepath.Join(exe, "3dsmax.exe")
		}
	}
	if strings.Contains(tag, "houdini") {
		version := strings.ReplaceAll(SoftwareVer(tag), "-", "")
		path := "SOFTWARE\\Side Effects Software\\Houdini " + version
		key := "InstallPath"
		val, err2 := lm64(path, key)

		// 全局没找到就找当前用户注册表
		if val == "" || err2 != nil {
			log.Println("SoftwareInstallPath houdini lmCurrentUser")
			val, err2 = lmCurrentUser(path, key)
		}

		if val == "" || err2 != nil {
			return "", err2
		}
		exe = filepath.Join(val, "bin", "hython.exe")
	}
	//TODO c4d路径检测
	if strings.Contains(tag, "c4d") && len(tag) >= 2 {
		version := tag[len(tag)-2:]
		versionNum, _ := strconv.Atoi(version)

		val, err2 := c4dInstall(versionNum)
		if err2 != nil || val == "" {
			return "", err2
		}

		exe = filepath.Join(val, "Commandline.exe")
	}

	if strings.Contains(tag, "katana") {
		version := tag[len(tag)-5:]
		val, err2 := KatanaInstall(version)
		if err2 != nil || val == "" {
			return "", err2
		}

		exe = filepath.Join(val, "bin", "katanaBin.exe")
	}

	if strings.Contains(tag, "clarisse") {
		version := strings.ToUpper(tag[len(tag)-7:])
		val, err2 := clarisseInstall(version)
		if err2 != nil || val == "" {
			return "", err2
		}

		exe = filepath.Join(val, "cnode.exe")
	}

	if strings.Contains(tag, "keyshot") {
		version := tag[len(tag)-1:]
		path := "SOFTWARE\\Luxion\\KeyShot " + version
		key := "InstallDir"
		val, err2 := lm64(path, key)

		// 全局没找到就找当前用户注册表
		if val == "" || err2 != nil {
			log.Println("SoftwareInstallPath houdini lmCurrentUser")
			val, err2 = lmCurrentUser(path, key)
		}

		if val == "" || err2 != nil {
			return "", err2
		}
		exe = filepath.Join(val, "bin", "keyshot.exe")
	}
	if strings.Contains(tag, "blender") {
		version := SoftwareVer(tag)
		val, err := blenderInstall(version)
		if err != nil {
			return "", err
		}
		exe = filepath.Join(val, "blender.exe")
	}
	if strings.Contains(tag, "vred") {
		version := SoftwareVer(tag)
		val, err := vredInstall(version)
		if err != nil {
			return "", err
		}
		exe = filepath.Join(val, "bin", "WIN64", "VREDPro.exe")
	}
	return exe, err
}

func lm64(path, key string) (value string, err error) {
	k, err2 := registry.OpenKey(registry.LOCAL_MACHINE, path, registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err2 != nil {
		return "", err2
	}
	val, _, err3 := k.GetStringValue(key)
	if err3 != nil {
		return "", err3
	}
	return val, nil
}

func lmCurrentUser(path, key string) (value string, err error) {
	k, err2 := registry.OpenKey(registry.CURRENT_USER, path, registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err2 != nil {
		return "", err2
	}
	val, _, err3 := k.GetStringValue(key)
	if err3 != nil {
		return "", err3
	}
	return val, nil
}

func root64(root registry.Key, path, key string) (value string, err error) {
	k, err2 := registry.OpenKey(root, path, registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err2 != nil {
		return "", err2
	}
	val, _, err3 := k.GetStringValue(key)
	if err3 != nil {
		return "", err3
	}
	return val, nil
}

func clarisseInstall(version string) (path string, err error) {
	k, err := registry.OpenKey(registry.LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err != nil {
		return "", err
	}
	keys, _ := k.ReadSubKeyNames(0)

	exepath := ""
	for _, v := range keys {
		path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
		if strings.Contains(v, "Clarisse") {
			DisplayIcon, err := lm64(path, "DisplayIcon")
			if err != nil {
				continue
			}
			DisplayVersion, err := lm64(path, "DisplayVersion")
			if err != nil {
				continue
			}
			DisplayIcon = strings.ReplaceAll(DisplayIcon, "\"", "")
			if strings.Contains(DisplayVersion, version) {
				exepath = filepath.Dir(DisplayIcon)
			}
		}
	}

	if exepath != "" {
		return exepath, nil
	}

	// 全局没找到就找当前用户注册表
	kuser, err := registry.OpenKey(registry.CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
	defer kuser.Close()
	if err != nil {
		return "", err
	}
	keys, _ = k.ReadSubKeyNames(0)
	for _, v := range keys {
		path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
		if strings.Contains(v, "Clarisse") {
			DisplayIcon, err := lmCurrentUser(path, "DisplayIcon")
			if err != nil {
				return "", err
			}
			DisplayVersion, err := lmCurrentUser(path, "DisplayVersion")
			if err != nil {
				return "", err
			}
			DisplayIcon = strings.ReplaceAll(DisplayIcon, "\"", "")
			if strings.Contains(DisplayVersion, version) {
				return filepath.Dir(DisplayIcon), nil
			}
		}
	}

	return "", err
}

func c4dInstall(version int) (path string, err error) {
	k, err := registry.OpenKey(registry.LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err != nil {
		return "", err
	}
	keys, _ := k.ReadSubKeyNames(0)
	maxVersion := ""
	itemName := ""
	for _, v := range keys {
		path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
		if strings.Contains(v, "MAXON") {
			versionName, err := lm64(path, "DisplayVersion")
			if err != nil {
				return "", err
			}
			displayName, err := lm64(path, "DisplayName")
			if err != nil {
				return "", err
			}
			if strings.Contains(strings.ToLower(displayName), "cinema 4d") {
				if versionName != "" {
					versionNum, _ := strconv.Atoi(strings.Split(versionName, ".")[0])
					if version == versionNum && strings.Compare(versionName, maxVersion) > 0 {
						maxVersion = versionName
						itemName = path
					}
				}
			}
		}
	}

	if itemName != "" {
		return lm64(itemName, "InstallLocation")
	}

	// 全局没找到就找当前用户注册表
	if itemName == "" {
		kuser, err := registry.OpenKey(registry.CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
		defer kuser.Close()
		if err != nil {
			return "", err
		}
		keys, _ = k.ReadSubKeyNames(0)
		for _, v := range keys {
			path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
			if strings.Contains(v, "MAXON") {
				versionName, err := lmCurrentUser(path, "DisplayVersion")
				if err != nil {
					return "", err
				}
				displayName, err := lmCurrentUser(path, "DisplayName")
				if err != nil {
					return "", err
				}
				if strings.Contains(strings.ToLower(displayName), "cinema 4d") {
					if versionName != "" {
						versionNum, _ := strconv.Atoi(strings.Split(versionName, ".")[0])
						if version == versionNum && strings.Compare(versionName, maxVersion) > 0 {
							maxVersion = versionName
							itemName = path
						}
					}
				}
			}
		}

		if itemName != "" {
			return lmCurrentUser(itemName, "InstallLocation")
		}
	}

	return "", err
}

func KatanaInstall(version string) (path string, err error) {
	k, err := registry.OpenKey(registry.LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
	defer k.Close()
	if err != nil {
		return "", err
	}
	keys, _ := k.ReadSubKeyNames(0)
	itemName := ""
	for _, v := range keys {
		path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
		if strings.Contains(v, "Katana") {
			displayName, err := lm64(path, "DisplayName")
			if err != nil {
				return "", err
			}
			if strings.Contains(strings.ToLower(displayName), "katana") {
				ver := displayName[len(displayName)-5:]
				if ver == version {
					itemName = path
					break
				}
			}
		}
	}

	if itemName != "" {
		return lm64(itemName, "InstallLocation")
	}

	// 全局没找到就找当前用户注册表
	if itemName == "" {
		kuser, err := registry.OpenKey(registry.CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
		defer kuser.Close()
		if err != nil {
			return "", err
		}
		keys, _ = k.ReadSubKeyNames(0)
		for _, v := range keys {
			path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
			if strings.Contains(v, "Katana") {
				displayName, err := lmCurrentUser(path, "DisplayName")
				if err != nil {
					return "", err
				}
				if strings.Contains(strings.ToLower(displayName), "katana") {
					ver := displayName[len(displayName)-5:]
					if ver == version {
						itemName = path
						break
					}
				}
			}
		}
		if itemName != "" {
			return lmCurrentUser(itemName, "InstallLocation")
		}
	}

	return "", err
}

var regRootL = []registry.Key{registry.LOCAL_MACHINE, registry.CURRENT_USER}
var regNotFound = errors.New("reg not found")
func blenderInstall(version string) (path string, err error) {
	for _, root := range regRootL {
		k, err := registry.OpenKey(root, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
		if err != nil {
			return "", err
		}
		keys, _ := k.ReadSubKeyNames(0)
		k.Close()
		for _, v := range keys {
			path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
			displayName, err := root64(root, path, "DisplayName")
			if err != nil {
				continue
			}
			if strings.Contains(strings.ToLower(displayName), "blender") {
				displayVer, err := root64(root, path, "DisplayVersion")
				if err != nil {
					return "", err
				}
				if version == displayVer || strings.Contains(displayVer, version) || strings.Contains(version, displayVer) {
					loc, err := root64(root, path, "InstallLocation")
					if err != nil {
						return "", err
					}
					return loc, nil
				}
			}
		}
	}

	return "", regNotFound
}

func vredInstall(version string) (path string, err error) {
	for _, root := range regRootL {
		k, err := registry.OpenKey(root, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", registry.READ|registry.WOW64_64KEY)
		if err != nil {
			return "", err
		}
		keys, _ := k.ReadSubKeyNames(0)
		k.Close()
		for _, v := range keys {
			path := "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + v
			if strings.Contains(strings.ToLower(v), "vred") {
				displayName, err := root64(root, path, "DisplayName")
				if err != nil {
					return "", err
				}
				if strings.Contains(strings.ToLower(displayName), "vred") {
					displayVer, err := root64(root, path, "DisplayVersion")
					if err != nil {
						return "", err
					}
					if version == displayVer || strings.Contains(displayVer, version) || strings.Contains(version, displayVer) ||
						strings.Contains(displayName, version) {
						loc, err := root64(root, path, "InstallLocation")
						if err != nil {
							return "", err
						}
						return loc, nil
					}
				}
			}
		}
	}

	return "", regNotFound
}

func Restart(exe string, arg ...string) {
	err := exec.Command(exe, arg...).Start()
	if err != nil {
		log.Println(err)
	}
}

func str2ptr(s string) *uint16 {
	u, _ := syscall.UTF16PtrFromString(s)
	return u
}

func AdminExec(se ShellExecute) error {
	//str2ptr := func(s string) *uint16 {
	//	u, _ := syscall.UTF16PtrFromString(s)
	//	return u
	//}

	shell32, err := syscall.LoadLibrary("Shell32.dll")
	if err != nil {
		return err
	}
	defer syscall.FreeLibrary(shell32)

	ShellExecuteW, err := syscall.GetProcAddress(shell32, "ShellExecuteW")
	if err != nil {
		return err
	}

	r, _, errno := syscall.Syscall6(uintptr(ShellExecuteW), 6,
		0,
		uintptr(unsafe.Pointer(str2ptr("runas"))),
		uintptr(unsafe.Pointer(str2ptr(se.FileName))),
		uintptr(unsafe.Pointer(str2ptr(se.Parameters))),
		uintptr(unsafe.Pointer(str2ptr(se.Directory))),
		uintptr(se.ShowCmd))

	if r != 42 {
		return errno
	}

	return nil
}

func regWrite(key registry.Key, path string, access uint32, name, value string) error {
	ac := uint32(registry.WRITE)
	if access != 0 {
		ac = ac | access
	}

	k, _, err := registry.CreateKey(key, path, ac)
	defer k.Close()
	if err != nil {
		return err
	}
	err = k.SetStringValue(name, value)
	if err != nil {
		return err
	}
	return nil
}
func regRead(key registry.Key, path string, access uint32, name string) (string, error) {
	ac := uint32(registry.READ)
	if access != 0 {
		ac = ac | access
	}

	k, err2 := registry.OpenKey(key, path, ac)
	defer k.Close()
	if err2 != nil {
		return "", err2
	}
	val, _, err3 := k.GetStringValue(name)
	if err3 != nil {
		return "", err3
	}
	return val, nil
}
func regDeleteKey(key registry.Key, path string, access uint32) error {
	ac := uint32(registry.ALL_ACCESS)
	if access != 0 {
		ac = ac | access
	}

	k, err := registry.OpenKey(key, path, ac)
	if err != nil {
		return err
	}
	defer k.Close()

	sks, _ := k.ReadSubKeyNames(0)
	for _, sk := range sks {
		regDeleteKey(key, path+"\\\\"+sk, access)
	}
	err = registry.DeleteKey(key, path)

	if err != nil {
		return err
	}
	return nil
}

type UnregContextMenu struct {
	Submit []string `json:"submit"` //提交渲云渲染
}

var unregTemp = "unreg.json"
var regTags = []string{"提交渲云渲染", "Open with XRender"}
func RegisterContextMenu(admin bool, sfxjson string, en2 bool) {
	var sfxs []string
	_ = json.Unmarshal([]byte(sfxjson), &sfxs)
	cmd := fmt.Sprintf("%s -scene ", AppExe()) + "\"%1\""
	icon := filepath.Join(AppDirPath(), "ui", "fileRMSubmit.ico")

	tag := regTags[0]
	if en2 {
		tag = regTags[1]
	}

	temp := UnregContextMenu{}

	for _, sfx := range sfxs {
		v1, err := regRead(registry.CLASSES_ROOT, sfx, 0, "")
		if err == nil {
			root := v1 + "\\shell\\" + tag
			path := root + "\\command"
			v2, _ := regRead(registry.CLASSES_ROOT, path, 0, "")
			v3, _ := regRead(registry.CLASSES_ROOT, root, 0, "Icon")
			log.Println(admin, cmd, v2, icon, v3, v1)
			if cmd != v2 || (IsExist(icon) && icon != v3) {
				if admin {
					err = AdminExec(ShellExecute{
						FileName:   AppExe(),
						Parameters: "-contextMenu " + strings.ReplaceAll(sfxjson, "\"", "\\\""),
					})
					log.Println("admin", err)
					return
				} else {
					err = regWrite(registry.CLASSES_ROOT, path, 0, "", cmd)
					log.Println(err, path, v2, "=>", cmd, v3, "=>", icon)
					if IsExist(icon) {
						_ = regWrite(registry.CLASSES_ROOT, root, 0, "Icon", icon)
					}

					temp.Submit = append(temp.Submit, root)
				}
			}
		} else {
			//只找已经关联到渲染软件的
			log.Println(sfx, err)
		}
	}

	if len(temp.Submit) > 0 {
		file, err := os.OpenFile(unregTemp, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0666)
		if err != nil {
			log.Println(err)
			return
		}
		defer file.Close()

		b, _ := json.Marshal(&temp)
		file.Write(b)
	}
}

func UnregisterContextMenu() {
	b, err := ioutil.ReadFile(unregTemp)
	if err != nil {
		log.Println(err)
		return
	}

	temp := UnregContextMenu{}
	json.Unmarshal(b, &temp)

	for _, path := range temp.Submit {
	//	idx := strings.LastIndex(path, "\\\\")
	//	for _, v := range regTags {
	//		path = path[:idx] + "\\\\" + v
			err = regDeleteKey(registry.CLASSES_ROOT, path, 0)
			log.Println("delete reg key", path, err)
		//}
	}
}

func RegisterPluginMenu(admin bool) {
	auto := func() error {
		cmd := exec.Command(filepath.Join(AppDirPath(), "auto_execute.cmd"))
		cmd.Env = os.Environ()
		cmd.Dir = AppDirPath()

		stdout, err := cmd.StdoutPipe()
		if err != nil {
			return err
		}
		stderr, err := cmd.StderrPipe()
		if err != nil {
			return err
		}

		err = cmd.Start()
		if err != nil {
			return err
		}

		b, err := ioutil.ReadAll(stderr)
		if err != nil {
			return err
		}
		{
			b, err := ioutil.ReadAll(stdout)
			if err != nil {
				return err
			}
			if len(b) > 0 {
				decodeBytes, err := simplifiedchinese.GBK.NewDecoder().Bytes(b)
				if err != nil {
					return err
				}
				log.Println("RegisterPluginMenu", "admin:", admin, "stdout:", string(decodeBytes))
			}
		}

		cmd.Wait()

		if len(b) > 0 {
			decodeBytes, err := simplifiedchinese.GBK.NewDecoder().Bytes(b)
			if err != nil {
				return err
			}
			return errors.New("retry: " + string(decodeBytes))
		}

		return nil
	}

	err := auto()
	if err != nil {
		log.Println("RegisterPluginMenu", "admin:", admin, err)
		if !admin && strings.HasPrefix(err.Error(), "retry: "){
			err = AdminExec(ShellExecute{
				FileName:   AppExe(),
				Parameters: "-pluginMenu -admin",
			})
			log.Println("admin", err)
		}
	}
}

//卸载插件式提交的渲染软件里的插件
func UnregisterPluginMenu() {
	log.Println("un ", filepath.Join(AppDirPath(), "deleteMaxPlugin.cmd"))
	cmd := exec.Command(filepath.Join(AppDirPath(), "deleteMaxPlugin.cmd"))
	cmd.Env = os.Environ()
	log.Println(cmd.Env)
	err := cmd.Start()
	log.Println("err", err)
	if err != nil {
		return
	}
	err = cmd.Wait()
	log.Println("wait err", err)
}


type ulong int32
type ulong_ptr uintptr

type PROCESSENTRY32 struct {
	dwSize              ulong
	cntUsage            ulong
	th32ProcessID       ulong
	th32DefaultHeapID   ulong_ptr
	th32ModuleID        ulong
	cntThreads          ulong
	th32ParentProcessID ulong
	pcPriClassBase      ulong
	dwFlags             ulong
	szExeFile           [260]byte
}

func SysProcessByPPID(parentPID int) (ret []ProcessStruct) {
	ss := SysProcessList()
	for _, s := range ss {
		if s.ParentProcessID == parentPID {
			ret = append(ret, s)
		}
	}
	return
}

func SysProcessList() (ret []ProcessStruct) {
	//进程快照
	kernel32 := syscall.NewLazyDLL("kernel32.dll")
	CreateToolhelp32Snapshot := kernel32.NewProc("CreateToolhelp32Snapshot")
	pHandle, _, _ := CreateToolhelp32Snapshot.Call(uintptr(0x2), uintptr(0x0))
	if int(pHandle) == -1 {
		log.Println("CreateToolhelp32Snapshot err")
		return
	}

	Process32Next := kernel32.NewProc("Process32Next")
	for {
		var proc PROCESSENTRY32
		proc.dwSize = ulong(unsafe.Sizeof(proc))
		if rt, _, _ := Process32Next.Call(uintptr(pHandle), uintptr(unsafe.Pointer(&proc))); int(rt) == 1 {

			len_szExeFile := 0
			for _, b := range proc.szExeFile {
				if b == 0 {
					break
				}
				len_szExeFile++
			}
			var bytetest []byte = []byte(proc.szExeFile[:len_szExeFile])
			ret = append(ret, ProcessStruct{
				ProcessName: string(bytetest),
				ProcessID:   int(proc.th32ProcessID),
				ParentProcessID: int(proc.th32ParentProcessID),
			})
		} else {
			break
		}
	}

	CloseHandle := kernel32.NewProc("CloseHandle")
	_, _, _ = CloseHandle.Call(pHandle)

	//sort.Sort(ProcessStructSlice(ret))
	return
}

var (
	kernel32         = syscall.NewLazyDLL("kernel32.dll")
	procSetStdHandle = kernel32.NewProc("SetStdHandle")
)

// redirectStderr to the file passed in
func RedirectStderr(f *os.File) {
	err := setStdHandle(syscall.STD_ERROR_HANDLE, syscall.Handle(f.Fd()))
	if err != nil {
		log.Println("Failed to redirect stderr to file:", err)
	}
	// SetStdHandle does not affect prior references to stderr
	os.Stderr = f
}

func setStdHandle(stdhandle int32, handle syscall.Handle) error {
	r0, _, e1 := syscall.Syscall(procSetStdHandle.Addr(), 2, uintptr(stdhandle), uintptr(handle), 0)
	if r0 == 0 {
		if e1 != 0 {
			return error(e1)
		}
		return syscall.EINVAL
	}
	return nil
}

func FileRealPath(file string) string {
	var rets []string
	tmp := ""
	dirs := strings.Split(file, "/")

	for _, dir := range dirs {
		if tmp == "" {
			tmp = dir
		} else {
			tmp += "/" + dir
		}

		var find syscall.Win32finddata
		h, err := syscall.FindFirstFile(str2ptr(tmp), &find)
		if err == nil {
			syscall.FindClose(h)
			name := syscall.UTF16ToString(find.FileName[:])
			rets = append(rets, name)
		} else {
			rets = append(rets, dir)
		}
	}

	return strings.Join(rets, "/")
}