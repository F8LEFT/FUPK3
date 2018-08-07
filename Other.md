Android代码是开源的，那么通过直接修改Android源码，把运行时的所有dex数据dump出来，不就可以实现一个通用的脱壳机了吗？那么我们来关注一下需要导出那些数据吧。

1. cookie

> 每一个被加载到内存中的dex文件都会被记录为一个cookie值，存放在程序的loader中,所有类的加载都需要通过该loader取值。Java层的loader可能会比较复杂，比如会用到双亲委派机制等，不太好遍历。但是，到了Jni层，一切都简单起来了，在native中，是通过一个Hash表来存储所有dex文件的。
```
// 每个打开的dex文件都会存储到gDvm.userDexFiles中
static void addToDexFileTable(DexOrJar* pDexOrJar) {
    u4 hash = (u4) pDexOrJar;
    void* result;

    dvmHashTableLock(gDvm.userDexFiles);
    result = dvmHashTableLookup(gDvm.userDexFiles, hash, pDexOrJar,
            hashcmpDexOrJar, true);
    dvmHashTableUnlock(gDvm.userDexFiles);

    if (result != pDexOrJar) {
        ALOGE("Pointer has already been added?");
        dvmAbort();
    }

    pDexOrJar->okayToFree = true;
}
```
> 因此，想要知道当前apk加载了那些dex就简单多了，只需要遍历这个userDexFile这个Hashtable就好了。直接在Android源码中导出接口。
```
// @F8LEFT exported function
HashTable* dvmGetUserDexFiles() {
    return gDvm.userDexFiles;
}
```

