package config

import (
	"bufio"
	"encoding/json"
	"fmt"
	"github.com/jinzhu/gorm"
	"gopkg.in/ini.v1"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"softUpdate/src/xdb"
	"softUpdate/src/xhttp"
	"softUpdate/src/xutil"
	"strings"
	"sync"
	"time"
)

// 存储内容
/***************************************/
type Version struct {
	Actions     []string `json:"actions"`
	Version     string   `json:"version"`
	VersionType string   `json:"versionType"`
	Detail      string   `json:"detail"`
}

func (v *Version) IsForce() bool {
	return v.VersionType == "force"
}

type Script struct {
	Currentversion string    `json:"currentversion"`
	SoftwareId     int       `json:"softwareId"`
	Updatetime     string    `json:"updatetime"`
	SoftwareTag    string    `json:"softwareTag"`
	Versions       []Version `json:"versions"`
}

type Scripts struct {
	Scripts []Script `json:"scripts"`
}

/***************************************/

// ForceVersion
/***************************************/
type ForceId struct {
	SoftwareId  int    `json:"softwareId"`
	Version     string `json:"version"`
	Detail      string `json:"detail"`
	SoftwareTag string `json:"softwareTag"`
	Force       bool   `json:"force"`
}
type ForceVersion struct {
	Force []ForceId `json:"force"`
}

/***************************************/

type args struct {
	StorePath      string
	AppPath        string
	LogsPath       string
	Brance         string
	ProductId      string
	Profiles       string
	ClientId       int
	AppVersion     string
	GinPort        string
	Kpid           int
	UpdaterVersion string
	Host           string
}

var Args args
var IsAdmin bool
var Uninstall = false
var mutex sync.Mutex
var configFile = ""
var First = true
var Progress int

var db *gorm.DB

func InitArgs() {
	//windows端的admin改由前端调接口触发，并兼容老版本
	if Args.UpdaterVersion == "" && !IsAdmin && xutil.IsWindows() {
		params := ""
		for i := 1; i < len(os.Args); i++ {
			if !strings.Contains(os.Args[i], "-isadmin") {
				params += os.Args[i] + " "
			}
		}
		params += "-isadmin=true"
		err := xutil.AdminExec(os.Args[0], params)
		log.Println(err)
		os.Exit(0)
		return
	}

	Args.AppPath = xutil.AppDirPath()
	setStorePath()

	_ = os.MkdirAll(Args.StorePath, os.ModePerm)
	_ = os.MkdirAll(Args.LogsPath, os.ModePerm)

	configFile = filepath.Join(Args.AppPath, "config.ini")

	db = xdb.InitDownloadDb(filepath.Join(Args.StorePath, "download_"+Args.Profiles+".db"))
	if db != nil {
		xdb.Download{ProductId: Args.ProductId, Profiles: Args.Profiles, Brance: Args.Brance}.Create(Args.ProductId, db)
	}

	initlog()

	SingleApp()

	log.Println("------NEW RUN------")
	log.Println(Args)

	//安装程序里面，对安装目录的权限提升，不完全，这里补一下，后期可删除
	//当界面以admin启动此exe时有效，但是启动中的xdemo.exe和此exe无法生效？
	if xutil.IsWindows() {
		go func() {
			cmd := exec.Command("icacls", Args.AppPath, "/grant", "EveryOne:F", "/t", "/C")
			xutil.HideCmd(cmd)
			cmd.Run()
		}()
	}
}

func setStorePath() {
	path := xutil.SysAppData()
	Args.StorePath = filepath.Join(path, "XcgmagicSoftUpgrade", "soft", Args.ProductId, Args.Profiles)
	Args.LogsPath = Args.StorePath + "-logs"
}

func UninstallClear() {
	setStorePath()
	log.Println(os.RemoveAll(Args.StorePath))
}

// 日志
func initlog() {
	var logFile *os.File
	{
		relog := func(time time.Time) {
			log.SetOutput(os.Stdout)
			if logFile != nil {
				_ = logFile.Close()
			}
			logFile, _ = os.OpenFile(filepath.Join(Args.LogsPath, "log-"+time.Format("2006-01-02")+".txt"), os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
			log.SetOutput(io.MultiWriter(logFile, os.Stdout))
			xutil.RedirectStderr(logFile)
		}

		log.SetPrefix("[X] ")
		log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)
		now := time.Now()
		relog(now)

		go func() {
			tk := time.NewTicker(5 * time.Second)
			for {
				select {
				case <-tk.C:
					//清理日志
					if time.Now().Day() != now.Day() {
						fs, _ := ioutil.ReadDir(Args.LogsPath)
						for _, v := range fs {
							if !v.IsDir() && strings.HasPrefix(v.Name(), "log-") && time.Now().Sub(v.ModTime()).Hours() > 15*24 {
								_ = os.Remove(filepath.Join(Args.LogsPath, v.Name()))
							}
						}
					}

					if time.Now().Day() != now.Day() {
						now = time.Now()
						relog(now)
					}
				}
			}
		}()
	}
}

