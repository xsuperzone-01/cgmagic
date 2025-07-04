package versionManager

import (
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"softUpdate/src/config"
	"softUpdate/src/download"
	"softUpdate/src/xhttp"
	"softUpdate/src/xutil"
	"strconv"
	"strings"
	"sync"
	"time"
)

// 更新内容
/***************************************/
type software struct {
	Id           int    `json:"id"`
	SoftwareId   int    `json:"softwareId"`
	SoftwareTag  string `json:"softwareTag"`
	PolicyUrl    string `json:"policyUrl"`
	Profile      string `json:"profile"`
	Branch       string `json:"branch"`
	Version      string `json:"version"`
	VersionType  string `json:"versionType"`
	Recall       int    `json:"recall"`
	ModifiedTime string `json:"modifiedTime"`
	ReleaseTime  string `json:"releaseTime"`
}

func (s *software) isForce() bool {
	return s.VersionType == "force"
}

type data struct {
	CallbackUrl string     `json:"callbackUrl"`
	ProductId   int        `json:"productId"`
	Branch      string     `json:"branch"`
	Url         string     `json:"url"`
	Profile     string     `json:"profile"`
	Softwares   []software `json:"softwares"`
}

type info struct {
	Result  int    `json:"result"`
	Status  int    `json:"status"`
	Details string `json:"details"`
	Data    data   `json:"data"`
}

/***************************************/

// 文件列表
/******************************************/
type file struct {
	Path string `json:"path"`
	Md5  string `json:"md5"`
}

type DownLoadFiles struct {
	Actions []string `json:"actions"`
	Files   []file   `json:"files"`
	Data    software `json:"data"`
}

/******************************************/

// releaseVersionId
/******************************************/
type releaseId struct {
	ProductId string `json:"productId"`
	VersionId int    `json:"releaseVersionId"`
}

/******************************************/

var ChechCh chan string

func Start(wg *sync.WaitGroup) {
	ChechCh = make(chan string, 1000)
	deleteTmpFile(config.Args.AppPath)

	verifyFile()

	var verM = newVersionManager()
	err := verM.Handle_Reply()

	check := func() {
		err := verM.Handle_Reply()
		if err != nil {
			<-time.After(3 * time.Second)
			ChechCh <- ""
		}
	}

	go func() {
		defer wg.Done()

		for {
			select {
			case <-time.After(time.Second * 60 * 5):
				check()
			case <-ChechCh:
				check()
			}
		}
	}()

	if err != nil {
		ChechCh <- ""
	}
}

type versionManager struct {
	verUrl         string
	policyMap      map[int][]software
	scriptsMap     map[int][]DownLoadFiles
	mutex          sync.Mutex
	totalNumber    int
	completeNumber int
	statusM        sync.Map
}

func newVersionManager() *versionManager {
	var verMan versionManager
	verMan.verUrl = config.GetVersionUrl()
	log.Println("url =", verMan.verUrl)

	verMan.policyMap = make(map[int][]software)
	verMan.scriptsMap = make(map[int][]DownLoadFiles)
	return &verMan
}

