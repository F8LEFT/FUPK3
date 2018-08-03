package f8left.upk;//===-----------------------------------------------------------*- xxxx -*-===//

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;

//
//                     Created by F8LEFT on 2018/4/12.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
public class DFileUtil {
    // 1 dex file
    // 2 dey flie
    // other no
    static int isDexFile(File f) {
        if (!f.exists()) return 0;
        if (f.isDirectory()) return 0;
        int type = 0;
        try {
            FileReader reader =  new FileReader(f);
            char sig[] = new char[3];
            if (reader.read(sig) == 3) {
                if (sig[0] == 0x64 && sig[1] == 0x65) {
                    if (sig[2] == 0x78) {
                        type = 1;
                    } else {
                        type = 2;
                    }
                }
            }
            reader.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return type;
    }

}
