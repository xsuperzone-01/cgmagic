package xutil

import (
	"archive/zip"
	"context"
	"crypto/md5"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

func Gid() string {
	return strconv.FormatInt(time.Now().UnixNano(), 10)
}
func Ugid(pre string, idx *int) string {
	t := Gid()
	if idx == nil {
		return t
	}
	if pre >= t {
		*idx++
		idxs := ToString(*idx)
		t = t[:len(t)-len(idxs)] + idxs
	} else {
		*idx = 0
	}
	return t
}

// 判断所给路径是否为文件夹
func IsDir(path string) bool {
	s, err := os.Stat(path)
	if err != nil {
		return false
	}
	return s.IsDir()
}

// 判断所给路径是否为文件
func IsFile(path string) bool {
	return !IsDir(path)
}

//扫描目录	会将文件夹下的文件和子文件夹分别返回出来；注：只扫描一层目录
func ScanDirOnce(path string) (files []string, dirs []string, err error) {
	if !IsExist(path) {
		err = errors.New("路径不存在")
		return
	}

	if IsFile(path) {
		files = append(files, path)
		return
	}

	fs, _ := ioutil.ReadDir(path)
	for _, v := range fs {
		// 遍历得到文件名
		if v.IsDir() {
			dirs = append(dirs, v.Name())
		} else {
			files = append(files, v.Name())
		}
	}
	return
}

func ScanDir(absolute string) (files []string) {
	if absolute == "" {
		return files
	}

	err := filepath.Walk(absolute, func(path string, f os.FileInfo, err error) error {
		if f == nil || f.IsDir() {
			return nil
		}
		files = append(files, path)
		return nil
	})
	if err != nil {
		log.Println("ScanDir", err)
	}
	return
}

func AppExe() string {
	//部分系统拿出来会带空格 导致xutil.RegisterContextMenu一直匹配不上
	return strings.TrimRight(string(os.Args[0]), " ")
}

func IsContextCanceld(ctx context.Context) bool {
	if ctx.Err() != nil {
		if strings.Contains(ctx.Err().Error(), "context canceled") {
			return true
		}
	}
	return false
}

func JsonDecode(in string, out interface{}) {
	d := json.NewDecoder(strings.NewReader(in))
	d.UseNumber()
	d.Decode(&out)
}

func FormatStamp(stamp int64) string {
	return time.Unix(stamp, 0).Format("2006-01-02 15:04:05")
}

func Min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}
func Max(a, b int) int {
	if a > b {
		return a
	} else {
		return b
	}
}
func FormatSize(sum int64) string {
	ret := "0.00B"
	if sum < 1024 {
		ret = fmt.Sprintf("%.2fB", float32(sum))
	} else if sum < 1048576 {
		ret = fmt.Sprintf("%.2fKB", float32(sum)/float32(1024))
	} else if sum < 1073741824 {
		ret = fmt.Sprintf("%.2fMB", float32(sum)/float32(1048576))
	} else {
		ret = fmt.Sprintf("%.2fGB", float32(sum)/float32(1073741824))
	}
	return ret
}

func IsLinux() bool {
	return "linux" == runtime.GOOS
}

var maxTag map[string][]string

func CreateMaxTag() {
	maxTag = make(map[string][]string)
	for year := 2010; year <= 2030; year++ {
		years := strconv.Itoa(year)
		tag, _ := strconv.Atoi(years[2:])
		tags := strconv.Itoa(tag+2) + ".0"
		var ts []string
		if year >= 2010 && year <= 2012 {
			ts = append(ts, tags+"\\MAX-1:409")
			ts = append(ts, tags+"\\MAX-1:804")
		} else {
			ts = append(ts, tags)
		}
		maxTag[years] = ts
	}
	//log.Println(maxTag)
}

func SoftwareVer(tag string) string {
	tag = strings.ToLower(tag)
	for _, s := range []string{"maya", "3ds", "max", "houdini", " "} {
		tag = strings.ReplaceAll(tag, s, "")
	}
	return tag
}

func Md5(str string) string {
	data := []byte(str)
	has := md5.Sum(data)
	md5str := fmt.Sprintf("%x", has)
	return md5str
}

func SetSysEnv(key, value string) {
	cmd := exec.Command("setx", key, value)
	err := cmd.Start()
	if err != nil {
		log.Println(err)
	}
	cmd.Wait()
}

func MapIfNo(m *map[string]interface{}, key string, value interface{}) {
	if m != nil {
		if _, ok := (*m)[key]; !ok {
			(*m)[key] = value
		}
	}
}
func MapRenameKey(m *map[string]interface{}, oldk, newk string) {
	if m != nil {
		if d, ok := (*m)[oldk]; ok {
			(*m)[newk] = d
			delete(*m, oldk)
		}
	}
}
func MapJoin(m *map[string]interface{}, m2 *map[string]interface{}) {
	if m != nil && m2 != nil {
		for k, v := range *m2 {
			(*m)[k] = v
		}
	}
}