func (this *versionManager) Handle_Reply() error {
	if _, ok := this.statusM.Load("ing"); ok {
		return nil
	}
	this.statusM.Store("ing", nil)

	var err error
	config.First = true
	defer func() {
		this.statusM.Delete("ing")
		if err == nil {
			config.First = false
		}
	}()

	this.policyMap = make(map[int][]software)
	this.scriptsMap = make(map[int][]DownLoadFiles)
	this.totalNumber = 0
	this.completeNumber = 0

	var keys info
	xhttp.Request{
		Method:  xhttp.GET,
		Url:     this.verUrl + fmt.Sprintf("&stamp=%v", time.Now().Unix()),
		Sync:    true,
		Timeout: 60,
	}.Do(func(r xhttp.Response) {
		err = json.Unmarshal(r.Body, &keys)
	})

	if err != nil {
		log.Println("Handle_Reply json.Unmarshal err =", err)
		return err
	}

	if len(keys.Data.Url) == 0 || len(keys.Data.CallbackUrl) == 0 || len(keys.Data.Softwares) == 0 {
		err = errors.New("Handle_Reply get info fail")
		log.Println(err.Error())
		return err
	}

	config.SetVersionUrl(keys.Data.Url)
	config.SetCallBackUrl(keys.Data.CallbackUrl)

	var verRecord = config.GetScriptVersion()
	//全部更新
	//force和optional只需要保留最新的
	var sm sync.Map
	var softs []software
	for _, v := range keys.Data.Softwares {
		// 插件由界面程序处理
		if v.SoftwareId != config.Args.ClientId {
			continue
		}
		sm.Store(v.SoftwareId, v)
	}
	sm.Range(func(key, value interface{}) bool {
		soft, _ := value.(software)
		softs = append(softs, soft)
		return true
	})
	log.Println("筛选最新的版本后", softs)
	keys.Data.Softwares = softs

	if len(verRecord.Scripts) == 0 {
		var needForce config.ForceVersion
		for _, v := range keys.Data.Softwares {
			//if v.VersionType == "force" {
			var force config.ForceId
			force.SoftwareId = v.SoftwareId
			force.Version = v.Version
			force.SoftwareTag = v.SoftwareTag
			force.Force = v.isForce()
			needForce.Force = append(needForce.Force, force)
			//}
			this.policyMap[v.SoftwareId] = append(this.policyMap[v.SoftwareId], v)
		}
		config.SetNeedForceVersion(needForce)
	} else {
		{
			forceRecord := config.GetNeedForceVersion()
			for i := 0; i < len(forceRecord.Force); {
				var isExist = false
				for _, v := range keys.Data.Softwares {
					if forceRecord.Force[i].SoftwareId == v.SoftwareId && forceRecord.Force[i].Version == v.Version {
						isExist = true
						break
					}
				}

				if !isExist {
					forceRecord.Force = append(forceRecord.Force[:i], forceRecord.Force[i+1:]...)
				} else {
					i++
				}
			}

			config.SetNeedForceVersion(forceRecord)
		}

		// 检测SoftwareId是否被撤销
		for i := 0; i < len(verRecord.Scripts); {
			tmp := verRecord.Scripts[i]

			var isExist = false
			for _, v := range keys.Data.Softwares {
				if tmp.SoftwareId == v.SoftwareId {
					isExist = true
					break
				}
			}

			// SoftwareId 已撤销，删除本地文件
			if !isExist {
				var path = filepath.Join(config.Args.StorePath, strconv.Itoa(tmp.SoftwareId))
				if xutil.IsExist(path) {
					errrem := os.RemoveAll(path)
					if errrem != nil {
						log.Println("os.RemoveAll(path) err :", err, "path :", path)
					}
				}

				verRecord.Scripts = append(verRecord.Scripts[:i], verRecord.Scripts[i+1:]...)
			} else {
				i++
			}
		}

		// 检测版本是否被撤销
		for k, v := range verRecord.Scripts {
			softwareId := v.SoftwareId
			currver := v.Currentversion
			tag := v.SoftwareTag

			for i := 0; i < len(v.Versions); {
				var isExist = false
				var forcever string
				for _, value := range keys.Data.Softwares {
					if value.SoftwareId != softwareId {
						continue
					}

					//if value.VersionType == "force" {
					forcever = value.Version
					//}

					if value.Version == v.Versions[i].Version {
						isExist = true
						break
					}
				}

				if !isExist {
					var path = filepath.Join(config.Args.StorePath, strconv.Itoa(softwareId), v.Versions[i].Version)
					if xutil.IsExist(path) {
						errrem := os.RemoveAll(path)
						if errrem != nil {
							log.Println("os.RemoveAll(path) err :", err, "path :", path)
						}
					}

					if v.Versions[i].Version == currver {
						var forceRecord = config.GetNeedForceVersion()
						var flag = false
						for _, x := range forceRecord.Force {
							if x.SoftwareId == v.SoftwareId && x.Version == v.Versions[i].Version {
								flag = true
								break
							}
						}
						if !flag {
							var forceid config.ForceId
							forceid.Version = forcever
							forceid.SoftwareId = softwareId
							forceid.SoftwareTag = tag
							forceid.Force = v.Versions[i].IsForce()
							forceRecord.Force = append(forceRecord.Force, forceid)

							config.SetNeedForceVersion(forceRecord)
						}
					}

					v.Versions = append(v.Versions[:i], v.Versions[i+1:]...)
				} else {
					i++
				}
			}

			verRecord.Scripts[k] = v
		}

		config.SetScriptVersion(verRecord)

		for _, v := range keys.Data.Softwares {
			if !this.check_version(v.SoftwareId, v.Version) && config.GetUpdateStatus() == "success" {
				continue
			}

			//if v.VersionType == "force" {
			var forceRecord = config.GetNeedForceVersion()

			var isExist = false
			for _, x := range forceRecord.Force {
				if v.SoftwareId == x.SoftwareId && v.Version == x.Version {
					isExist = true
					break
				}
			}

			if !isExist {
				var forceid config.ForceId
				forceid.SoftwareId = v.SoftwareId
				forceid.Version = v.Version
				forceid.SoftwareTag = v.SoftwareTag
				forceid.Force = v.isForce()
				forceRecord.Force = append(forceRecord.Force, forceid)

				config.SetNeedForceVersion(forceRecord)
			}
			//}

			this.policyMap[v.SoftwareId] = append(this.policyMap[v.SoftwareId], v)
		}
	}

	jsonData, _ := json.Marshal(this.policyMap)
	log.Println("policyMap =", string(jsonData), len(this.policyMap))

	if len(this.policyMap) > 0 {
		this.downPolicy()
	}

	jsonData, _ = json.Marshal(this.scriptsMap)
	log.Println("scriptsMap =", string(jsonData))

	//程序文件下载
	if len(this.scriptsMap) > 0 {
		config.SetUpdateStatus("updating")
		this.downScripts()
		this.getDetail()
	}

	this.CheckAppVersion(config.Args.ClientId)

	// 比较完版本后再修改状态
	log.Println("GetUpdateStatus", config.GetUpdateStatus())
	if config.GetUpdateStatus() == "updating" {
		config.SetUpdateStatus("success")
		log.Println("GetUpdateStatus", config.GetUpdateStatus())
	}
	config.First = false
	return nil
}

