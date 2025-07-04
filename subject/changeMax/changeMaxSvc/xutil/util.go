package xutil

import (
	"archive/zip"
	"context"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"github.com/shirou/gopsutil/process"
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
	"time"
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
func IsExist(path string) bool {
	_, err := os.Stat(path)
	if err != nil {
		//log.Println("xutil.IsExist", err, "os.IsExist", os.IsExist(err))
	}
	return err == nil || os.IsExist(err)
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

func AppDirPath() string {
	dir, _ := filepath.Abs(filepath.Dir(os.Args[0]))
	//dir = strings.ReplaceAll(dir, "\\", "/")
	return dir
}
func AppExe() string {
	//部分系统拿出来会带空格 导致xutil.RegisterContextMenu一直匹配不上
	//前端用spawn(path.join(exePath, "xneo"))启动时，可能不带.exe
	exe := strings.TrimRight(string(os.Args[0]), " ")
	if IsWindows() && !strings.HasSuffix(exe, ".exe") {
		exe += ".exe"
	}
	return exe
}

func IsContextCanceld(ctx context.Context) bool {
	if ctx.Err() != nil {
		if strings.Contains(ctx.Err().Error(), "context canceled") {
			return true
		}
	}
	return false
}

func IsContextDeadline(ctx context.Context) bool {
	if ctx.Err() != nil {
		if strings.Contains(ctx.Err().Error(), "context deadline") {
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

func IsWindows() bool {
	sys := runtime.GOOS
	return "windows" == sys
}

func IsLinux() bool {
	return "linux" == runtime.GOOS
}

func SysAppData() (path string) {
	if IsWindows() {
		path = os.Getenv("AppData")
		if !IsExist(path) {
			path = AppDirPath()
		}
	} else {
		path = os.Getenv("HOME")
	}
	return
}

func SysHomeDir() (path string) {
	if IsWindows() {
		path = os.Getenv("USERPROFILE")
	} else {
		path = os.Getenv("HOME")
	}
	return
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
	for _, s := range []string{"vred", "blender", "maya", "3ds", "max", "houdini", " "} {
		tag = strings.ReplaceAll(tag, s, "")
	}
	return tag
}

func FileMd5(fileName string) (string, error) {
	file, err := os.Open(fileName)
	if err != nil {
		return "", err
	}
	m5 := md5.New()
	_, err = io.Copy(m5, file)
	if err != nil {
		return "", err
	}
	md5Str := hex.EncodeToString(m5.Sum(nil))
	return md5Str, nil
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

var vps = []string{"\\", "/", ":", "*", "?", "\"", "<", ">", "|"}

func ValidPath(path string) string {
	for _, v := range vps {
		path = strings.ReplaceAll(path, v, "_")
	}
	path = strings.TrimRight(path, " ")
	return path
}

//不含 / or \\
var vpe = []string{":", "*", "?", "\"", "<", ">", "|"}

func ValidPathExcept(path string) string {
	for _, v := range vpe {
		path = strings.ReplaceAll(path, v, "_")
	}
	path = strings.TrimRight(path, " ")
	return path
}

var TssExps = []struct {
	Exp string
	Rep string
}{
	{Exp: "\\s+/", Rep: "/"},
	{Exp: "\\s+\\\\", Rep: "\\"},
}

//去除 / or \\ 前的空格
func TrimSpaceSeparator(path string) string {
	for _, e := range TssExps {
		reg, err := regexp.Compile(e.Exp)
		if err != nil {
			log.Println(err)
			continue
		}
		path = reg.ReplaceAllString(path, e.Rep)
	}
	return path
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

const (
	SW_HIDE   = 0
	SW_NORMAL = 1
)

type ShellExecute struct {
	FileName   string
	Parameters string
	Directory  string
	ShowCmd    int
}

func Lang() string {
	lang := "cn"

	home, _ := os.UserHomeDir()
	file := filepath.Join(home, "xneo install tag.json")
	b, err := ioutil.ReadFile(file)
	if err == nil {
		m := make(map[string]interface{})
		if err = json.Unmarshal(b, &m); err == nil {
			lang = ToString(m["lang"])
		}
	}
	return lang
}

type ProcessStruct struct {
	ProcessName     string // 进程名称
	ProcessID       int    // 进程id
	ParentProcessID int    //父进程id
}

type ProcessStructSlice []ProcessStruct

func (a ProcessStructSlice) Len() int { // 重写 Len() 方法
	return len(a)
}
func (a ProcessStructSlice) Swap(i, j int) { // 重写 Swap() 方法
	a[i], a[j] = a[j], a[i]
}
func (a ProcessStructSlice) Less(i, j int) bool { // 重写 Less() 方法， 从大到小排序
	if strings.Compare(a[j].ProcessName, a[i].ProcessName) < 0 {
		return true
	} else {
		return false
	}
}

//重试 fn返回true则跳出
func RetryFunc(fn func() bool) {
	if fn != nil {
		for i := 1; i <= 5; i++ {
			if i > 1 {
				<-time.NewTimer(time.Duration(i) * time.Second).C
			}
			if fn() {
				break
			}
		}
	}
}

func ToSysPath(path string) string {
	if IsLinux() {
		path = ToSingleFSlash(path)
	}
	return path
}

type FrontMessage struct {
	Code int    `json:"code"`
	Msg  string `json:"msg"`
	Type string `json:"type"` //success error等
}

func SelfKill(pid int)  {
	tk := time.NewTicker(3*time.Second)
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

func KillExe(kexe string) error {
	kexe = ToSingleFSlash(kexe)

	pros, err := process.Processes()
	if err != nil {
		return err
	}

	//最好是管理员权限
	for _, pro := range pros {
		exe, err := pro.Exe()
		if err != nil {
			log.Println("pro.Exe", err.Error())
			continue
		}
		exe = ToSingleFSlash(exe)
		if kexe == exe {
			err := pro.Terminate()
			log.Println("pro.Terminate", exe, err)
		}
	}
	return nil
}

//未指定stdin 且os.DevNull未启动时(检测：sc query null, sc stop null, sc start null)
func CmdStart(cmd *exec.Cmd) (stdout, stderr io.ReadCloser, err error) {
	stdout, err = cmd.StdoutPipe()
	if err != nil {
		return
	}

	stderr, err = cmd.StderrPipe()
	if err != nil {
		return
	}

	err = cmd.Start()
	if err != nil {
		if strings.Contains(err.Error(), "open NUL: The system cannot find the file specified") {
			var r, w *os.File
			r, w, err = os.Pipe()
			if err != nil {
				return
			}
			defer r.Close()
			defer w.Close()
			cmd.Stdin = r

			cmd.Stdout = nil
			cmd.Stderr = nil
			cmd.Process = nil

			stdout, err = cmd.StdoutPipe()
			if err != nil {
				return
			}

			stderr, err = cmd.StderrPipe()
			if err != nil {
				return
			}

			err = cmd.Start()
			if err != nil {
				return
			}
		}
	}

	return
}