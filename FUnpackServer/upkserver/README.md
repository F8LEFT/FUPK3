# FUPK3 server part

The server part of FUPK3. Use this to rebuild the 
unpacked dex file. 

## How to build
```
./gradlew :upkserver:fatjar
```

## How to use
```
// pull the unpacked data
adb pull /data/local/tmp/.fupk3 localFolder
// and rebuild
java -jar upkserver.jar localFolder

```