func (this *versionManager) CheckAppVersion(softwareId int) {
	appver := config.Args.AppVersion
	verRecord := config.GetScriptVersion()
	force := config.GetNeedForceVersion()

	for i := 0; i < len(verRecord.Scripts); i++ {
		if verRecord.Scripts[i].SoftwareId != softwareId {
			continue
		}
		for j := 0; j < len(verRecord.Scripts[i].Versions); j++ {
			//if verRecord.Scripts[i].Versions[j].VersionType == "force" {
			if verRecord.Scripts[i].Versions[j].Version != appver {
				var ForceId config.ForceId
				ForceId.SoftwareId = verRecord.Scripts[i].SoftwareId
				ForceId.Version = verRecord.Scripts[i].Versions[j].Version
				ForceId.Detail = verRecord.Scripts[i].Versions[j].Detail
				ForceId.SoftwareTag = verRecord.Scripts[i].SoftwareTag
				ForceId.Force = verRecord.Scripts[i].Versions[j].IsForce()

				var flag = false
				for k := 0; k < len(force.Force); k++ {
					if force.Force[k].SoftwareId != softwareId {
						continue
					}

					force.Force[k] = ForceId
					flag = true
				}

				if !flag {
					force.Force = append(force.Force, ForceId)
				}

				verRecord.Scripts[i].Currentversion = appver
			} else {
				if verRecord.Scripts[i].Currentversion != appver {
					verRecord.Scripts[i].Currentversion = appver
					if verRecord.Scripts[i].Updatetime == "" {
						verRecord.Scripts[i].Updatetime = time.Now().Format("2006-01-02 15:04:05")
					}
				}

				// 先删除强制更新中的数据
				for k := 0; k < len(force.Force); {
					if force.Force[k].SoftwareId != softwareId {
						k++
						continue
					}
					force.Force = append(force.Force[:k], force.Force[k+1:]...)
				}
			}
			break
			//}
		}
	}

	force.Force = RemoveRepeatedElement(force.Force)

	for i, x := range verRecord.Scripts {
		x.Versions = RemoveRepeated(x.Versions)
		verRecord.Scripts[i] = x
	}

	config.SetScriptVersion(verRecord)
	config.SetNeedForceVersion(force)
}

