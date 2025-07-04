// xhttp
package xhttp

import (
	"bufio"
	"bytes"
	"changeMaxSvc/xutil"
	"encoding/json"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"runtime/debug"
	"strings"
	"sync"
	"time"
)

const (
	GET  = "GET"
	POST = "POST"
	PUT  = "PUT"
)

var ReleaseUserCh = make(chan string, 50)

var mutex sync.Mutex
var sn = 0

type Response struct {
	Succ   bool
	Status int
	Body   []byte
	Resp   *http.Response
	Err    string
}

func (r Response) Msg() string {
	if len(r.Body) != 0 {
		return string(r.Body)
	} else {
		return r.Err
	}
}

type Request struct {
	Method   string
	Url      string
	Body     interface{}
	Header   map[string]string
	Query    map[string][]interface{}
	Sync     bool
	Timeout  int
	Cookies  []http.Cookie
	UserId   string
	UserName string
	ContentType string
	Reader   io.Reader
}

func (r *Request) UserTag() string {
	tag := r.UserName
	if tag == "" {
		tag = r.UserId
		if dx := len(tag) - 8; dx >= 0 {
			tag = tag[dx:]
		}
	}
	return tag
}
func (r Request) Do(hand func(r Response)) Response {
	var ret Response

	var body io.Reader
	reqBody := ""

	if r.Reader == nil {
		if v, ok := r.Body.(string); ok {
			reqBody = v
		} else if r.Body != nil {
			//b, err := json.Marshal(r.Body)
			//if err == nil {
			//	reqBody = string(b)
			//}
			bf := bytes.NewBuffer([]byte{})
			je := json.NewEncoder(bf)
			je.SetEscapeHTML(false)
			err := je.Encode(&r.Body)
			if err == nil {
				reqBody = bf.String()
			} else {
				log.Println("xhttp json.NewEncoder", err)
			}
		}

		body = strings.NewReader(reqBody)
	} else {
		body = r.Reader
	}

	var p = url.Values{}
	for k, vs := range r.Query {
		for _, v := range vs {
			p.Add(k, xutil.ToString(v))
		}
	}
	if ps := p.Encode(); ps != "" {
		if strings.Contains(r.Url, "?") {
			r.Url = r.Url + "&" + ps
		} else {
			r.Url = r.Url + "?" + ps
		}
	}

	req, err := http.NewRequest(r.Method, r.Url, body)
	if nil != err {
		log.Printf("http.Request error - %s", err)
		ret.Err = err.Error()
		if hand != nil {
			hand(ret)
		}
		return ret
	}

	mutex.Lock()
	curSn := sn
	sn++
	mutex.Unlock()

	for k, v := range r.Header {
		req.Header[k] = []string{v}
	}

	//log.Println(req.Header)

	for _, ck := range r.Cookies {
		req.AddCookie(&ck)
	}

	log.Printf("-%d- [%s]%s %s\r\n%v\r\n%v", curSn, r.UserTag(), r.Method, req.URL.String(), req.Header.Get("Cookie"), reqBody)

	if r.ContentType != "" {
		req.Header.Set("Content-Type", r.ContentType)
	}
	//req.Header.Set("Content-Type", "application/x-www-form-urlencoded")

	client := &http.Client{}

	//if xini.ServerCfg.Proxy != "" {
	//	if !strings.Contains(r.Url, xini.ServerCfg.Master) && !strings.Contains(r.Url, xini.HOST.Local) {
	//		purl, err := url.Parse(xini.ServerCfg.Proxy)
	//		if err == nil {
	//			client = &http.Client{
	//				Transport: &http.Transport{
	//					Proxy: http.ProxyURL(purl),
	//				},
	//			}
	//		} else {
	//			log.Println("xhttp proxy parse", err)
	//		}
	//	}
	//}

	if 0 == r.Timeout {
		r.Timeout = 30
	}
	client.Timeout = time.Duration(r.Timeout) * time.Second

	//异步
	dohttp := func() {
		defer func() {
			if e := recover(); e != nil {
				log.Println("奔溃httprequest", e, string(debug.Stack()))
			}
		}()

		hresp, herr := client.Do(req)

		sse := false
		if herr == nil {
			//log.Println("cookies", hresp.Cookies())
			ret.Status = hresp.StatusCode
			if ret.Status >= 200 && ret.Status < 300 {
				ret.Succ = true
			}
			ret.Resp = hresp

			//TODO SSE 判断头
			contentType := hresp.Header.Get("Content-Type")
			//log.Println(contentType)
			if strings.Contains(strings.ToLower(contentType), "text/event-stream") {
				sse = true

				rd := bufio.NewReader(hresp.Body)
				data := ""
				for {
					line, err := rd.ReadString('\n')
					//line = strings.TrimRight(line, "\n")
					//log.Println(line)
					if err != nil || io.EOF == err {
						_ = hresp.Body.Close()
						ret.Status = -1
						ret.Err = err.Error()
						if hand != nil {
							hand(ret)
						}
						break
					}
					if line == "\n" {
						if data != "" {
							ret.Body = []byte(data)
							//log.Printf("-sse- %s\r\n", ret.Body)
							if hand != nil {
								hand(ret)
							}
						}
						data = ""
					} else {
						data += line
					}
				}
			} else {
				body, _ := ioutil.ReadAll(hresp.Body)
				ret.Body = body
				defer hresp.Body.Close()

				log.Printf("-%d- -%d- %s", curSn, ret.Status, ret.Body)

				if ret.Status == 403 && r.UserId != "" {
					if x := X(body, nil); 10403 == x.CodeI() {
						ReleaseUserCh <- r.UserId
					}
				}
			}
		} else {
			log.Printf("-%d- %s", curSn, herr.Error())
			ret.Err = herr.Error()
		}

		if !sse {
			if hand != nil {
				hand(ret)
			}
		}
	}

	if r.Sync {
		dohttp()
	} else {
		go dohttp()
	}
	return ret
}

