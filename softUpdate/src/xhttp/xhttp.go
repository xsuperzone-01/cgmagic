// xhttp
package xhttp

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"softUpdate/src/xutil"
	"strings"
	"sync"
	"time"
)

const (
	GET  = "GET"
	POST = "POST"
	PUT  = "PUT"
)
var ProxyServer = ""

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
	Method  string
	Url     string
	Body    interface{}
	Header  map[string]string
	Query   map[string][]interface{}
	Sync    bool
	Timeout int
	Cookies []http.Cookie
}

func (r Request) Do(hand func(r Response)) Response {
	var ret Response

	reqBody := ""
	if v, ok := r.Body.(string); ok {
		reqBody = v
	} else if r.Body != nil {
		b, err := json.Marshal(r.Body)
		if err == nil {
			reqBody = string(b)
		}
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

	req, err := http.NewRequest(r.Method, r.Url, strings.NewReader(reqBody))
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

	log.Printf("-%d- %s %s\r\n%v\r\n%v", curSn, r.Method, req.URL.String(), req.Header.Get("Cookie"), reqBody)

	// req.Header.Set("Content-Type", "application/x-www-form-urlencoded")

	client := &http.Client{}
	// 访问本地接口不需要代理
	if ProxyServer != "" && !strings.Contains(r.Url, "http://127.0.0.1") {
		server := make(map[string]string)
		_ = json.Unmarshal([]byte(ProxyServer), &server)
		proxy, ok := server["proxy"]
		if ok && proxy != "" {
			log.Println("open proxy =", proxy)
			purl, _ := url.Parse(proxy)
			client = &http.Client{
				Transport: &http.Transport{
					Proxy: http.ProxyURL(purl),
				},
			}
		}
	}


	if 0 != r.Timeout {
		client.Timeout = time.Duration(r.Timeout) * time.Second
	}

	//异步
	dohttp := func() {
		hresp, herr := client.Do(req)

		sse := false
		if herr == nil {
			defer hresp.Body.Close()

			//log.Println("cookies", hresp.Cookies())
			ret.Status = hresp.StatusCode
			if ret.Status >= 200 && ret.Status < 300 {
				ret.Succ = true
			}
			ret.Resp = hresp

			//TODO SSE 判断头
			contentType := hresp.Header.Get("Content-Type")
			log.Println("contentType =", contentType)
			ct := strings.ToLower(contentType)
			if strings.Contains(ct, "application/json") || strings.Contains(ct, "text/html") {
				body, _ := ioutil.ReadAll(hresp.Body)
				ret.Body = body

				log.Printf("-%d- -%d- %s", curSn, ret.Status, ret.Body)
			} else {
				log.Printf("-%d- -%d-", curSn, ret.Status)
				sse = true
				if hand != nil {
					hand(ret)
				}
			}
		} else {
			log.Println(herr.Error())
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