func (this *versionManager) check_version(softwareId int, version string) bool {
	verRecord := config.GetScriptVersion()
	if len(verRecord.Scripts) == 0 {
		return true
	}

	for i := 0; i < len(verRecord.Scripts); i++ {
		if verRecord.Scripts[i].SoftwareId != softwareId {
			continue
		}
		for j := 0; j < len(verRecord.Scripts[i].Versions); j++ {
			path := filepath.Join(config.Args.StorePath, strconv.Itoa(softwareId), version)
			if xutil.IsExist(path) && version == verRecord.Scripts[i].Versions[j].Version { // 路径存在切版本一致
				return false
			}
		}
	}
	return true
}

func (this *versionManager) downPolicy() {
	for _, v := range this.policyMap {
		for _, x := range v {
			var fileinfo download.FileInfo
			fileinfo.Path = filepath.Join(config.Args.StorePath, "policy.json")
			fileinfo.Record = false
			fileinfo.Url = x.PolicyUrl
			fileinfo.StartPos = 0
			fileinfo.Md5 = ""
			var httpdown download.DownLoadInfo
			httpdown.Init(fileinfo)

			this.analysisPolicy(fileinfo.Path, x)
		}
	}
}

func (this *versionManager) analysisPolicy(file string, soft software) {
	if !xutil.IsExist(file) {
		log.Println("file is not found :", file, soft)
		return
	}

	f, err := os.Open(file)
	if err != nil {
		log.Println("analysisPolicy os.Open err =", err)
		return
	}
	defer f.Close()

	context, err := ioutil.ReadAll(f)
	if err != nil {
		log.Println("analysisPolicy ioutil.ReadAll err =", err)
		return
	}

	var downfile DownLoadFiles
	err = json.Unmarshal([]byte(context), &downfile)
	if err != nil {
		log.Println("analysisPolicy json.Unmarshal err =", err)
		return
	}

	downfile.Data = soft
	this.scriptsMap[soft.SoftwareId] = append(this.scriptsMap[soft.SoftwareId], downfile)
}

func (this *versionManager) downScripts() {
	var wg sync.WaitGroup
	for softwareId := range this.scriptsMap {
		lst := this.scriptsMap[softwareId]

		wg.Add(1)
		go this.downScriptsList(lst, softwareId, &wg)
	}

	wg.Wait()
}

func (this *versionManager) handleDownPanic(quit *sync.WaitGroup, softwareId int) {
	err := recover()
	if err != nil {
		str := fmt.Sprintf("%s,{%d:%s}", config.GetUpdateStatus(), softwareId, err.(error).Error())
		log.Println(str)
		config.SetUpdateStatus(str)
	}
	quit.Done()
}