type XsData struct {
	ResultCode interface{} `json:"-"`
	ResultMsg  string      `json:"-"`
	ResultData interface{} `json:"-"`

	Code interface{} `json:"code"`
	Msg  string      `json:"msg"`
	Data interface{} `json:"data"`
}

func (x *XsData) CodeI() (code int) {
	code = xutil.ToInt(x.ResultCode)
	if 0 == code {
		code = xutil.ToInt(x.Code)
	}
	return
}
func (x *XsData) CodeS() (code string) {
	code = xutil.ToString(x.ResultCode)
	if "" == code {
		code = xutil.ToString(x.Code)
	}
	return
}

type XsResp struct {
	Status  interface{} `json:"status"`
	Code    interface{} `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data"`

	Msg    string `json:"-"`
	XsData XsData `json:"-"`
}

func (x *XsResp) CodeI() int {
	return xutil.ToInt(x.Code)
}
func (x *XsResp) CodeS() string {
	return xutil.ToString(x.Code)
}
func X(b []byte, ds interface{}) (x XsResp) {
	err := json.Unmarshal(b, &x)
	if err != nil {
		log.Println(err)
		return x
	}

	if md, ok := (x.Data).(map[string]interface{}); ok {
		x.XsData.ResultCode = md["resultCode"]
		x.XsData.ResultMsg = xutil.ToString(md["resultMsg"])
		x.XsData.Code = md["code"]
		x.XsData.Msg = xutil.ToString(md["msg"])

		if ds != nil {
			dataFunc := func(md *map[string]interface{}, key string) (ret bool) {
				v, ok := (*md)[key]
				ret = ok && v != nil
				if ret {
					dj, err := json.Marshal(v)
					err = json.Unmarshal(dj, &ds)
					if err != nil {
						log.Println(err)
					}
				}
				return
			}
			if dataFunc(&md, "resultData") {
			} else if dataFunc(&md, "data") {
			} else {
				dj, _ := json.Marshal(x.Data)
				_ = json.Unmarshal(dj, &ds)
			}
		}
	} else if sd, ok := (x.Data).([]interface{}); ok {
		dj, err := json.Marshal(sd)
		err = json.Unmarshal(dj, &ds)
		if err != nil {
			log.Println(err)
		}
	}

	return x
}

func HttpClient() *http.Client {
	client := &http.Client{}

	//if xini.ServerCfg.Proxy != "" {
	//	purl, err := url.Parse(xini.ServerCfg.Proxy)
	//	if err == nil {
	//		client.Transport = &http.Transport{
	//			Proxy: http.ProxyURL(purl),
	//		}
	//	} else {
	//		log.Println("xhttp proxy parse", err)
	//	}
	//}

	return client
}