package xdb

import (
	"github.com/jinzhu/gorm"
	"log"
	"sync"
)

type Download struct {
	ProductId    string `json:"productId" gorm:"primary_key"`
	Profiles     string `json:"profiles"`
	Brance       string `json:"brance"`
	Url          string `json:"url"`
	CallbackUrl  string `json:"callbackUrl"`
	ForceVersion string `json:"forceVersion" gorm:"type:blob"`
	Version      string `json:"version" gorm:"type:blob"`
	Status       string `json:"status"`
}

var mutex sync.Mutex

func (r Download) Create(ProductId string, db *gorm.DB) {
	r.ProductId = ProductId
	ret := Download{ProductId: ProductId}.Get(db)

	mutex.Lock()
	defer mutex.Unlock()
	if ret.ProductId == "" {
		err := db.Create(&r).Error
		if err != nil {
			log.Println("db Download Create", err)
		}
	}

}

func (r Download) Update(db *gorm.DB) {
	mutex.Lock()
	defer mutex.Unlock()

	err := db.Model(&Download{}).Where("product_id = ?", r.ProductId).Update(&r).Error
	if err != nil {
		log.Println("更新错误", err, r.ProductId)
	}

}

func (r Download) Get(db *gorm.DB) (ret Download) {
	mutex.Lock()
	defer mutex.Unlock()

	err := db.Model(&Download{}).Where("product_id = ?", r.ProductId).First(&ret).Error
	if err != nil {
		log.Println("获取（Get）错误", err, r.ProductId)
	}

	return
}

func InitDownloadDb(name string) *gorm.DB {
	d := OpenDb(name)

	//如果是新库，forceVersion改成blob。原先的varchar(255)好像也能容下超过255的字符。
	CreateTable(d, Download{})

	return d
}
