# 这是一个esp32-s2的智能穿戴手表

![watch](watch.png)

* 本项目硬件
1. esp32s2 4M flash 2M Psram
2. 128MB flash 存储
3. gps
4. 三轴加速计
5. 地磁传感器
6. qi无线充电
7. 振动马达
8. 1.69寸 240*280 触摸屏
9. NFC
10. 350mah电池

* 功耗
1. 待机0.2ma
2. 开机21ma
3. [2]+wifi 38ma
4. [2+3]+联网升级85ma
5. 开gps增加30ma

* 项目目录结构
```
components      代码组件
    cJSON       json 库
    lpbsp       esp32-s2 驱动
    lplib       一些通用代码
    lvgl_839    ui库
    mbedtls     tls库
    miniz_300   zip处理库
    minmea_030  gps处理库
    sdl2        vs pc库

Debug           vs编译输出目录
main            功能代码
tool            本地服务器软件
LpTestCa.key    测试密钥
lptool.py       打包固件工具
LpWatch.sln     vs模拟工程
``` 

* 项目使用
1. 先搭建esp32开发环境，本项目使用[esp-idf-v5.1.1]
2. 使用命令idf.py build进行编译
3. 使用命令python lptool.py -o app.zip --PackAppTest=build/lwatch.bin对固件打包，或vscode用户直接快捷键任务[PackAppTest]
4. [操作3]会在tool目录下出现[app.zip]固件
5. 操作电脑生成[移动热点],点击[tool/dufs.exe]启动服务器(热点ip应是192.168.137.1)
6. 手表操作连接热点wifi,再点击连接升级(connect up)，下载完程序，点击[app up]进行升级(点击back up恢复出厂固件)
7. 电脑模拟运行:双击打开[LpWatch.sln]设置为[x86],点击运行。

* 项目其他说明
1. lpverify是固件校验处理功能模块，sdl2是模拟esp32s2的电脑环境，它们不开源。
2. lvgl_839,mbedtls以库形式提供以节约编译时间，它们的源码请到它们的官网上下载。

* 注意
1. 在不熟悉本项目前，一定一定不要去除现有的升级功能以及版本恢复功能，否则系统将无法再升级，只能拆机重刷固件。
2. 注意esp32s2 线程init_page_task-目前配置1024*16-现测试使用最大深度为13KB左右(2023-10).
3. vs测试环境不等同esp32s2真实环境，升级固件前谨慎，有严重bug无法升级了，只能拆机重刷固件!!!