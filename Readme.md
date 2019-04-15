# FUPK3 - The Android unpacker v3 by F8LEFT
FUPK v3, 早期的一个Android半自动脱壳机。基于Android 系统 KTU84P (4.4.4_r1)开发，可以脱没被V掉的smali函数。目前市面上的壳基本上都可以过掉，使用前记得先修改一下特征，比如导出的函数接名称，用到的配置文件名称，包名，脱壳魔数等。
## 注意
目前市面上的壳基本上已经对这个做了anti。所以不要再提issues了。
像这类脱壳机都是一旦放出去，就不会再更新的。
思路永远比方法重要，各位多动手改改，依旧能通杀所有壳。
## 安装
目前只支持Nexus5 hammerhead手机，其他机型请参考下面的编译选项
另外刷机有风险，操作需谨慎.
1. 下载release中的fupk3.apk，hammerhead.zip, upkserver-2.2.3-dev-fat.jar
2. 连接手机并进行刷机
```
adb reboot bootloader
fastboot -w update hammerhead.zip
```
3. root手机
4. 安装fupk3.apk
```
adb install fupk3.apk
```
## 编译
1. 编译系统源码
```
下载Android 源码，版本号为KTU84P
复制并替换AndroidSource目录下相应的项目到下载好的Android源码里 (dalvik/vm & frameworks/base/core/java/android/app)
编译，然后刷机（没有测试过模拟器，没手机的自行测试吧）
Root手机（Root方法自行搜索）
``` 
2. 编译并安装应用 FUpk3
```
# 在Android Studio 打开项目FUpk3, 编译并运行到上面编译好的手机里面。
# 或者命令行中执行命令 ./gradlew clean assembleRelease，然后签名安装.
```
3. 编译项目 FUnpackServer
```
# idea 打开项目FUnpackServer，然后编译
# 或者命令行中执行命令 ./gradlew :upkserver:fatjar
```
## 使用
1. 手机端打开FUpk3，点击图标选取要脱壳的应用，点击UPK脱壳.
2. 在Logcat里面会显示当前脱壳的信息, Filter 为LOG TAG： F8LEFT。
3. 信息界面中，脱壳成功的dex显示为蓝色，失败的为红色。
3. 可能存在部分dex一次没法完整脱出来，多点几次UPK。脱壳机会自动重试
4. dump出来的dex位于/data/data/pkgname/.fupk3目录下
5. 点击CPY，拷贝脱出来的dex到临时目录中 /data/local/tmp/.fupk3
6. 导出dex到电脑中 adb pull /data/local/tmp/.fupk3 localFolder
7. 使用FUnpackServer 重构dex文件 java -jar upkserver.jar localFolder

## 原理
有句话是说代码就是最好的文档，有兴趣的自行去查看代码吧。这里简单说一下.
1. 遍历 gDvm 中的dvmUserDexFiles结构，获取所有cookie(已加载的dex)
2. 对内存中的dex文件，遍历触发函数，并通过在解析器处插桩，截取解密后的code_item,获取后直接返回不执行该函数。
3. 对截取出来的数据进行重组，生成dex文件。
4. 利用修改过的smali/baksmali对dump下来的dex文件进行修复
5. Android修改过的源码额外记录为patch了，自行查看吧。
6. 更详细的内容看Other.md文件

