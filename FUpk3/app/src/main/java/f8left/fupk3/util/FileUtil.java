package f8left.fupk3.util;
//===-----------------------------------------------------------*- xxxx -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// export some help func
//===----------------------------------------------------------------------===//

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;

public class FileUtil {

    public static boolean isSameFile(BufferedInputStream f1, BufferedInputStream f2) {
        boolean isSame = false;
        try {
            int i1 = f1.available();
            int i2 = f2.available();
            if (i1 != i2) {
                return isSame;
            }
            byte[] b1 = new byte[i1];
            byte[] b2 = new byte[i2];

            f1.read(b1);
            f2.read(b2);

            return Arrays.equals(b1, b2);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return isSame;
    }

    public static boolean ReleaseAssests(Context ctx, String assetName, String targetPath, String targetName) {
        String file = targetPath + "/" + targetName;
        File targetFile = new File(targetPath);
        if(!targetFile.exists()) {
            targetFile.mkdirs();
        }
        try {
            targetFile = new File(file);
            if (targetFile.exists()) {
                InputStream is = ctx.getResources().getAssets().open(assetName);
                FileInputStream fs = new FileInputStream(targetFile);
                BufferedInputStream bi = new BufferedInputStream(is);
                BufferedInputStream fi = new BufferedInputStream(((InputStream)fs));
                if(isSameFile(bi, fi)) {
                    is.close();
                    ((InputStream)fs).close();
                    bi.close();
                    fi.close();
                    return true;
                }
            }

            targetFile.delete();
            InputStream is = ctx.getResources().getAssets().open(assetName);
            FileOutputStream fs = new FileOutputStream(file);
            byte[] buf = new byte[0x1C00];
            while(true) {
                int iReadLen = is.read(buf);
                if(iReadLen <= 0) {
                    break;
                }

                fs.write(buf, 0, iReadLen);
            }

            fs.close();
            is.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static boolean FileCopy(String src, String dst) {
        try {
            FileOutputStream outStream = new FileOutputStream(dst);
            FileInputStream inStream = new FileInputStream(src);

            byte[] buf = new byte[0x1000];

            while(true) {
                int iReadLen = inStream.read(buf);
                if(iReadLen <= 0) {
                    break;
                }

                outStream.write(buf, 0, iReadLen);
            }

            outStream.close();
            inStream.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static String read(String path) {
        try {
            BufferedReader reader = new BufferedReader(new FileReader(path));
            String lineText;
            String rel = "";

            while((lineText = reader.readLine()) != null) {
                rel += lineText;
            }
            reader.close();

            return rel;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }
}

