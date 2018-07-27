package f8left.fupk3;
//===-----------------------------------------------------------*- xxxx -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// The entry for Xposed
//===----------------------------------------------------------------------===//

import java.io.File;
import java.io.FileReader;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.callbacks.XC_LoadPackage;
import f8left.fupk3.core.Fupk;
import f8left.fupk3.util.RootUtil;

public class XpEntry implements IXposedHookLoadPackage{
    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam param)
            throws Throwable {
        if (!param.isFirstApplication) {
            return;
        }

        UpkConfig config = new UpkConfig();
        if (!config.load()) {
            return;
        }
        if (!config.mHookModel.equals(Global.XposedModule) ||
                !config.mTargetPackage.equals(param.packageName))  {
            return;
        }
        Fupk upk = new Fupk(config.mTargetPackage);
        upk.unpackAfter(10000);
        return;
    }
}