func SyncMapLen(m *sync.Map) (len int) {
	if m != nil {
		m.Range(func(key, value interface{}) bool {
			len++
			return true
		})
	}
	return
}

//https://www.jianshu.com/p/4593cfffb9e9
func Zip(srcFiles []string, destZip string) error {
	zipfile, err := os.Create(destZip)
	if err != nil {
		return err
	}
	defer zipfile.Close()

	archive := zip.NewWriter(zipfile)
	defer archive.Close()

	for _, srcFile := range srcFiles {
		_ = filepath.Walk(srcFile, func(path string, info os.FileInfo, err error) error {
			if err != nil {
				return err
			}

			header, err := zip.FileInfoHeader(info)
			if err != nil {
				return err
			}

			path = filepath.Join(path)
			header.Name = strings.TrimPrefix(path, filepath.Dir(filepath.Join(srcFile)))

			if info.IsDir() {
				header.Name += "/"
			} else {
				header.Method = zip.Deflate
			}

			writer, err := archive.CreateHeader(header)
			if err != nil {
				return err
			}

			if !info.IsDir() {
				file, err := os.Open(path)
				if err != nil {
					return err
				}
				defer file.Close()
				_, err = io.Copy(writer, file)
			}
			return err
		})
	}
	return err
}
func Unzip(zipFile string, destDir string) error {
	zipReader, err := zip.OpenReader(zipFile)
	if err != nil {
		return err
	}
	defer zipReader.Close()

	for _, f := range zipReader.File {
		fpath := filepath.Join(destDir, f.Name)

		if f.FileInfo().IsDir() {
			os.MkdirAll(fpath, os.ModePerm)
		} else {
			if err = os.MkdirAll(filepath.Dir(fpath), os.ModePerm); err != nil {
				return err
			}

			inFile, err := f.Open()
			if err != nil {
				return err
			}
			defer inFile.Close()

			outFile, err := os.OpenFile(fpath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, f.Mode())
			if err != nil {
				return err
			}
			defer outFile.Close()

			_, err = io.Copy(outFile, inFile)
			if err != nil {
				return err
			}
		}
	}
	return nil
}

var lanIP []string

func LanIP() []string {
	if len(lanIP) > 0 {
		return lanIP
	}

	var ips []string
	netInfs, err := net.Interfaces()
	if err != nil {
		log.Println(err)
		return []string{}
	}
	for _, inf := range netInfs {
		if inf.Flags&net.FlagUp != 0 {
			match, _ := regexp.MatchString("virtualbox|vmware", strings.ToLower(inf.Name))
			if !match {
				addrs, _ := inf.Addrs()
				for _, addr := range addrs {
					if ipnet, ok := addr.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
						if ipnet.IP.To4() != nil {
							ips = append(ips, ipnet.IP.To4().String())
						}
					}
				}
			}
		}
	}

	lanIP = ips
	return lanIP
}

func ValidPath(path string) string {
	reg, _ := regexp.Compile(":")
	return reg.ReplaceAllString(path, "_")
}

//根据系统平台 .exe -> .sh
func SysExe(exe string) string {
	return exe
}

func CopyFile(dst string, src string) error {
	sf, err := os.Open(src)
	if err != nil {
		return err
	}
	defer sf.Close()
	if IsExist(dst) {
		os.Remove(dst)
	}
	err = os.MkdirAll(filepath.Dir(dst), os.ModePerm)
	if err != nil {
		return err
	}
	df, err := os.OpenFile(dst, os.O_WRONLY|os.O_CREATE, os.ModePerm)
	if err != nil {
		return err
	}

	_, err = io.Copy(df, sf)
	_ = sf.Close()
	_ = df.Close()
	return err
}

func AdminExec(file string, params string) error {
	str2ptr := func(s string) *uint16 {
		u, _ := syscall.UTF16PtrFromString(s)
		return u
	}

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
		uintptr(unsafe.Pointer(str2ptr(file))),
		uintptr(unsafe.Pointer(str2ptr(params))),
		uintptr(unsafe.Pointer(str2ptr(""))),
		0)

	if r != 42 {
		return errno
	}

	return nil
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

func HideCmd(cmd *exec.Cmd) {
	if cmd.SysProcAttr == nil {
		cmd.SysProcAttr = &syscall.SysProcAttr{}
	}
	cmd.SysProcAttr.HideWindow = true
}
