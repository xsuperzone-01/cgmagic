package xhttp

import (
	"bytes"
	"crypto/md5"
	"encoding/hex"
	"github.com/aliyun/aliyun-oss-go-sdk/oss"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"path/filepath"
)

type Oss struct {
	AccessKeyId      string `json:"accessKeyId"`
	Policy           string `json:"policy"`
	Signature        string `json:"signature"`
	ObjectName       string `json:"objectName"`
	Host             string `json:"host"`
	Expire           string `json:"expire"`
	File             string `json:"file"`
	AccessKeySecret  string `json:"accessKeySecret"`
	SecurityToken    string `json:"securityToken"`
	BucketName       string
	CpDir            string //断点续传记录目录
	ProgressListener *OssProgressListener
}

func FormUploadOss(oss Oss) error {
	buf := new(bytes.Buffer)

	writer := multipart.NewWriter(buf)
	writer.WriteField("key", oss.ObjectName)
	writer.WriteField("OSSAccessKeyId", oss.AccessKeyId)
	writer.WriteField("policy", oss.Policy)
	writer.WriteField("Signature", oss.Signature)

	part, err := writer.CreateFormFile("file", filepath.Base(oss.File))
	if err != nil {
		return err
	}

	b, err := ioutil.ReadFile(oss.File)
	if err != nil {
		return err
	}

	part.Write(b)

	if err = writer.Close(); err != nil {
		return err
	}

	req, err := http.NewRequest("POST", oss.Host, buf)
	if err != nil {
		return err
	}

	req.Header.Add("Content-Type", writer.FormDataContentType())

	client := HttpClient()
	_, err = client.Do(req)
	if err != nil {
		return err
	}

	return nil
}

// 定义进度条监听器。
type OssProgressListener struct {
	PeCh chan oss.ProgressEvent
}

// 定义进度变更事件处理函数。
func (listener *OssProgressListener) ProgressChanged(event *oss.ProgressEvent) {
	switch event.EventType {
	case oss.TransferDataEvent:
		listener.PeCh <- *event
		//fmt.Printf("\rTransfer Data, ConsumedBytes: %d, TotalBytes %d, %d%%.",
		//	event.ConsumedBytes, event.TotalBytes, event.ConsumedBytes*100/event.TotalBytes)
	default:
	}
}

func (o Oss) PutObject() error {
	// 创建OSSClient实例。
	client, err := oss.New(o.Host, o.AccessKeyId, o.AccessKeySecret)
	if err != nil {
		return err
	}
	client.Config.SecurityToken = o.SecurityToken

	// 获取存储空间。
	bucket, err := client.Bucket(o.BucketName)
	if err != nil {
		return err
	}

	h := md5.New()
	h.Write([]byte(o.ObjectName))
	cp := filepath.Join(o.CpDir, hex.EncodeToString(h.Sum(nil))+".cp")

	// 上传本地文件。
	pl := o.ProgressListener
	if pl == nil {
		pl = &OssProgressListener{}
	}
	err = bucket.UploadFile(o.ObjectName, o.File, 1024*1024, oss.Routines(3), oss.Checkpoint(true, cp), oss.Progress(pl))
	if err != nil {
		return err
	}

	return nil
}
