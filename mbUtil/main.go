package main

import (
	"flag"
	"log"
	"os"
	"strings"
	"syscall"
	"unsafe"
)

/*
// 初始化
extern "C" DLLAPI int MbUtil_Init();
extern "C" DLLAPI void MbUtil_Exit();

// 更新license
extern "C" DLLAPI void MbUtil_UpdateLicense(const wchar_t* appsdesc);

// 模型降版本(返回值: 0-成功;1-失败)
extern "C" DLLAPI int MbUtil_DowngradeMaxFile(const wchar_t* filename);
// 材质库降版本(返回值:0-成功;1-失败)
extern "C" DLLAPI int MbUtil_DowngradeMatFile(const wchar_t* filename);
// 获取Max文件版本（返回值: 0-失败）
extern "C" DLLAPI int MbUtil_QueryMaxVersion(const wchar_t* filename);

// 批量模型降版本(filenames以;分隔)
extern "C" DLLAPI void MbUtil_BatchDowngradeMaxFiles(const wchar_t* filenames);
// 批量材质库降版本(filenames以;分隔)
extern "C" DLLAPI void MbUtil_BatchDowngradeMatFiles(const wchar_t* filenames);

// 唤醒Max窗口
extern "C" DLLAPI void MbUtil_WakeupMax();

// 显示Max帧缓存
extern "C" DLLAPI void MbUtil_ShowMaxFrameBuffer();
// 显示VRay帧缓存
extern "C" DLLAPI void MbUtil_ShowVRayFrameBuffer();
// 显示Corona帧缓存
extern "C" DLLAPI void MbUtil_ShowCoronaFrameBuffer();

// 更新插件UI语言（参数lang: 0-英文; 1-中文）
extern "C" DLLAPI void MbUtil_ChangeLanguage(int lang);//不用这个方法
extern "C" DLLAPI void MbUtil_ChangeENLanguage;
extern "C" DLLAPI void MbUtil_ChangeCNLanguage;
*/

func main() {
	log.SetOutput(os.Stdout)

	flag.CommandLine = flag.NewFlagSet(os.Args[0], 10)

	d := flag.String("d", "", "dll路径")
	c := flag.String("c", "", "命令行")
	v := flag.String("v", "", "输入参数")
	flag.Parse()

	log.Println(*c, *v)
	*v = strings.ReplaceAll(*v, "\"", "")

	dll, err := syscall.LoadDLL(*d)
	if err != nil {
		log.Println("LoadDLL", err)
		os.Exit(1)
	}
	defer dll.Release()

	proc, err := dll.FindProc(*c)
	if err != nil {
		log.Println("FindProc", err)
		os.Exit(1)
	}

	var vs []uintptr
	if *v != "" {
		vp, err := syscall.UTF16PtrFromString(*v)
		if err != nil {
			log.Println("UTF16PtrFromString", err)
			os.Exit(1)
		}
		vs = append(vs, uintptr(unsafe.Pointer(vp)))
	}

	// v1是int MbUtil_QueryMaxVersion()的返回值
	v1, v2, err := proc.Call(vs...)
	log.Println(v1, v2, err)
	log.Println("mbUtilReturn", v1)
}