func (this *versionManager) downScriptsList(filelists []DownLoadFiles, softwareId int, quit *sync.WaitGroup) {
	defer this.handleDownPanic(quit, softwareId)

	needDownload := func(path string) bool {
		for _, sfx := range []string{"readme.txt", "cgmagic.exe"} {
			if strings.HasSuffix(path, sfx) {
				return true
			}
		}
		return false
	}

	var httpdown download.DownLoadInfo
	for _, v := range filelists {
		this.totalNumber += len(v.Files)
		for i, x := range v.Files {
			url := strings.ReplaceAll(v.Data.PolicyUrl, "policy.json", "") + x.Path

			var fileinfo download.FileInfo
			fileinfo.SoftwareId = softwareId
			fileinfo.Url = url
			fileinfo.Path = filepath.Join(config.Args.StorePath, strconv.Itoa(softwareId), v.Data.Version, x.Path)
			fileinfo.Md5 = x.Md5
			fileinfo.StartPos = 0
			fileinfo.Record = true
			fileinfo.FileName = x.Path
			fileinfo.Version = v.Data.Version

			//比对当前运行目录文件，md5一致的可以不下载
			target := filepath.Join(config.Args.AppPath, x.Path)
			hash, _ := xutil.FileMd5(target)
			if xutil.IsMac() || i == 0 || needDownload(x.Path) || hash != x.Md5 || httpdown.ExistTmpFile(fileinfo) {
				//改成循环下载，并回显进度
				var err error
				for {
					err = httpdown.Init(fileinfo)
					if err == nil {
						break
					}
					<-time.After(3 * time.Second)
				}
			} else {
				log.Println("AppPath存在相同MD5文件:", x.Path)
			}

			this.completeNumber += 1
			log.Println("当前个数进度:", this.completeNumber, this.totalNumber)
			if this.totalNumber > 0 {
				config.Progress = this.completeNumber * 100 / this.totalNumber
			}

			//err := httpdown.Init(fileinfo)
			//if err != nil {
			//	if err.Error() == "check md5 err" {
			//		log.Println("check md5 err", err)
			//		err = httpdown.Init(fileinfo)
			//	}
			//
			//	if err != nil {
			//		log.Println("downScriptsList err=", err)
			//		panic(err)
			//	}
			//}
		}

		this.postProductId(v.Data.Id)

		{
			this.mutex.Lock()
			var verRecord = config.GetScriptVersion()
			var isExist = false
			for k, x := range verRecord.Scripts {
				if x.SoftwareId != softwareId {
					continue
				}

				var flag = false
				for _, y := range x.Versions {
					if y.Version == v.Data.Version {
						flag = true
						break
					}
				}

				if !flag {
					var ver config.Version
					ver.Version = v.Data.Version
					ver.VersionType = v.Data.VersionType
					ver.Actions = v.Actions
					verRecord.Scripts[k].Versions = append(verRecord.Scripts[k].Versions, ver)
				}

				isExist = true
				break
			}

			if !isExist {
				var scpt config.Script
				var ver config.Version
				scpt.SoftwareId = softwareId
				scpt.SoftwareTag = v.Data.SoftwareTag
				ver.Version = v.Data.Version
				ver.VersionType = v.Data.VersionType
				ver.Actions = v.Actions
				scpt.Versions = append(scpt.Versions, ver)
				verRecord.Scripts = append(verRecord.Scripts, scpt)
			}

			config.SetScriptVersion(verRecord)

			this.mutex.Unlock()
		}
	}
}

func (this *versionManager) postProductId(VersionId int) {
	CallUrl := config.GetCallBackUrl()
	if CallUrl == "" {
		log.Println("postProductId CallUrl nil")
		return
	}
	log.Println("PostProductId url = ", CallUrl, "VersionId = ", VersionId)

	xhttp.Request{
		Method: xhttp.POST,
		Url:    CallUrl,
		Body: map[string]interface{}{
			"productId":        config.Args.ProductId,
			"releaseVersionId": VersionId,
		},
		Header: map[string]string{
			"content-type": "application/json",
		},
		Sync:    true,
		Timeout: 3,
	}.Do(nil)
}

