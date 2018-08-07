Android代码是开源的，那么通过直接修改Android源码，把运行时的所有dex数据dump出来，不就可以实现一个通用的脱壳机了吗？那么我们来看一下FUPK3是怎么工作的吧。FUPK3需要修改Android源码，导出数据接口.不过，最为核心的脱壳操作是在一个so中执行的，脱壳时应用会自动加载该so，这样，就算脱壳时出了bug也可以直接修改这个so，避免重新刷机。

1. 代码注入
> 程序启动后读取配置文件/data/local/tmp/FUpk3.txt,如果命中脱壳的配置就会直接加载so /data/local/tmp/libFupk3.so，进行脱壳。修改Android源码
```
try {
    UpkConfig config = new UpkConfig();
    if (config.load() && config.mTargetPackage.equals(data.info.getPackageName())) {
        Fupk upk = new Fupk(config.mTargetPackage);
        upk.unpackAfter(10000);
    }
} catch (Throwable t) {
}
```
2. cookie

> 脱壳需要知道app当前加载的所有dex文件。每一个被加载到内存中的dex文件都会被记录为一个cookie值，存放在程序的loader中,所有类的加载都需要通过该loader进行。Java层的loader可能会比较复杂，比如会用到双亲委派机制等，不太好遍历。但是，到了Jni层，一切都简单起来了，在native中，是通过一个Hash表来存储所有dex文件的。
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
> 然后，直接在so里面调用接口,这样就可以取出所有dex文件的内存布局了。
```
auto fn = (HashTable* (*)())dlsym(libdvm, "dvmGetUserDexFiles");
if (fn == nullptr) {
    goto bail;
}
gDvmUserDexFiles = fn();
```
3. dex重建
> dex文件被加载起来后，一些值就不会再用到了。如文件头sig，数据偏移等，部分壳会把这些数据抹除。这些数据直接从应用的运行时数据 DvmDex, ClassObject, MethodObject等取出来就好,对数据进行重建。还有一些常量数据，StringPool,TypePool等，这些是不会加密的，直接dump出来。
```
bool DexDumper::fixDexHeader() {
    DexFile *pDexFile = mDvmDex->pDexFile;

    mDexHeader.stringIdsOff = (u4) ((u1 *) pDexFile->pStringIds - (u1 *) pDexFile->pHeader);
    mDexHeader.typeIdsOff = (u4) ((u1 *) pDexFile->pTypeIds - (u1 *) pDexFile->pHeader);
    mDexHeader.fieldIdsOff = (u4) ((u1 *) pDexFile->pFieldIds - (u1 *) pDexFile->pHeader);
    mDexHeader.methodIdsOff = (u4) ((u1 *) pDexFile->pMethodIds - (u1 *) pDexFile->pHeader);
    mDexHeader.protoIdsOff = (u4) ((u1 *) pDexFile->pProtoIds - (u1 *) pDexFile->pHeader);
    mDexHeader.classDefsOff = (u4) ((u1 *) pDexFile->pClassDefs - (u1 *) pDexFile->pHeader);
    return true;
}
```
> 对于Method的重构要复杂得多，每一代加固的发展重点都是增强对Method的CodeItem的加密。从整体加密到函数抽取到最新的函数VMP，还原难度也越来越高。VMP暂且不说，函数抽取本身还是可以攻破的。有些壳做得非常复杂，会在函数执行时还原代码，执行完后又加密回去。针对这种情况，可以看到，在程序运行时的某一个时刻，dex的部分数据肯定是还原的，那么我们可以通过不断地手动构造代码，来遍历触发这些还原点，同时进行数据提取，最终完成修复。
```
// 从Android源码中导出方法
/* @F8LEFT
 * This method is used to export some data for fupk3 to dump dex file.
 * Fupk3 will hook this method and get data from it.
 */
void fupkInvokeMethod(Method* meth) {
    // it is no need to init or link class, the code of the method will
    // not exec actually, so just ignore it
    // anyway, I should make sure this method has code to execute
    if (dvmIsMirandaMethod(meth) || dvmIsAbstractMethod(meth)) {
        return;
    }
    dvmInvokeMethod((Object*)0xF88FF88F, meth, NULL, NULL, NULL, true);
}

bool fupkExportMethod(Thread* self, const Method* method) {
    return false;
}

FupkInterface gFupk = {
    NULL, NULL, NULL, NULL, 
    fupkExportMethod
};
// @F8LEFT add end

// 对解析器入口插桩
 void dvmInterpret(Thread* self, const Method* method, JValue* pResult)
 {
    // @F8LEFT insert point for Fupk
    if ((u4)pResult->i == 0xF88FF88F) {
        gFupk.ExportMethod(self, method);
        return;
    }
    // @F8LEFT add end
    ...
 }
```
> 在上面，直接在Android源码中构造了脱壳用到的接口 gFupk，它提供了4个void*的空间来存放临时数据，与一个可以替换的接口方法fupkExportMethod。这样，只需要手动调用fupkInvokeMethod方法，程序就会走函数正常的执行流程，从dvmInvokeMethod开始，进入dvmInterpret中，并最终调用完gFupk.ExportMethod后直接退出Interpret。这样当我们手动调用函数时，既可以让壳还原出原始代码，又不会影响到程序本身的稳定性。
```
// 在so中直接替换ExportMethod
auto interface = FupkImpl::gUpkInterface;
if (interface == nullptr) {
    FLOGE("Unable to found fupk interface");
    return;
}
// Hook all
interface->ExportMethod = fupk_ExportMethod;
```
```
// 传递参数，然后直接调用
gUpkInterface->reserved0 = &shared;
shared.mCurMethod = dexMethod;
FupkImpl::fupkInvokeMethod(m);
shared.mCurMethod = nullptr;
```
```
// 在ExportMethod中直接提取CodeItem数据
bool fupk_ExportMethod(void *thread, Method *method) {
    DexSharedData* shared = (DexSharedData*)gUpkInterface->reserved0;
    DexMethod* dexMethod = shared->mCurMethod;
    u4 ac = (method->accessFlags) & mask;
    if (method->insns == nullptr || ac & ACC_NATIVE) {
        if (ac & ACC_ABSTRACT) {
            ac = ac & ~ACC_NATIVE;
        }
        dexMethod->accessFlags = ac;
        dexMethod->codeOff = 0;
        return false;
    }

    if (ac != dexMethod->accessFlags) {
        dexMethod->accessFlags = ac;
    }
    dexMethod->codeOff = shared->total_point;
    DexCode *code = (DexCode*)((const u1*) method->insns - 16);

    u1 *item = (u1*) code;
    int code_item_len = 0;
    if (code->triesSize) {
        const u1*handler_data = dexGetCatchHandlerData(code);
        const u1 **phandler = (const u1**) &handler_data;
        u1 *tail = codeitem_end(phandler);
        code_item_len = (int)(tail - item);
    } else {
        code_item_len = 16 + code->insnsSize * 2;
    }
    shared->extra.append((char*)item, code_item_len);
    shared->total_point += code_item_len;
    while(shared->total_point & 3) {
        shared->extra.push_back(shared->padding);
        shared->total_point++;
    }
    return true;
}
```
> 这样，所有加密的数据都提取出来了，直接进行组合，以加密的方式dump出来。
```
size_t myfwrite(const void* buffer, size_t size, size_t count, FILE* stream) {
    char *tmp = new char[size * count];
    mymemcpy(tmp, buffer, size * count);
    for (size_t i = 0; i < size * count; ++i) {
        tmp[i] ^= encryptKey;
    }
    size_t rel = fwrite(tmp, size, count, stream);
    delete []tmp;
    return rel;
}
```
4. dex修复
> 上面dump下来的dex文件是非标准的，可能存在部分的class数据不合法，并且一些软件不认。所以需要一个写一个修复的server来跳过非法的数据。server主要修改了baksmali与smali的代码，自动跳过无法反编译的类。

5. 其他
> Android加固发展到现在，经历了好几个大版本改动。最初的是dex整体加固，现在的是VMP加固，中间出了不少非常不错的脱壳机，其中最为经典的有2个:ZjDroid与dexHunter，这两个都是开源的，并且写得非常好，即使是放到今天来看，也具有相当的参考价值,想要学习脱壳的同学们可以拜读一下。另外，FUpk3是运行在dalvik上的，那么要在art下脱壳怎么办呢？道理还是一样的，只是art下会复杂很多, 跑解析的跑编译的都有，修复起来需要记录很多数据，这里由于某些原因就不公开art下的脱壳机了。

6. 写在最后
> 新手想要入门，需要什么？1. 一台谷歌亲儿子 2. 一个Ubuntu系统 3. 一套完整的Android源码。平时有事没事可以多看看源码,刷刷机之类的。Android平台与Windows上的不同，到目前为止，Android上系统的优秀的教材实在是少，如果没人手把手地带入门的话，基本上就是在白做功，幸好大部分难点的解决方案答案都可以在源码里面找到，多熟悉一下总会有好处的。



