package service

import (
	"encoding/json"
	"fmt"
	"github.com/gin-gonic/gin"
	"log"
	"net/http"
	"os"
	"runtime/debug"
	"softUpdate/src/config"
	"softUpdate/src/versionManager"
	"softUpdate/src/xhttp"
	"softUpdate/src/xutil"
	"strings"
	"sync"
	"time"
)

func StartHttpServer(wg *sync.WaitGroup, port string) {
	defer wg.Done()

	// 记录日志
	gin.DefaultWriter = os.Stdout

	router := initRouter()
	host := "http://127.0.0.1"
	port = ":" + port

	i := 0
	for i = 0; i < 3; i++ {
		routerOk := true
		go func() {
			<-time.NewTimer(1 * time.Second).C
			if routerOk {
				wg.Done()
				return
			}
		}()
		log.Println("开启GinPort", port)
		err := router.Run(port)
		if err != nil {
			routerOk = false
			log.Println("无法开启GinPort", port, err)
			if strings.Contains(err.Error(), "Only one usage of each socket address") {
				xhttp.Request{
					Method: xhttp.POST, Url: host + port + "/softupgrade/exitApp",
				}.Do(nil)
				time.Sleep(2 * time.Second)
			} else {
				os.Exit(0)
			}
		}
	}
	os.Exit(0)
}

//https://www.jianshu.com/p/2946513b81fa
//https://blog.csdn.net/qq_30525851/article/details/93754157
func cors() gin.HandlerFunc {
	return func(c *gin.Context) {
		method := c.Request.Method
		origin := c.Request.Header.Get("Origin")

		if origin != "" {
			c.Header("Access-Control-Allow-Origin", origin)
			//c.Header("Access-Control-Allow-Methods", "*")
			c.Header("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE")
			c.Header("Access-Control-Allow-Headers", "userType, userId, userSession, Content-Type, userName, sign")
			//c.Header("Access-Control-Allow-Headers", "*")
			c.Header("Access-Control-Expose-Headers", "*")
			c.Header("Access-Control-Max-Age", "172800")
			c.Header("Access-Control-Allow-Credentials", "true")
		}
		if method == "OPTIONS" {
			c.JSON(http.StatusOK, "Options Request!")
		}
		c.Next()
	}
}

func first() gin.HandlerFunc {
	return func(c *gin.Context) {
		if strings.Contains(c.Request.URL.Path, "exitApp") {
		} else {
			if config.First {
				c.AbortWithStatusJSON(http.StatusBadRequest, fmt.Sprintf("文件下载中%d", config.Progress))
			} else {
				if c.Request.Method == "PUT" && strings.Contains(c.Request.URL.Path, "/softupgrade/force") {
					log.Println("点击升级, 需要立刻检测")
					if versionManager.ChechCh != nil {
						versionManager.ChechCh <- ""
					}
				}
			}
		}
		c.Next()
	}
}
func resover() gin.HandlerFunc {
	return func(c *gin.Context) {
		defer func() {
			if r := recover(); r != nil {
				log.Println(r)
				log.Println(string(debug.Stack()))
			}
		}()
		c.Next()
	}
}

func initRouter() *gin.Engine {
	gin.SetMode(gin.ReleaseMode)

	//禁用控制台颜色
	gin.DisableConsoleColor()

	r := gin.Default()
	r.Use(resover())
	r.Use(cors())
	r.Use(first())

	softupgrade := r.Group("/softupgrade")
	{
		status := softupgrade.Group("/status")
		status.GET("", getStatus)
	}
	{
		force := softupgrade.Group("/force")
		force.GET("", getForce)
		force.PUT("", forceUpgrade)
	}
	{
		vers := softupgrade.Group("/versions")
		vers.GET("", getVersion)
		vers.PUT("", versionsUpgrade)
	}
	{
		update := softupgrade.Group("/updatestatus")
		update.GET("", getUpdateStatus)
	}
	{
		pro := r.Group("/softupgrade/exitApp")
		pro.POST("", exitApp)
	}

	{
		cmdLine := r.Group("/softupgrade/cmd")
		cmdLine.PUT("", goCmd)
	}

	return r
}

func getStatus(c *gin.Context) {
	log.Println("getStatus")
	state := config.GetUpdateStatus()
	if state == "success" {
		var status map[string]interface{}
		status = make(map[string]interface{})
		status["status"] = 0
		data200(c, status)
	} else {
		message400(c, "文件正在下载中")
	}
}

func getForce(c *gin.Context) {
	state := config.GetUpdateStatus()
	if state != "success" {
		message400(c, "文件正在下载中")
		return
	}

	ForceVersions := config.GetNeedForceVersion()
	data200(c, ForceVersions)
}

