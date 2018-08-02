package android.app.fupk3;
//===-----------------------------------------------------------*- xxxx -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

import android.util.ArrayMap;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.lang.reflect.Method;

import android.app.fupk3.Global;

public class Fupk {
    private String mPackageName;
    // TODO How can I get the class loader???
    // maybe I can search from loaded Class, then get
    // loader from MainActivity
    public ClassLoader appLoader = null;

    static {
//        System.loadLibrary("Fupk3");
        System.load(Global.hookSo);
    }

    public Fupk(String packageName) {
        Log.d("F8LEFT", "Init Fupk3 for package " + packageName);
        mPackageName = packageName;
    }


    public void unpackAfter(final long millis) {
        Thread t = new Thread() {
            @Override
            public void run() {
                try {
                    Log.d("F8LEFT", "unpack after " + millis + " millisec");
                    Thread.sleep(millis);
                    unpackNow();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        };
        t.start();
    }

    public void unpackNow() {
        Log.d("F8LEFT", "unpack now");
        appLoader = getSystemLoader();
        unpackAll("/data/data/" + mPackageName);
    }

    private native void unpackAll(String folder);

    // TODO try more loader case
    public ClassLoader getSystemLoader() {
        try {
            Class ActivityThread = FRefInvoke.getClass(null, "android.app.ActivityThread");
            Object currentActivityThread = FRefInvoke.invokeMethod(
                    ActivityThread,
                    "currentActivityThread", new Class[]{},
                    null, new Object[]{});
            ArrayMap mPackages = (ArrayMap) FRefInvoke.getFieldOjbect(
                    ActivityThread, currentActivityThread, "mPackages");
            WeakReference wr = (WeakReference) mPackages.get(mPackageName);

            Class LoadedApk = FRefInvoke.getClass(null, "android.app.LoadedApk");
            ClassLoader loader = (ClassLoader) FRefInvoke.getFieldOjbect(
                    LoadedApk, wr.get(), "mClassLoader");
            return loader;
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("F8LEFT", "Unable to find system loader");
        }
        return null;
    }

    // I will load the class from java(the loader may has been replaced)
    public Class tryLoadClass(String className) {
        try {
            return FRefInvoke.getClass(appLoader, className);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
