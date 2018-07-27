package f8left.fupk3;
//===-----------------------------------------------------------*- xxxx -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// The global config file for Fupk3
//===----------------------------------------------------------------------===//

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;

import f8left.fupk3.util.RootUtil;

public class UpkConfig {
    String mHookModel = Global.XposedModule;
    String mTargetPackage = "";
    String mTargetApplication = "";
    String mTargetActivity = "";

    UpkConfig() {

    }

    void clear() {
        mHookModel = Global.XposedModule;
        mTargetPackage = "";
        mTargetApplication = "";
        mTargetActivity = "";
    }

    boolean load() {
        File file = new File(Global.hookFile);
        if (!file.exists()) {
            return false;
        }
        try {
            FileReader reader = new FileReader(file);
            BufferedReader br = new BufferedReader(reader);
            mHookModel = br.readLine();
            mTargetPackage = br.readLine();
            mTargetApplication = br.readLine();
            mTargetActivity = br.readLine();
            br.close();
            reader.close();
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return !mTargetPackage.isEmpty();
    }

    // TODO file lock needed ???
    boolean save() {
        File file = new File(Global.hookFile);
        if (!file.exists()) {
            RootUtil rootUtil = RootUtil.getInstance();
            if (rootUtil.startShell()) {
                rootUtil.execute("touch " + Global.hookFile, null);
                rootUtil.execute("chmod 777 " + Global.hookFile, null);
            }
        }

        try {
            FileWriter writer = new FileWriter(file);
            BufferedWriter wr = new BufferedWriter(writer);
            wr.write(mHookModel + "\n");
            wr.write(mTargetPackage + "\n");
            wr.write(mTargetApplication + "\n");
            wr.write(mTargetActivity + "\n");
            wr.close();
            writer.close();
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }
}