func (this *versionManager) getDetail() {
	verRecord := config.GetScriptVersion()
	for i, v := range verRecord.Scripts {
		for j, x := range v.Versions {
			if x.Detail != "" {
				continue
			}

			fileName := filepath.Join(config.Args.StorePath, strconv.Itoa(v.SoftwareId), x.Version, "readme.txt")
			file, err := os.Open(fileName)
			if err != nil {
				log.Println("getDetail Open err :", err, "file :", fileName)
				continue
			}

			buff := make([]byte, 32*1024)
			nr, ne := file.Read(buff)
			_ = file.Close()

			if ne != nil {
				log.Println("getDetail Read err :", err, "file :", fileName)
				continue
			}

			verRecord.Scripts[i].Versions[j].Detail = string(buff[:nr])
		}
	}

	config.SetScriptVersion(verRecord)

	forceRecord := config.GetNeedForceVersion()
	for i, v := range forceRecord.Force {
		if v.Detail != "" {
			continue
		}

		fileName := filepath.Join(config.Args.StorePath, strconv.Itoa(v.SoftwareId), v.Version, "readme.txt")
		file, err := os.Open(fileName)
		if err != nil {
			log.Println("getDetail force Open err :", err, "file :", fileName)
			continue
		}

		buff := make([]byte, 32*1024)
		nr, ne := file.Read(buff)
		_ = file.Close()
		if ne != nil {
			log.Println("getDetail force Read err :", err, "file :", fileName)
			continue
		}
		forceRecord.Force[i].Detail = string(buff[:nr])
	}
	config.SetNeedForceVersion(forceRecord)
}

// 删除临时文件
func deleteTmpFile(srcPath string) error {
	if srcInfo, err := os.Stat(srcPath); err != nil {
		log.Println(err.Error())
		return err
	} else {
		if !srcInfo.IsDir() {
			e := errors.New("srcPath不是一个正确的目录！")
			log.Println(e.Error())
			return e
		}
	}

	err := filepath.Walk(srcPath, func(path string, f os.FileInfo, err error) error {
		if f == nil {
			return err
		}
		if !f.IsDir() {
			path := strings.ReplaceAll(path, "\\", "/")
			if strings.Contains(path, appDirTmpFileExt) {
				errrem := os.RemoveAll(path)
				if errrem != nil {
					log.Println("os.RemoveAll(path) err :", err, "path :", path)
				}
			}
		}
		return nil
	})
	if err != nil {
		log.Printf(err.Error())
	}
	return err
}

// 校验安装目录下的文件
func verifyFile() {
	if config.GetUpdateStatus() != "success" {
		return
	}
	verRecord := config.GetScriptVersion()
	for _, v := range verRecord.Scripts {
		if v.Currentversion == "" {
			continue
		}

		path := filepath.Join(config.Args.StorePath, strconv.Itoa(v.SoftwareId), v.Currentversion)
		list, err := GetAllFile(path)
		if err != nil {
			continue
		}

		for _, x := range list {
			fileName := strings.ReplaceAll(x, path, config.Args.AppPath)
			if strings.Contains(fileName, ".XneoUpdateTmpFile") {
				fileName = strings.ReplaceAll(fileName, ".XneoUpdateTmpFile", "")
			}
			if !xutil.IsExist(fileName) {
				log.Println("VerifyFile ", x, fileName)
				_, _ = copyFile(x, fileName)
			}
		}
	}
}

func GetAllFile(pathname string) ([]string, error) {
	var list []string
	rd, err := ioutil.ReadDir(pathname)
	if err != nil {
		return list, err
	}
	for _, fi := range rd {
		if fi.IsDir() {
			tmp, err := GetAllFile(pathname + "/" + fi.Name())
			if err != nil {
				return list, err
			}
			for i := 0; i < len(tmp); i++ {
				list = append(list, tmp[i])
			}
		} else {
			list = append(list, pathname+"/"+fi.Name())
		}
	}
	return list, err
}

// 强制更新去重
func RemoveRepeatedElement(arr []config.ForceId) (newArr []config.ForceId) {
	newArr = make([]config.ForceId, 0)
	for i := 0; i < len(arr); i++ {
		repeat := false
		for j := i + 1; j < len(arr); j++ {
			if arr[i] == arr[j] {
				repeat = true
				break
			}
		}
		if !repeat {
			newArr = append(newArr, arr[i])
		}
	}
	return
}

// versions去重
func RemoveRepeated(arr []config.Version) (newArr []config.Version) {
	newArr = make([]config.Version, 0)
	for i := 0; i < len(arr); i++ {
		repeat := false
		for j := i + 1; j < len(arr); j++ {
			if arr[i].Version == arr[j].Version {
				repeat = true
				break
			}
		}
		if !repeat {
			newArr = append(newArr, arr[i])
		}
	}
	return
}
