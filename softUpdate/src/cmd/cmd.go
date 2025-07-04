package cmd

import (
	"github.com/urfave/cli"
	"os"
	"softUpdate/src/config"
	"softUpdate/src/service"
	"softUpdate/src/versionManager"
	"softUpdate/src/xutil"
	"sync"
)

func Cmd(c *cli.Context) error {
	if config.Uninstall {
		appPath, _ := os.Getwd()
		_ = xutil.KillExeByDir(appPath)
		config.UninstallClear()
		return nil
	}

	config.InitArgs()

	if config.Args.Kpid != 0 {
		go xutil.SelfKill(config.Args.Kpid)
	}

	var wg sync.WaitGroup

	wg.Add(1)
	go service.StartHttpServer(&wg, config.Args.GinPort)
	wg.Wait()

	wg.Add(1)
	versionManager.Start(&wg)

	wg.Wait()
	return nil
}
