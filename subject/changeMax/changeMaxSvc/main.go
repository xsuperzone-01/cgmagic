package main

/*
#include <stdlib.h>
typedef struct {
	char *data;
	int size;
}GoMem;
 */
import "C"

import (
	"bytes"
	"changeMaxSvc/xdb"
	"changeMaxSvc/xhttp"
	"changeMaxSvc/xminio"
	"changeMaxSvc/xutil"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	osssdk "github.com/aliyun/aliyun-oss-go-sdk/oss"
	"github.com/jinzhu/gorm"
	"golang.org/x/text/encoding/simplifiedchinese"
	"io"
	"log"
	"math"
	"mime/multipart"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

type PushTo int

const (
	PushToCache PushTo = 0
	PushToSrc   PushTo = 1
)

var (
	version    = "1.0.0"
	Host       = ""
	Token      = ""
	UserId     = ""
	UserName   = ""
	DownloadDb *gorm.DB
	Ctx        context.Context
	Cancel     context.CancelFunc

	getJobM sync.Map

	autoOpenDir = true
	autoPush = true
	pushTo = PushToCache
	cacheDir string
)

//go build -buildmode=c-shared -o changeMaxSvc.dll exportgo.go
func main() {
	//var gj string
	//var err error
	//GetJobs(1, 2, &gj, &err)
	//log.Println(gj, err)
	{
		//InitChangeMaxSvc(C.CString("http://10.23.10.102"), C.CString("c5fd2c9047ab4df4a3beb5d48df5c06f"), C.CString("462127"), C.CString("C:/Users/0550SXD/AppData/Roaming/XRenderXGT/462127"))
		//var ret string
		//var err error
		//GetJobs(0, 0, &ret, &err)
		//log.Println(ret)
	}
	{
		//var err error
		//PostJobs(C.CString("D:/test/茶壶.max"), C.CString("2014"), C.CString("2010"), &err)
	}

	//var wg sync.WaitGroup
	//wg.Add(1)
	//wg.Wait()
}

//初始化环境
//host *C.char, token *C.char, userId *C.char, userDir *C.char
//export InitChangeMaxSvc
func InitChangeMaxSvc(host *C.char, token *C.char, userId *C.char, userDir *C.char) {
	if Ctx != nil {
		Cancel()
	}

	root := C.GoString(userDir)

	logFile, err := os.OpenFile(filepath.Join(root, "log-cmax.txt"), os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
	if err == nil {
		log.SetOutput(io.MultiWriter(logFile, os.Stdout))
		xutil.RedirectStderr(logFile)
	}

	Ctx, Cancel = context.WithCancel(context.Background())

	log.Println("InitChangeMaxSvc0", version, Host, Token, UserId, root)
	Host = C.GoString(host)
	Token = C.GoString(token)
	UserId = C.GoString(userId)
	log.Println("InitChangeMaxSvc1", Host, Token, UserId, root)

	DownloadDb = xdb.InitDownloadDb(filepath.Join(root, "cmax.db"))

	//推送下载
	go func() {
		tk := time.NewTicker(3 * time.Second)
		defer tk.Stop()

		var downloadM sync.Map

		for {
			select {
			case <-Ctx.Done():
				log.Println("ctx done")
				return
			case <-tk.C:
				//待上传的
				{
					uw := xdb.Download{}.ToUpload(DownloadDb)
					if uw.Valid() {
						xdb.Download{Id: uw.Id, State: xdb.Upping}.Update(DownloadDb)

						go func(uw xdb.Download) {
							var err error
							defer func() {
								log.Println(uw.Id, "上传结束", err)
								if err != nil {
									xdb.Download{Id: uw.Id, State: xdb.Upwait}.Update(DownloadDb)
								}
							}()

							ji := &jobsInfo{
								state: xdb.Upping,
							}
							getJobM.Store(uw.Id, ji)

							var res PostJobRes
							_ = json.Unmarshal([]byte(uw.PostRes), &res)

							HW := false

							if res.CloudType == "huawei" {
								HW = true

								r := xhttp.Request{
									Method: xhttp.GET, Url: Host + fmt.Sprintf("/api/v1/job/%v", res.Id),
									Sync: true,
								}.Do(nil)
								if r.Status != 200 {
									err = errors.New("GET error")
									return
								}
								_ = json.Unmarshal([]byte(r.Body), &res)

								var file *os.File
								file, err = os.Open(uw.SourceFile)
								if err != nil {
									return
								}
								defer file.Close()
								var fi os.FileInfo
								fi, err = file.Stat()
								if err != nil {
									return
								}

								//上传
								var req *http.Request
								var response *http.Response
								start := int64(-1)
								//TODO range
								{
									tmp := xdb.Download{Id: uw.Id}.Get(DownloadDb)

									log.Println(uw.Id, "获取range")
									//req, err = http.NewRequest("GET", res.DownloadSourceUrl, nil)
									//if err != nil {
									//	log.Println(err)
									//	return
									//}
									//client := xhttp.HttpClient()
									//response, err = client.Do(req)
									//if err != nil {
									//	return
									//}
									//log.Println(uw.Id, "获取range的", response.StatusCode, response.Header)
									var length int64
									//if response.StatusCode == 200 {
									//	length, _ = strconv.ParseInt(response.Header.Get("Content-Length"), 10, 64)
									//}
									length = tmp.CompleteSize
									log.Println(uw.Id, "已有大小", length, "实际大小", fi.Size(), length < fi.Size())

									if length < fi.Size() {
										start = length
									}
									//response.Body.Close()
								}

								if start != -1 {
									log.Println(uw.Id, "上传开始", "start:", start)

									if start > 0 {
										file.Seek(start, 0)
										req.Header.Add("Range", fmt.Sprintf("bytes=%d-", start))
									}

									var fileReader io.Reader
									fileReader = file

									pfi := xminio.ProFileInfo{
										Size: fi.Size(),
										Name: uw.Id,
									}

									doneCtx, doneC := context.WithCancel(context.Background())
									defer doneC()
									ji.cancel = doneC

									//进度
									bar, proCh, _ := xminio.BindProgress(doneCtx, pfi, &ji.progress)
									defer func() {
										if proCh != nil {
											close(proCh)
										}
									}()
									if bar != nil {
										proCh <- strconv.FormatInt(start, 10)
										fileReader = bar.NewProxyReader(file)
									}

									req, err = http.NewRequest("PUT", res.SignedSourceUrl, fileReader)
									req = req.WithContext(doneCtx)
									if err != nil {
										return
									}
									client := xhttp.HttpClient()
									response, err = client.Do(req)
									if err != nil {
										return
									}
									response.Body.Close()
								}
							}

							//test
							//res := PostJobRes{
							//	DownloadSourceUrl: "http://wdm-static.oss-cn-beijing.aliyuncs.com/task_ip91/AnonymousUser/test_310.max",
							//	OssAccessid: "STS.NU6BoeoLYRjzcuCzNsr2K5ved",
							//	OssAccesskeysecret: "C6QCurgP1dPH99jHsA3syBAaxy2gtEQfVsqarojSjcCk",
							//	OssSecuritytoken: "CAISrQJ1q6Ft5B2yfSjIr5aDCdXRgpN45aiRYVPynk4mfr1n2rPOhjz2IHFNfXRqBu8YsP82mWlT6/gclqVoRoReREvCKM1565kPYosEgAyH6aKP9rUhpMCPDAr6UmzrvqL7Z+H+U6mqGJOEYEzFkSle2KbzcS7YMXWuLZyOj+wIDLkQRRLqL0BgZrFsKxBltdUROFbIKP+pKWSKuGfLC1dysQcO0wEn4K+kkMqH8Uic3h+o1eMQ4p/tK4SlZI5wNYx8ZNatjbUxX6zG1j9V7AVRybx7lusG20+e747NUwAAs0veYrOOooAyFmIjOPhmQZwjhePniPh1ttbUk4nK0BtXNYlXKX+OHdn9mpSbRr3yb4tgLuulZW6qyNmKKpTurwo8eW0BNR9Ha2X8y6CGlrVHGoABTBFJLHfoSUaHcwS1il6UqbPH2KhObee5bVzQ9ODJVJHwTebp7r3OaOLSFC7vvHgFtxZ4roqB9Zye+db3qytY2LFwgOfScDMD48VOe4lHJd3wnwDv8/W02HwabOJPWKz61pE5f+MOmab91UJFUJc6y4JorZaQ3Q+wP0gpVfdJ0Lc=",
							//}

							if !HW {
								oss := res.oss()
								oss.File = uw.SourceFile
								oss.CpDir = root

								//进度统计
								pl := &xhttp.OssProgressListener{
									PeCh: make(chan osssdk.ProgressEvent),
								}
								oss.ProgressListener = pl

								defer close(pl.PeCh)
								go func() {
									tk := time.NewTicker(1 * time.Second)
									defer tk.Stop()
									for {
										select {
										case <-tk.C:
											ji.progress.GetPro()
										case pe, ok := <-pl.PeCh:
											if !ok {
												log.Println("oss upload progress event over")
												return
											}
											ji.progress.SetPro(pe.ConsumedBytes, ji.progress.TotalSize == 0)
											ji.progress.TotalSize = pe.TotalBytes
										}
									}
								}()

								err := oss.PutObject()
								log.Println("oss put:", uw.SourceFile, "=>", oss.ObjectName, err)

								if err != nil {
									//重新获取key
									if strings.Contains(err.Error(), "InvalidAccessKeyId") {
										gres := GetJob(uw.Id)
										if gres.OssAccessid != "" {
											res.OssAccessid = gres.OssAccessid
											res.OssAccesskeysecret = gres.OssAccesskeysecret
											res.OssSecuritytoken = gres.OssSecuritytoken

											b, _ := json.Marshal(res)
											xdb.Download{Id: uw.Id, PostRes: string(b)}.Update(DownloadDb)
										}
									}
									xdb.Download{Id: uw.Id, State: xdb.Upwait}.Update(DownloadDb)
									return
								}
							}

							xdb.Download{Id: uw.Id, State: xdb.Upfinish}.Update(DownloadDb)
							ji.state = xdb.Upfinish
						}(uw)
					}
				}

				//提交
				{
					uw := xdb.Download{}.ToSubmit(DownloadDb)
					if uw.Valid() {
						//通知服务开始转模
						buf := new(bytes.Buffer)
						writer := multipart.NewWriter(buf)
						_ = writer.WriteField("token", Token)
						writer.Close()

						r := xhttp.Request{
							Method: xhttp.PUT, Url: Host + fmt.Sprintf("/api/v1/job/%s/submit", uw.Id),
							Reader:      buf,
							ContentType: writer.FormDataContentType(),
							Sync:        true,
						}.Do(nil)

						if r.Status == 200 {
							xdb.Download{Id: uw.Id, State: xdb.Downwait}.Update(DownloadDb)
							getJobM.Delete(uw.Id)
						} else {
							var res ErrorInfo
							_ = json.Unmarshal(r.Body, &res)
							//max文件不存在
							if res.Error == 40403 {
								xdb.Download{Id: uw.Id, State: xdb.Upwait}.Update(DownloadDb)
							}
						}
					}
				}

				//下载
				{
					uws := xdb.Download{}.ToDownload(DownloadDb)
					for _, uw := range uws {
						if uw.Valid() {
							if _, ok := downloadM.Load(uw.Id); ok {
								continue
							}
							downloadM.Store(uw.Id, "")

							go func(uw xdb.Download) {
								defer func() {
									downloadM.Delete(uw.Id)
								}()

								res := GetJob(uw.Id)

								/**
								  2 - 排队中
								  3 - 转模中
								  4 - 转模成功
								  5 - 转模失败
								  6 - 已完成
								  7 - 已取消
								*/
								if res.Status == "5" || res.Status == "7" {
									xdb.Download{Id: uw.Id, State: xdb.Downfinish}.Update(DownloadDb)
								}
								if res.Status == "4" {
									if uw.AutoPush != 2 && !autoPush {
										xdb.Download{Id: uw.Id, AutoPush: 1}.Update(DownloadDb)
										return
									}
								}
								if res.Status == "4" || res.Status == "6" {
									log.Println("下载开始", uw.Id)
									xdb.Download{Id: uw.Id, State: xdb.Downing}.Update(DownloadDb)

									ji := &jobsInfo{
										state: xdb.Downing,
									}
									getJobM.Store(uw.Id, ji)

									var err error
									defer func() {
										log.Println("下载结束", uw.Id, err)
										if err != nil {
											xdb.Download{Id: uw.Id, State: xdb.Downwait}.Update(DownloadDb)
										}
									}()

									saveRoot := saveRoot(uw)
									xdb.Download{Id: uw.Id, SavePath: saveRoot}.Update(DownloadDb)

									realFile := filepath.Join(saveRoot, res.DownloadName)
									if xutil.IsExist(realFile) {
										realFile = filepath.Join(saveRoot, strings.TrimSuffix(filepath.Base(realFile), filepath.Ext(realFile)) + " " + xutil.ToString(time.Now().Unix()) + filepath.Ext(realFile))
									}
									file := realFile + "." + uw.Id
									log.Println("DownloadJob to local:", file)

									err = os.MkdirAll(filepath.Dir(file), os.ModePerm)
									if err != nil {
										return
									}

									var f *os.File
									f, err = os.OpenFile(file, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0666)
									if err != nil {
										return
									}
									defer f.Close()

									var fi os.FileInfo
									fi, err = f.Stat()
									if err != nil {
										return
									}

									//确保不超范围
									start := int64(math.Max(float64(fi.Size()-1), 0))
									f.Seek(start, 0)

									var body io.ReadCloser
									var header http.Header
									err, body, header = xhttp.DownloadR(context.Background(), res.SignedTargetUrl, start)
									if err != nil {
										return
									}
									defer body.Close()
									fileLength, _ := strconv.ParseInt(header.Get("Content-Length"), 10, 64)

									var fileWriter io.Writer
									fileWriter = f

									pfi := xminio.ProFileInfo{
										Size: start + fileLength,
										Name: file,
									}

									doneCtx, doneC := context.WithCancel(context.Background())
									defer doneC()

									//进度
									bar, proCh, _ := xminio.BindProgress(doneCtx, pfi, &ji.progress)
									defer func() {
										if proCh != nil {
											close(proCh)
										}
									}()
									if bar != nil {
										proCh <- strconv.FormatInt(start, 10)
										fileWriter = bar.NewProxyWriter(f)
									}

									_, err = io.Copy(fileWriter, body)
									if err != nil {
										return
									}

									buf := new(bytes.Buffer)
									writer := multipart.NewWriter(buf)
									_ = writer.WriteField("token", Token)
									writer.Close()

									r := xhttp.Request{
										Method: xhttp.PUT, Url: Host + fmt.Sprintf("/api/v1/job/%s/finish", uw.Id),
										Reader:      buf,
										ContentType: writer.FormDataContentType(),
										Sync:        true,
									}.Do(nil)

									if r.Status != 200 {
										err = errors.New(r.Msg())
										return
									}

									xdb.Download{Id: uw.Id, State: xdb.Downfinish}.Update(DownloadDb)
									ji.state = xdb.Downfinish

									f.Close()
									os.Rename(file, realFile)

									//打开本地目录
									//TODO 路径带空格的打不开 why 加""也没用
									//cmd := exec.Command("explorer.exe", fmt.Sprintf("/select,%s", realFile))
									//_, _, _ = xutil.CmdStart(cmd)
									//cmd.Wait()

									if !autoOpenDir {
										return
									}
									bat := filepath.Join(root, uw.Id+"cmaxOpenDir.bat")
									if f, err := os.Create(bat); err == nil {
										wf := []byte(realFile)
										decodeBytes, err := simplifiedchinese.GBK.NewEncoder().Bytes([]byte(wf))
										if err == nil {
											wf = decodeBytes
										}
										f.WriteString(fmt.Sprintf(`explorer.exe /select,%s`, wf))
										f.Close()

										cmd := exec.Command("cmd.exe", "/c", bat)
										cmd.SysProcAttr = &syscall.SysProcAttr{
											HideWindow: true,
										}
										_, _, _ = xutil.CmdStart(cmd)
										cmd.Wait()

										os.Remove(bat)
									} else {
										log.Println(err)
									}
								}
							}(uw)
						}
					}
				}

			}
		}
	}()
}

//设置用户名
//export SetUserName
func SetUserName(name *C.char) {
	UserName = C.GoString(name)
}

type GetJobRes struct {
	Status             string `json:"status"`
	DownloadTargetUrl  string `json:"download_target_url"`
	SignedTargetUrl    string `json:"signed_target_url"`
	DownloadName       string `json:"convert_download_file_name"`
	OssAccessid        string `json:"oss_accessid"`
	OssAccesskeysecret string `json:"oss_accesskeysecret"`
	OssSecuritytoken   string `json:"oss_securitytoken"`
}

type GetJobsRes struct {
	Rows  []map[string]interface{} `json:"rows"`
	Total int                      `json:"total"`
}

func (g *GetJobsRes) changeRows(all bool) {
	var rows []map[string]interface{}
	for _, row := range g.Rows {
		var toState string
		var ji jobsInfo

		id := xutil.ToInt(row["id"])
		status := xutil.ToString(row["status"])
		if status == "1" || status == "4" || status == "6" {
			if jii, ok := getJobM.Load(strconv.Itoa(id)); ok {
				ji = *jii.(*jobsInfo)
				state := ji.state

				switch state {
				case xdb.Downfinish:
					getJobM.Delete(strconv.Itoa(id))
				}

				toState = strconv.Itoa(state)
			}
		}
		row["toStatus"] = row["status"]
		if toState != "" {
			log.Println("change job state:", id, status, "=>", toState)
			row["toStatus"] = toState
			row["progress"] = ji.progress.Progress
			row["progressText"] = fmt.Sprintf("%d%% %s", ji.progress.Progress, ji.progress.SpeedT())
		}
		if all || toState != "" {
			rows = append(rows, row)
		}
	}
	g.Rows = rows
}

func GetJob(id string) GetJobRes {
	//先问服务要状态
	r := xhttp.Request{
		Method: xhttp.GET, Url: Host + fmt.Sprintf("/api/v1/job/%s", id),
		Query: map[string][]interface{}{
			"token": {Token},
		},
		Sync: true,
	}.Do(nil)

	if r.Status == 404 {
		if strings.Contains(string(r.Body), "40401") {
			xdb.Download{Id: id}.Delete(DownloadDb)
		}
	}

	var res GetJobRes
	_ = json.Unmarshal(r.Body, &res)

	return res
}

//转模列表
//export GetJobs
func GetJobs(page int, pageCount int, ret *C.GoMem, err *error) {
	if page == 0 {
		page = 1
	}
	if pageCount == 0 {
		pageCount = 100
	}

	r := xhttp.Request{
		Method: xhttp.GET, Url: Host + "/api/v1/jobs",
		Query: map[string][]interface{}{
			"page":   {page},
			"count":  {pageCount},
			"token":  {Token},
			"userId": {UserId},
		},
		Sync: true,
	}.Do(nil)

	if r.Status == 200 {
		var res GetJobsRes
		json.Unmarshal(r.Body, &res)
		res.changeRows(true)

		jobsResCacheM.Lock()
		jobsResCache = res
		jobsResCacheM.Unlock()

		r.Body, _ = json.Marshal(&res)

		ret.data = C.CString(string(r.Body))
		*err = nil
	} else {
		ret.data = C.CString("")
		*err = errors.New(r.Msg())
	}
}

var jobsResCache GetJobsRes
var jobsResCacheM sync.Mutex

//转模列表 待更新缓存
//ret *string
//export GetJobsCache
func GetJobsCache(ret *C.GoMem) {
	jobsResCacheM.Lock()
	tmp := jobsResCache
	tmp.changeRows(false)
	c, _ := json.Marshal(&tmp)
	jobsResCacheM.Unlock()

	ret.data = C.CString(string(c))
}

type PostJobRes struct {
	Id                 int    `json:"id"`
	DownloadSourceUrl  string `json:"download_source_url"`
	SignedSourceUrl    string `json:"signed_source_url"`
	CloudType          string `json:"cloud_type"`
	OssAccessid        string `json:"oss_accessid"`
	OssAccesskeysecret string `json:"oss_accesskeysecret"`
	OssSecuritytoken   string `json:"oss_securitytoken"`
	RemainCount        int    `json:"remainCount"` // 剩余可使用次数
	TotalCount         int    `json:"totalCount"` // 最大可使用次数
	Empower            int    `json:"empower"` // 是否授权 0:false 1:true
	Commit             int    `json:"commit"` // 是否可提交 0:false 1:true
}

func (p PostJobRes) oss() xhttp.Oss {
	//https://blog.csdn.net/flysnow_org/article/details/103521006

	var oss xhttp.Oss
	log.Println(p.DownloadSourceUrl)
	rs := regexp.MustCompile(`(http://|https://)(.*?)\.(.*?)/`).FindStringSubmatch(p.DownloadSourceUrl)
	log.Println(rs)
	if len(rs) > 1 {
		oss.Host = rs[3]
		oss.BucketName = rs[2]
		oss.ObjectName = strings.ReplaceAll(p.DownloadSourceUrl, rs[0], "")
	}
	oss.AccessKeyId = p.OssAccessid
	oss.AccessKeySecret = p.OssAccesskeysecret
	oss.SecurityToken = p.OssSecuritytoken
	return oss
}

type PutJob struct {
	Token string `json:"token"`
}

//新建转模任务，返回oss信息，上传文件，通知服务开始转模
//file *C.char, sourceVersion *C.char, targetVersion *C.char, retErr *error
//export PostJobs
func PostJobs(fileC *C.char, sourceVersionC *C.char, targetVersionC *C.char, retErr *error) {
	file := C.GoString(fileC)
	sourceVersion := C.GoString(sourceVersionC)
	targetVersion := C.GoString(targetVersionC)

	buf := new(bytes.Buffer)

	writer := multipart.NewWriter(buf)

	_ = writer.WriteField("filename", filepath.Base(file))
	_ = writer.WriteField("source_version", sourceVersion)
	_ = writer.WriteField("target_version", targetVersion)
	_ = writer.WriteField("token", Token)
	_ = writer.WriteField("userId", UserId)
	_ = writer.WriteField("cloud_type", "huawei")
	_ = writer.WriteField("from_where_submit", "cgmagic")

	writer.Close()

	log.Println(buf)
	r := xhttp.Request{
		Method: xhttp.POST, Url: Host + "/api/v1/job",
		Reader:      buf,
		ContentType: writer.FormDataContentType(),
		Sync:        true,
	}.Do(nil)

	var res PostJobRes
	_ = json.Unmarshal(r.Body, &res)

	if r.Status != 200 {
		*retErr = errors.New(r.Msg())
		return
	}

	// 埋点
	name := url.QueryEscape(UserName)
	log.Println("埋点", UserName, "->", name)
	xhttp.Request{
		Method: xhttp.GET, Url: "https://analysis.xrender.com/CG_P/G00-A01-0001",
		Header: map[string]string{
			"nickname": name,
		},
	}.Do(nil)

	xdb.Download{
		Id:         strconv.Itoa(res.Id),
		PostRes:    string(r.Body),
		SourceFile: file,
		SavePath:   filepath.Dir(file),
		State:      xdb.Upwait,
	}.Create(DownloadDb)
}

//取消转换 上传与排队阶段
//jobId int
//export CancelJob
func CancelJob(jobId int) {
	id := strconv.Itoa(jobId)
	job := GetJob(id)
	switch job.Status {
	case "1": fallthrough
	case "2":
		break
	default:
		log.Println("CancelJob ignore", "status", job.Status)
		return
	}

	r := xhttp.Request{
		Method: xhttp.PUT, Url: Host + fmt.Sprintf("/api/v1/job/%v/cancel", jobId),
		Query: map[string][]interface{}{
			"token": {Token},
		},
		Sync:        true,
	}.Do(nil)

	if r.Status == 200 {
		if v, ok := getJobM.Load(id); ok {
			ji := *v.(*jobsInfo)
			if ji.cancel != nil {
				ji.cancel()
			}
			getJobM.Delete(id)
		}

		xdb.Download{Id: id}.Delete(DownloadDb)
	}
}

//删除记录
//jobId int
//export DeleteJob
func DeleteJob(jobId int) {
	id := strconv.Itoa(jobId)
	job := GetJob(id)
	switch job.Status {
	case "4": fallthrough
	case "5": fallthrough
	case "6": fallthrough
	case "7":
		break
	default:
		log.Println("DeleteJob ignore", "status", job.Status)
		return
	}

	r := xhttp.Request{
		Method: "DELETE", Url: Host + fmt.Sprintf("/api/v1/job/%v", jobId),
		Query: map[string][]interface{}{
			"token": {Token},
		},
		Sync:        true,
	}.Do(nil)

	if r.Status == 200 {
		if v, ok := getJobM.Load(id); ok {
			ji := *v.(*jobsInfo)
			if ji.cancel != nil {
				ji.cancel()
			}
			getJobM.Delete(id)
		}

		xdb.Download{Id: id}.Delete(DownloadDb)
	}
}

//下载转模后的文件, 且本地库记录了源文件路径 通知服务下载完成
//jobId int
//export DownloadJob
func DownloadJob(jobId int) {
	resumeDownload(jobId)

	xdb.Download{Id: strconv.Itoa(jobId), State: xdb.Downwait, AutoPush: 2}.Update(DownloadDb)
}

//下载路径
//jobId int, ret *string
//export DownloadDir
func DownloadDir(jobId int, ret *C.GoMem) {
	resumeDownload(jobId)

	r := ""
	d := xdb.Download{Id: strconv.Itoa(jobId)}.Get(DownloadDb)
	if d.Valid() {
		r = d.SavePath
	}

	ret.data = C.CString(r)
}

// 支持本地无记录也能下载
func resumeDownload(jobId int) {
	d := xdb.Download{Id: strconv.Itoa(jobId)}.Get(DownloadDb)
	if d.Valid() {
		return
	}

	id := strconv.Itoa(jobId)
	res := GetJob(id)
	if res.Status == "" {
		return
	}

	xdb.Download{
		Id:       id,
		AutoPush: 2,
		SavePath: saveRoot(xdb.Download{}),
	}.Create(DownloadDb)
}

func saveRoot(uw xdb.Download) string {
	saveRoot := uw.SavePath
	if PushToCache == pushTo {
		saveRoot = cacheDir
	} else if PushToSrc == pushTo {
		srcRoot := filepath.Dir(uw.SourceFile)
		if uw.SourceFile != "" && xutil.IsExist(srcRoot) {
			saveRoot = srcRoot
		} else {
			saveRoot = cacheDir
		}
	}
	return saveRoot
}

type jobsInfo struct {
	state    int
	progress xminio.ProgressInfo
	cancel   context.CancelFunc
}

type ErrorInfo struct {
	Error   int    `json:"error"`
	Message string `json:"message"`
}

//自动推送
//v bool
//export SetAutoPush
func SetAutoPush(v bool) {
	autoPush = v
	log.Println("SetAutoPush", autoPush)
}

//自动打开下载目录
//v bool
//export SetAutoOpenDir
func SetAutoOpenDir(v bool) {
	autoOpenDir = v
	log.Println("SetAutoOpenDir", autoOpenDir)
}

//推送结果到 0:缓存 1:原路径
//v int
//export SetPushTo
func SetPushTo(v int) {
	pushTo = PushTo(v)
	log.Println("SetPushTo", pushTo)
}

//推送路径
//dir *C.char
//export SetCacheDir
func SetCacheDir(dir *C.char) {
	cacheDir = C.GoString(dir)
	log.Println("SetCacheDir", cacheDir)
}

//export Free
func Free(s *C.char) {
	C.free(unsafe.Pointer(s))
}