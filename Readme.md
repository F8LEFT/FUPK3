# FUPK3 - The Android unpacker v3 by F8LEFT
FUPK v3, 早期的一个Android半自动脱壳机。基于Android 系统 KTU84P (4.4.4_r1)开发，可以脱没被V掉的smali函数。目前市面上的壳基本上都可以过掉，使用前记得先修改一下特征，比如导出的函数接口名称，用到的配置文件名称等。

## 编译
1. 编译系统源码
```
下载Android 源码，版本号为KTU84P
复制并替换AndroidSource目录下相应的项目到下载好的Android源码里
编译，然后刷机（没有测试过模拟器，没手机的自行修改吧）
Root手机（Root方法自行搜索）
``` 
2. 编译并安装应用 FUpk3
```
# Android Studio 打开项目FUpk3, 编译并运行到上面编译好的手机里面。
# 或者命令行中执行命令 ./gradlew clean assembleRelease，然后安装
```
3. 编译项目 FUnpackServer
```
# idea 打开项目FUnpackServer，然后编译
# 或者命令行中执行命令 ./gradlew :upkserver:fatjar
```
## 使用
1. 手机端打开FUpk3，选取要脱壳的应用，点击Unpack。
2. 在Logcat里面会显示当前脱壳的信息。
3. 可能存在部分dex一次没法完整脱出来，多点几次Unpack。
4. 点击Copy，拷贝脱出来的dex到临时目录中 /data/local/tmp/.fupk3
5. 导出dex到电脑中 adb pull/data/local/tmp/.fupk3 localFolder
6. 使用FUnpackServer 重构dex文件 java -jar upkserver.jar localFolder

## 原理
有句话是说代码就是最好的文档，有兴趣的自行去查看代码吧。这里简单说一下.
1. 遍历 gDvm 中的dvmUserDexFiles结构，获取所有cookie(已加载的dex)
2. 对内存中的dex文件，遍历触发相关的函数，并通过在解析器处插桩，截取解密后的code_item
3. 对截取出来的数据进行重组，生成dex文件。
4. 利用修改过的smali/baksmali对dump下来的dex文件进行修复
5. Android修改过的源码额外记录为patch了，自行查看吧。