// 单例
func SingleApp() {
	homedir, err := os.UserHomeDir()
	if err != nil {
		log.Println("singleApp UserHomeDir err =", err)
		return
	}

	singleFile := ""
	if Args.ProductId == "4" { // 影视客户端部
		singleFile = filepath.Join(homedir, "XneoSoftUpdateGinPort")
	} else if Args.ProductId == "5" { // 网盘
		singleFile = filepath.Join(homedir, "XneoNetDiskSoftUpdateGinPort")
	} else {
		singleFile = filepath.Join(homedir, "SoftUpdateGinPort_"+Args.ProductId)
	}

	if xutil.IsExist(singleFile) {
		file, err := os.OpenFile(singleFile, os.O_RDONLY, 0666)
		if err != nil {
			log.Println("singleApp OpenFile err =", err)
			return
		}
		defer file.Close()

		rd := bufio.NewReader(file)
		context, _, err := rd.ReadLine()
		if err != nil {
			log.Println("singleApp ReadLine err =", err)
			return
		}

		ginPort := string(context)
		log.Println("singleApp ginPort =", ginPort, "config.Args.GinPort =", Args.GinPort)
		if ginPort == Args.GinPort {
			return
		}

		host := "http://127.0.0.1"
		port := ":" + ginPort
		xhttp.Request{
			Method: xhttp.POST, Url: host + port + "/softupgrade/exitApp",
		}.Do(nil)
		time.Sleep(5 * time.Second)
	}

	filew, err := os.OpenFile(singleFile, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0666)
	if err != nil {
		log.Println("singleApp OpenFile err =", err)
		return
	}
	defer filew.Close()

	nw, err := filew.Write([]byte(Args.GinPort))
	if err != nil {
		log.Println("singleApp file.Write err =", err, nw)
		return
	}
}

func readUserIni(file, section, key string) string {
	mutex.Lock()
	defer mutex.Unlock()
	if cfg, ok := load(file); ok {
		return cfg.Section(section).Key(key).Value()
	}
	return ""
}
func saveUserIni(file, section, key, value string) {
	mutex.Lock()
	defer mutex.Unlock()
	if cfg, ok := load(file); ok {
		cfg.Section(section).Key(key).SetValue(value)
		cfg.SaveTo(file)
	}
}
func load(file string) (*ini.File, bool) {
	cfg, err := ini.LooseLoad(file)
	if err != nil {
		log.Println("打开", file, err)
	}
	return cfg, err == nil
}

func GetCustomInfo() string {
	cfg, err := ini.Load(configFile)
	if err != nil {
		log.Println(err)
		return ""
	}

	var custom map[string]string
	custom = make(map[string]string)

	Section := cfg.Section("customization")
	for i := 0; i < len(Section.Keys()); i++ {
		key := Section.Keys()[i].Name()
		value := Section.Keys()[i].Value()
		if key != "" && value != "" {
			custom[key] = value
		}
	}

	if len(custom) != 0 {
		mjson, _ := json.Marshal(custom)
		mString := string(mjson)
		return mString
	}
	return ""
}

func GetVersionUrl() string {
	var value string
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		value = ret.Url
	}
	//value = "http://update.xrender.com"
	//if IsEN2() {
	//	value = "http://update.xrender.cloud"
	//}
	value = Args.Host
	value += "/release-version/product"

	value += "?productId=%s&branch=%s&profile=%s"
	value = fmt.Sprintf(value, Args.ProductId, Args.Brance, Args.Profiles)
	custom := GetCustomInfo()
	if custom != "" {
		value += "&customization=" + custom
	}
	return value
}

func SetVersionUrl(url string) {
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		ret.Url = url
		ret.Update(db)
	}
}

func SetCallBackUrl(url string) {
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		ret.CallbackUrl = url
		ret.Update(db)
	}
}

func GetCallBackUrl() string {
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		return ret.CallbackUrl
	}
	return ""
}

func SetScriptVersion(ver Scripts) {
	jsonStr, _ := json.Marshal(ver)
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		ret.Version = string(jsonStr)
		ret.Update(db)
	}
}

func GetScriptVersion() Scripts {
	var verRecord Scripts
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		_ = json.Unmarshal([]byte(ret.Version), &verRecord)
	}
	return verRecord
}

func SetNeedForceVersion(ver ForceVersion) {
	jsonStr, _ := json.Marshal(ver)
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		ret.ForceVersion = string(jsonStr)
		ret.Update(db)
	}
}

func GetNeedForceVersion() ForceVersion {
	var force ForceVersion
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		_ = json.Unmarshal([]byte(ret.ForceVersion), &force)
	}
	return force
}

func SetUpdateStatus(status string) {
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		ret.Status = status
		ret.Update(db)
	}
}

func GetUpdateStatus() string {
	if db != nil {
		ret := xdb.Download{ProductId: Args.ProductId}.Get(db)
		return ret.Status
	}
	return ""
}
