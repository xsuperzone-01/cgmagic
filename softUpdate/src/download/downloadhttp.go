package download

import (
	"crypto/md5"
	"encoding/hex"
	"errors"
	"io"
	"log"
	"os"
	"path/filepath"
	"softUpdate/src/xhttp"
	"softUpdate/src/xutil"
	"strings"
)

type FileInfo struct {
	Path       string
	Url        string
	Md5        string
	StartPos   int64
	Record     bool
	Version    string
	FileName   string
	SoftwareId int
}

const tmpFileExt = ".XneoUpdateTmpFile"

type DownLoadInfo struct {
	fileinfo FileInfo
}

//继续下载，Continue = true，断点续传;Continue = false，重新下载
func (this *DownLoadInfo) Again(Continue bool) error {
	if !Continue {
		this.fileinfo.StartPos = 0
		_ = os.Remove(this.fileinfo.Path)
	}

	var info = this.fileinfo
	return this.Init(info)
}

func (this *DownLoadInfo) ExistTmpFile(info FileInfo) bool {
	return xutil.IsExist(info.Path + tmpFileExt)
}

func (this *DownLoadInfo) Init(info FileInfo) error {
	this.fileinfo = info
	this.fileinfo.Path = strings.ReplaceAll(this.fileinfo.Path, "//", "/")
	this.fileinfo.Path = strings.ReplaceAll(this.fileinfo.Path, "\\", "/")
	this.fileinfo.Url = strings.ReplaceAll(this.fileinfo.Url, "\\", "/")

	if xutil.IsExist(this.fileinfo.Path) {
		log.Println("文件存在", this.fileinfo.Path)
		hashCode := this.fileMD5(this.fileinfo.Path)
		if hashCode == this.fileinfo.Md5 {
			return nil
		}

		_ = os.Remove(this.fileinfo.Path)
	}

	// 中间文件，防止下载时特殊文件被杀毒软件查杀
	this.fileinfo.Path += tmpFileExt

	// 校验文件信息
	if xutil.IsExist(this.fileinfo.Path) { // 文件存在
		log.Println(".XneoUpdateTmpFile 文件存在", this.fileinfo.Path)

		hashCode := this.fileMD5(this.fileinfo.Path)
		if hashCode == this.fileinfo.Md5 {
			log.Println("hashCode is same to rename", this.fileinfo.Path)
			name := strings.ReplaceAll(this.fileinfo.Path, tmpFileExt, "")
			_ = os.Rename(this.fileinfo.Path, name)
			return nil
		}

		if !this.fileinfo.Record {
			this.fileinfo.StartPos = 0
		} else {
			result := getFileSize(this.fileinfo.Path)
			// 防止下载出错时，把错误信息写到文件中
			if result > 1000 {
				result -= 1000
			} else {
				result = 0
			}
			this.fileinfo.StartPos = result
		}

		if this.fileinfo.StartPos == 0 {
			_ = os.Remove(this.fileinfo.Path)
		}
	} else if this.fileinfo.StartPos != 0 {
		this.fileinfo.StartPos = 0
	}

	dirpath := filepath.Dir(this.fileinfo.Path)
	_, direrr := os.Stat(dirpath)
	if direrr != nil { // 文件夹不存在，创建目录
		_ = os.MkdirAll(dirpath, os.ModePerm)
	}

	return this.requestData()
}

func getFileSize(filename string) int64 {
	var result int64
	filepath.Walk(filename, func(path string, f os.FileInfo, err error) error {
		result = f.Size()
		return nil
	})
	return result
}

func (this *DownLoadInfo) requestData() (retErr error) {
	//obs用range下载0kb文件会报范围错误，直接全覆盖
	//var Range string
	//Range = fmt.Sprintf("bytes=%d-", this.fileinfo.StartPos)
	//log.Println("requestData Range :", Range)
	xhttp.Request{
		Method: xhttp.GET,
		Url:    this.fileinfo.Url,
		Header: map[string]string{
			"content-type": "application/json",
			"Referer":      "www.xrender.net",
			//"Range":        Range,
		},
		Sync: true,
	}.Do(func(r xhttp.Response) {
		newFile, err := os.OpenFile(this.fileinfo.Path, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, os.ModePerm)
		retErr = err
		if err != nil {
			log.Println("requestData os.OpenFile err=", err)
			return
		}
		defer newFile.Close()
		//_, err = newFile.Seek(this.fileinfo.StartPos, 0)
		//retErr = err
		//if err != nil {
		//	log.Println("newFile.Seek err:", err)
		//	log.Println("close file:", newFile.Close())
		//	return
		//}

		if len(r.Body) > 0 {
			_, err = newFile.Write(r.Body)
		} else {
			if r.Resp == nil {
				retErr = errors.New("r.Resp is nil")
				return
			}
			_, err = io.Copy(newFile, r.Resp.Body)
		}
		retErr = err
		if err != nil {
			if err != io.EOF {
				log.Println("requestData io.Copy err=", err)
				log.Println("close file:", newFile.Close())
				return
			}
			err = nil
			retErr = err
		}

		err = newFile.Close()
		retErr = err
		if err != nil {
			log.Println("newFile.Close() err=", err)
			return
		}

		hashCode := this.fileMD5(this.fileinfo.Path)
		if this.fileinfo.Md5 != "" && hashCode != this.fileinfo.Md5 {
			err = errors.New("check md5 err")
			retErr = err
			return
		}

		name := strings.ReplaceAll(this.fileinfo.Path, tmpFileExt, "")
		err = os.Rename(this.fileinfo.Path, name)
		retErr = err

		if err != nil {
			log.Println("os.Rename err=", err)
		}
	})

	log.Println("download file:", this.fileinfo.Path, "err:", retErr)
	return retErr
}

func (this *DownLoadInfo) fileMD5(file string) string {
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
