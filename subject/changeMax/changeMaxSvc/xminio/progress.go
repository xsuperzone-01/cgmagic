package xminio

import (
	"bytes"
	"changeMaxSvc/xutil"
	"context"
	"github.com/cheggaaa/pb/v3"
	"io"
	"log"
	"strconv"
	"time"
)

type ProgressInfo struct {
	speeds       []int64
	Progress     int
	Speed        int64
	SpeedText    string
	oldSize      int64
	newSize      int64
	currentSize  int64
	CompleteSize int64
	TotalSize    int64
}

//----暂未完成---
//自定义进度条读取
type ProgressReader struct {
	io.Reader
	bar *ProgressInfo
}

func (p *ProgressInfo) ProxyReader(rr io.Reader) *ProgressReader {
	return &ProgressReader{rr, p}
}

// Read reads bytes from wrapped reader and add amount of bytes to progress bar
func (r *ProgressReader) Read(p []byte) (n int, err error) {
	n, err = r.Reader.Read(p)
	//r.bar.SetPro(int64(n), 0 ,false)
	return
}

// Close the wrapped reader when it implements io.Closer
func (r *ProgressReader) Close() (err error) {
	if closer, ok := r.Reader.(io.Closer); ok {
		return closer.Close()
	}
	return
}

//-------------

type ProFileInfo struct {
	Size int64
	Name string
}

func (p *ProgressInfo) SetPro(complete int64, isRecord bool) {
	if p.speeds == nil {
		p.speeds = make([]int64, 8, 8)
	}
	if isRecord {
		p.currentSize += complete
		p.CompleteSize = p.currentSize
	} else {
		p.CompleteSize = complete
		if complete >= p.currentSize {
			p.newSize = complete - p.currentSize
			p.speeds[len(p.speeds)-1] = p.newSize - p.oldSize
		}
	}
}
func (p *ProgressInfo) GetPro() {
	var sum int64
	if p.speeds != nil {
		for _, x := range p.speeds {
			sum += x
		}
		if len := len(p.speeds); len > 0 {
			sum /= int64(len)
		}

		p.speeds = p.speeds[1:]
		p.speeds = append(p.speeds, 0)
	}

	p.oldSize = p.newSize

	p.Speed = sum
	p.SpeedT()

	if p.TotalSize != 0 {
		p.Progress = int(p.CompleteSize * 100 / p.TotalSize)
	}
}
func (p *ProgressInfo) SpeedT() string {
	p.SpeedText = xutil.FormatSize(p.Speed) + "/s"
	return p.SpeedText
}
func limitSpeed(speed *int64) (outSpeed int64) {
	outSpeed = 1000 * 1024 * 1024
	if speed != nil && *speed > 0 {
		outSpeed = *speed
	}
	return
}

func BindProgress(ctx context.Context, fileInfo ProFileInfo, hpro *ProgressInfo) (bar *pb.ProgressBar, proCh chan string, retryCh chan string) {
	if hpro != nil {
		//Example: 'Prefix 20/100 [-->______] 20% 1 p/s ETA 1m Suffix'
		protemp := `{{string . "prefix"}}{{counters . }} {{percent . }} {{speed . }}{{string . "suffix"}}`
		bar = pb.New64(fileInfo.Size).SetTemplateString(protemp).SetWriter(bytes.NewBuffer([]byte("")))
		proCh = make(chan string)
		retryCh = make(chan string)
		hpro.TotalSize = fileInfo.Size
		go func() {
			speedOut := 0
			tk := time.NewTicker(1 * time.Second)
			defer tk.Stop()
			defer func() {
				bar.Finish()
				close(retryCh)
			}()
			for {
				select {
				case <-tk.C:
					hpro.SetPro(bar.Current(), false)
					hpro.GetPro()
					log.Println("-TS-", fileInfo.Name, hpro.Progress, hpro.SpeedText, bar.String())

					if hpro.Speed == 0 {
						if speedOut++; speedOut >= 20 {
							//if hpro.Progress >= 100 {
							//	retryCh <- "100"
							//} else {
							//	retryCh <- "0"
							//}
						}
					} else {
						speedOut = 0
					}
				case s := <-proCh:
					add, _ := strconv.ParseInt(s, 10, 64)
					bar.Add64(add)
					hpro.SetPro(add, true)
				case <-ctx.Done():
					return
				}
			}
		}()
		bar.Start()
	}
	return bar, proCh, retryCh
}
