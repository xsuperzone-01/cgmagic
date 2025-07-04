客户端的主体用qt5.14.2+vs2017。主面板是qt开发，任务管理是网页。   
工程结构搬的云工作站，业务逻辑搬的效果图  

1.根目录概览  
  install 安装、卸载  
  killExe 杀目录下的进程，go开发  
  mbUtil 托盘工具，内部调用插件的功能，go开发  
  package 打包  
  softUpdate 更新程序，go开发  
  subject 主界面  

2.subject  
  changeMax 云转模  
  changeMax/changeMaxSvc 云转模的业务底层，go开发，编译了dll给qt调用，执行build.bat编译出dll  
  common 公共类  
  config 用户信息  
  db 数据库  
  io 跟插件通信  
  tool 工具包  
  transfer 传输  
  view 其他界面类  

3.package  
  目前未集成到jekins，客户端执行build.bat打包，插件执行plugin.bat打包  
  客户端的更新说明文件windows\client\dll\readme.txt  
  插件打包时，替换client\dll\mobao中的文件，修改version.txt为相应的版本号，修改windows\plugin目录的readme.txt的更新说明  

