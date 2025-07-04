package versionManager

import (
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"softUpdate/src/config"
	"softUpdate/src/xutil"
	"strconv"
	"strings"
	"time"
)

const appDirTmpFileExt = ".updatetmpfile"

type Version struct {
	SoftwareId int    `json:"softwareId"`
	Version    string `json:"version"`
}

type Versions struct {
	Versions []Version `json:"versions"`
}

var isUpgrading = false
var UpdateStatus = -1

func (this *Versions) Init(context []byte) error {
	var vers Versions
	err := json.Unmarshal(context, &vers)
	if err == nil {
		status := config.GetUpdateStatus()
		if status != "success" {
			return errors.New("文件正在下载中")
		}
		this.Versions = vers.Versions
		if len(this.Versions) == 0 {
			return errors.New("Versions is nil")
		}

		if isUpgrading {
			return errors.New("isUpgrading")
		}
	}
	return err
}

func (this *Versions) startCmd(dir string, tag string) {
	upgradeCmd := filepath.Join(dir, fmt.Sprintf("%s.bat", tag))
	if xutil.IsMac() {
		upgradeCmd = strings.ReplaceAll(upgradeCmd, ".bat", ".sh")
	}
	if xutil.IsExist(upgradeCmd) {
		cmd := exec.Command(upgradeCmd)
		xutil.HideCmd(cmd)
		b, err := cmd.CombinedOutput()
		log.Println("执行脚本:", upgradeCmd, err, string(b))
		log.Println("备份脚本:", os.Rename(upgradeCmd, upgradeCmd+".bak"))
	}
}

func (this *Versions) Update() {
	UpdateStatus = -1
	isUpgrading = true
	verRecord := config.GetScriptVersion()
	forceRecord := config.GetNeedForceVersion()

	timeStr := time.Now().Format("2006-01-02 15:04:05")
	for _, v := range this.Versions {
		fromDir := filepath.Join(config.Args.StorePath, strconv.Itoa(v.SoftwareId), v.Version)
		this.startCmd(fromDir, "upgradeBefor")
		log.Println("Update fromDir =", fromDir, "AppPath =", config.Args.AppPath)
		err := CopyDir(fromDir, config.Args.AppPath)
		log.Println("CopyDir err =", err)

		if err != nil {
			isUpgrading = false
			UpdateStatus = -2
			return
		}

		for i, x := range verRecord.Scripts {
			if x.SoftwareId == v.SoftwareId {
				x.Currentversion = v.Version
				x.Updatetime = timeStr
				verRecord.Scripts[i] = x
				break
			}
		}

		for i, x := range forceRecord.Force {
			if v.SoftwareId == x.SoftwareId {
				forceRecord.Force = append(forceRecord.Force[:i], forceRecord.Force[i+1:]...)
				break
			}
		}
	}

	config.SetScriptVersion(verRecord)
	config.SetNeedForceVersion(forceRecord)

	log.Println("升级完成 执行脚本")
	this.startCmd(config.Args.AppPath, "upgradeAfter")

	// 执行脚本
	var restart_app = false
	var restart_self = false
	for _, v := range this.Versions {
		for _, x := range verRecord.Scripts {
			if v.SoftwareId == x.SoftwareId {
				for _, y := range x.Versions {
					if v.Version == y.Version {
						for _, cmdname := range y.Actions {
							if strings.Contains(cmdname, "restart app") {
								restart_app = true
							} else if strings.Contains(cmdname, "restart self") {
								restart_self = true
							} else if strings.Contains(cmdname, "cmd") {

								cmdname = strings.ReplaceAll(cmdname, "cmd ", "")
								program := path.Join(config.Args.AppPath, cmdname)
								log.Println("program:", program)
								cmd := exec.Command(program)
								cmd.Env = os.Environ()
								_ = cmd.Start()
							} else {
								log.Println("update cmdname outside :", cmdname)
							}
						}
						break
					}
				}
				break
			}
		}
	}

	isUpgrading = false
	// define a retcode: 773 = 'r'+'e'+'s'+'t'+'a'+'r'+'t' = restart
	if restart_app {
		UpdateStatus = 1094
	} else if restart_self {
		UpdateStatus = 1199
	} else {
		UpdateStatus = 0
	}
}

func CopyDir(srcPath string, destPath string) error {
	//检测目录正确性
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
	if destInfo, err := os.Stat(destPath); err != nil {
		log.Println(err.Error())
		return err
	} else {
		if !destInfo.IsDir() {
			e := errors.New("destInfo不是一个正确的目录！")
			log.Println(e.Error())
			return e
		}
	}

	err := filepath.Walk(srcPath, func(path string, f os.FileInfo, err error) error {
		if f == nil {
			return err
		}
		if !f.IsDir() {
			destNewPath := strings.Replace(path, srcPath, destPath, -1)
			if strings.Contains(destNewPath, ".XneoUpdateTmpFile") {
				destNewPath = strings.ReplaceAll(destNewPath, ".XneoUpdateTmpFile", "")
			}
			srcMd5 := getMD5(path)
			destMd5 := getMD5(destNewPath)

			if srcMd5 == destMd5 {
				log.Println("file is same:", destNewPath, destMd5)
				return nil
			}

			//可能出现The process cannot access the file because it is being used by another process
			var ecp error
			if strings.HasSuffix(destNewPath, "cgmagic.exe") {
				ecp = errors.New("goto rename")
				log.Println("直接rename", destNewPath)
			} else {
				_, ecp = copyFile(path, destNewPath)
			}
			if ecp != nil {
				for i := 0; i < 20; i++ {
					err1 := os.Rename(destNewPath, destNewPath+appDirTmpFileExt)
					if err1 != nil {
						log.Println("os.Rename err=", err1)
						<-time.After(1 * time.Second)
					} else {
						break
					}
				}

				_, ecp = copyFile(path, destNewPath)
				if ecp != nil {
					log.Println("copyFile err=", ecp, path, destNewPath)
					return ecp
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

//生成目录并拷贝文件
func copyFile(src, dest string) (w int64, err error) {
	var srcFile *os.File
	srcFile, err = os.Open(src)
	if err != nil {
		return
	}
	defer srcFile.Close()

	var srcStat os.FileInfo
	srcStat, err = srcFile.Stat()
	if err != nil {
		return
	}

	dir := filepath.Dir(dest)
	_ = os.MkdirAll(dir, os.ModePerm)

	//mac签名可能有缓存，直接覆盖内容，会导致打开app失败。需要生成新的文件
	destName := dest
	destTmpName := dest + appDirTmpFileExt
	if xutil.IsMac() {
		os.Rename(destName, destTmpName)
	}
	defer func() {
		if xutil.IsMac() {
			if err == nil {
				err2 := os.Remove(destTmpName)
				log.Println("mac copyFile, then remove", destTmpName, err2)
			} else {
				err2 := os.Rename(destTmpName, destName)
				log.Println("mac copyFile, back name", destName, err2)
			}
		}
	}()

	var dstFile *os.File
	dstFile, err = os.OpenFile(dest, os.O_RDWR|os.O_CREATE|os.O_TRUNC, srcStat.Mode())
	if err != nil {
		return
	}
	defer dstFile.Close()

	w, err = io.Copy(dstFile, srcFile)
	return
}

func getMD5(file string) string {
	f, err := os.Open(file)
	defer f.Close()

	if err == nil {
		md5h := md5.New()
		_, _ = io.Copy(md5h, f)
		return hex.EncodeToString(md5h.Sum([]byte("")))
	} else {
		return ""
	}
}
