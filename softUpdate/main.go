package main

import (
	"github.com/urfave/cli"
	"os"
	"softUpdate/src/cmd"
	"softUpdate/src/config"
	"softUpdate/src/xhttp"
)

func main() {
	app := cli.NewApp()
	app.Name = "softUpdate"
	app.Version = "1.0.4"
	app.Usage = "Software upgrade tools"
	app.Flags = []cli.Flag{
		cli.StringFlag{
			Name:        "ginPort",
			Value:       "47150",
			Usage:       `Specify service port`,
			Destination: &config.Args.GinPort,
		},
		cli.StringFlag{
			Name:        "Brance",
			Value:       "windows",
			Usage:       `platform`,
			Destination: &config.Args.Brance,
		},
		cli.StringFlag{
			Name:        "ProductId",
			Value:       "-1000",
			Usage:       "ProductId",
			Destination: &config.Args.ProductId,
		},
		cli.StringFlag{
			Name:        "Profiles",
			Value:       "prod",
			Usage:       `Branch type`,
			Destination: &config.Args.Profiles,
		},
		cli.IntFlag{
			Name:        "clientId",
			Value:       0,
			Usage:       `clientId`,
			Destination: &config.Args.ClientId,
		},
		cli.StringFlag{
			Name:        "AppVersion",
			Value:       "1.0.0",
			Usage:       `app version`,
			Destination: &config.Args.AppVersion,
		},
		cli.IntFlag{
			Name:        "Kpid",
			Value:       0,
			Usage:       `parent pid`,
			Destination: &config.Args.Kpid,
		},
		cli.StringFlag{
			Name:        "Server",
			Value:       "",
			Usage:       `ProxyServer`,
			Destination: &xhttp.ProxyServer,
			Required:    false,
		},
		cli.BoolFlag{
			Name:        "isadmin",
			Destination: &config.IsAdmin,
			Required:    false,
		},
		cli.BoolFlag{
			Name:        "Uninstall",
			Destination: &config.Uninstall,
			Required:    false,
		},
		cli.StringFlag{
			Name:        "updaterVersion",
			Destination: &config.Args.UpdaterVersion,
			Required:    false,
		},
		cli.StringFlag{
			Name:        "host",
			Destination: &config.Args.Host,
			Required:    true,
		},
	}

	app.Action = cmd.Cmd

	app.Run(os.Args)
}