type isForceUpgrade struct {
	IsForceUpgrade bool `json:"isForceUpgrade"`
}

func forceUpgrade(c *gin.Context) {
	isForce := isForceUpgrade{}
	if err := c.BindJSON(&isForce); err != nil {
		message400(c, err.Error())
		return
	}

	state := config.GetUpdateStatus()
	log.Println("forceUpgrade = ", isForce.IsForceUpgrade, state)
	if isForce.IsForceUpgrade && state == "success" {
		var updateVers versionManager.Versions
		forceRecord := config.GetNeedForceVersion()
		for _, v := range forceRecord.Force {
			log.Println("forceUpgrade SoftwareId = ", v.SoftwareId, "version = ", v.Version)
			var updateVer versionManager.Version
			updateVer.Version = v.Version
			updateVer.SoftwareId = v.SoftwareId
			updateVers.Versions = append(updateVers.Versions, updateVer)
		}

		if len(updateVers.Versions) > 0 {
			jsonData, _ := json.Marshal(updateVers)
			log.Println("forceUpgrade :", string(jsonData))
			err := updateVers.Init(jsonData)
			if err != nil {
				message400(c, err.Error())
				return
			}

			versionManager.UpdateStatus = -1
			go updateVers.Update()
		} else {
			versionManager.UpdateStatus = 0
		}
	} else {
		versionManager.UpdateStatus = 0
	}

	var status map[string]int
	status = make(map[string]int)
	status["status"] = versionManager.UpdateStatus
	data200(c, status)
}

func getVersion(c *gin.Context) {
	state := config.GetUpdateStatus()
	if state != "success" {
		message400(c, "文件正在下载中")
		return
	}

	verRecord := config.GetScriptVersion()
	data200(c, verRecord)
}

func versionsUpgrade(c *gin.Context) {
	verUp := versionManager.Versions{}
	if err := c.BindJSON(&verUp); err != nil {
		message400(c, err.Error())
		return
	}

	jsonData, _ := json.Marshal(verUp)
	log.Println("versionsUpgrade = ", string(jsonData))
	err := verUp.Init(jsonData)
	if err != nil {
		message400(c, err.Error())
		return
	}
	versionManager.UpdateStatus = -1
	go verUp.Update()
	empty200(c)
	return
}

func getUpdateStatus(c *gin.Context) {
	var status map[string]int
	status = make(map[string]int)
	status["status"] = versionManager.UpdateStatus
	data200(c, status)
}

func exitApp(c *gin.Context) {
	empty200(c)
	log.Println("exitApp")
	os.Exit(0)
}

func goCmd(c *gin.Context) {
	cmdline := xutil.CmdLine{}
	if err := c.ShouldBindJSON(&cmdline); err != nil {
		message400(c, err.Error())
		return
	}

	if cmdline.RequireAdmin {
		admin(c)
		return
	}

	var ret = make(map[string]interface{})
	if cmdline.ExeDir != "" {
		log.Println("KillExeByDir", cmdline.ExeDir)
		err := xutil.KillExeByDir(cmdline.ExeDir)
		if err != nil {
			ret["err"] = err.Error()
		} else {
			ret["err"] = ""
		}
	} else {
		ret["err"] = ""
	}

	data200(c, ret)
}

func admin(c *gin.Context) {
	err := xutil.AdminExec(os.Args[0], strings.Join(os.Args[1:], " "))
	if err != nil {
		message400(c, err.Error())
		return
	}

	data200(c, "")
	c.Writer.Flush()
	os.Exit(0)
}

type XsData struct {
	ResultCode interface{} `json:"-"`
	ResultMsg  string      `json:"-"`
	ResultData interface{} `json:"-"`

	Code interface{} `json:"code"`
	Msg  string      `json:"msg"`
	Data interface{} `json:"data"`
}

type XsResp struct {
	Status  interface{} `json:"status"`
	Code    interface{} `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data"`

	Msg    string `json:"-"`
	XsData XsData `json:"-"`
}

func empty200(c *gin.Context) {
	c.JSON(http.StatusOK, XsResp{Data: XsData{Code: http.StatusOK}})
}
func data200(c *gin.Context, data interface{}) {
	c.JSON(http.StatusOK, XsResp{Data: XsData{Code: http.StatusOK, Data: data}})
}
func message404(c *gin.Context, text string) {
	c.JSON(http.StatusOK, XsResp{Data: XsData{Code: http.StatusNotFound, Msg: text}})
}
func message400(c *gin.Context, text string) {
	c.JSON(http.StatusOK, XsResp{Data: XsData{Code: http.StatusBadRequest, Msg: text}})
}
