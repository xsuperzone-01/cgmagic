package xdb

import (
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
	"log"
)

func OpenDb(name string) *gorm.DB {
	db, err := gorm.Open("sqlite3", name)
	if err != nil {
		log.Println("open", name, err)
	} else {
		log.Println("open", name, "succ")
	}
	//设置全局表名禁用复数
	db.SingularTable(true)

	return db
}

func CreateTable(db *gorm.DB, value interface{}) {
	if !db.HasTable(value) {
		db.CreateTable(value)
	} else {
		//改变表字段，只新增
		db.AutoMigrate(value)
	}
}
