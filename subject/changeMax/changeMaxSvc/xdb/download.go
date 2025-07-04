package xdb

import (
	"github.com/jinzhu/gorm"
	"sync"
)

const (
	Preordererror = 99

	Resolvewait   = 100
	Opening       = 101
	Resolving     = 102
	Resolvepause  = 103
	Resolvewarn   = 104
	Resolveerror  = 105
	Resolveok     = 106
	Resolvecancel = 107

	Submitwait  = 200
	Submitting  = 201
	Submiterror = 202
	Checkwait   = 210
	Checkaccept = 211
	Checkreject = 212

	Upwait   = 300
	Upping   = 301
	Uppause  = 302
	Uperror  = 303
	Upcancel = 304
	Upfinish = 305
	Updelete = 306

	Downwait   = 310
	Downing    = 311
	Downpause  = 312
	Downerror  = 313
	Downfinish = 314
	Downdelete = 315

	ScanDirready = 330
	ScanDirWait  = 331
	ScanDirIng   = 332
	ScanDirEnd   = 333
	ScanDirErr   = 334
)

type Download struct {
	Id         string `json:"id" gorm:"primary_key"` //
	SourceFile string `json:"sourceFile"`
	State      int    `json:"state"`
	PostRes    string `json:"postRes" gorm:"type:blob"`
	SavePath   string `json:"savePath"` //指定下载路径
	CompleteSize int64
	AutoPush   int    `json:"autoPush"` //0:自动下载 1:不自动下载 2:手动下载
}

var mu sync.Mutex

func (r Download) Valid() bool {
	return r.Id != ""
}

func (r Download) Create(db *gorm.DB) {
	mu.Lock()
	defer mu.Unlock()

	db.Create(&r)

	//推送的全部合并，手动下载接口下载的全部分开
	//if r.Push == 1 {
	//	db.New().Model(&Download{}).Where("order_number = ? AND vip = ? AND push = ?", r.OrderNumber, r.Vip, r.Push).First(&rt)
	//
	//	//下载结束->待下载
	//	if rt.History == 1 {
	//		err := db.New().Model(&Download{}).Where("id = ?", rt.Id).Update(map[string]interface{}{"history": 0, "finish_time": 0, "state": Downwait, "priority": 50}).Error
	//		if err != nil {
	//			log.Println("rt.History == 1 db Download Create", err)
	//		}
	//	}
	//}

	//if rt.Id == "" {
	//	r.Id = xutil.Gid()
	//	r.State = Downwait
	//	r.Priority = 50
	//	err := db.Create(&r).Error
	//	if err != nil {
	//		log.Println("db Download Create", err)
	//	}
	//} else {
	//	r.Id = rt.Id
	//}
}
func (r Download) Get(db *gorm.DB) (ret Download) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("id = ?", r.Id).First(&ret)
	return
}

func (r Download) ToUpload(db *gorm.DB) (ret Download) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("state = ?", Upwait).First(&ret)
	return
}

func (r Download) ToSubmit(db *gorm.DB) (ret Download) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("state = ?", Upfinish).First(&ret)
	return
}

func (r Download) ToDownload(db *gorm.DB) (rets []Download) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("state = ? and auto_push <> 1", Downwait).Find(&rets)
	return
}

func (r Download) Update(db *gorm.DB) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("id = ?", r.Id).Update(&r)
}

func (r Download) Delete(db *gorm.DB) {
	mu.Lock()
	defer mu.Unlock()

	db.Model(&Download{}).Where("id = ?", r.Id).Delete(&r)
}

func (r Download) SetError() {

}

func InitDownloadDb(name string) *gorm.DB {
	d := OpenDb(name)

	CreateTable(d, Download{})

	d.Model(&Download{}).Where("state = ?", Upping).Update("state", Upwait)
	d.Model(&Download{}).Where("state = ?", Downing).Update("state", Downwait)

	//if xini.App == xini.Ys {
	//	CreateTable(d, Download{})
	//	//清理超过5天的下载结束记录
	//	{
	//		var ids []string
	//		h, _ := time.ParseDuration("-120h")
	//		d.Model(&Download{}).Where("history = ? and finish_time < ?", 1, time.Now().Add(h).Unix()).Pluck("id", &ids)
	//		if len(ids) > 0 {
	//			d.Where("id in (?)", ids).Delete(&Download{})
	//			for _, id := range ids {
	//				d.DropTable(Upload{ParentId: id}.TableName())
	//			}
	//		}
	//	}
	//} else if xini.App == xini.Netdisk {
	//	CreateTable(d, DownloadSky{})
	//}

	return d
}
