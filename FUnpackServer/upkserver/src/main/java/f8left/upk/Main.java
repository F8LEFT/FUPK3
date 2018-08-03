package f8left.upk;//===-----------------------------------------------------------*- xxxx -*-===//

import java.io.File;

//
//                     Created by F8LEFT on 2018/4/12.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
public class Main {

    static void _useage() {
        System.out.println("useage: FUnpackDecoder");
        System.out.println("    path/to/.fupk(contain information.json)");
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            _useage();
            return;
        }
        File file = new File(args[0]);
        if (!file.exists() || !file.isDirectory()) {
            _useage();
            return;
        }
        File fixPath = new File(file, "fix");
        File srcPath = new File(file, "src");
        if (!fixPath.exists()) {
            fixPath.mkdir();
        }
        if (!srcPath.exists()) {
            srcPath.mkdir();
        }

        for(File f : file.listFiles()) {
            try {
                int ftype = DFileUtil.isDexFile(f);
                if (ftype == 0) continue;

                File src = new File(srcPath, f.getName());
                File fix = new File(fixPath, f.getName() + ".dex");
                // d if 0 x if 1
                String deccmd[] = new String[4];
//                if (ftype == 1) deccmd[0] = "d";
//                else deccmd[0] = "x";
                deccmd[0] = "d";
                deccmd[1] = f.getAbsolutePath();
                deccmd[2] = "-o";
                deccmd[3] = src.getAbsolutePath();
                org.jf.baksmali.Main.main(deccmd);

                String becmd[] = new String[4];
                becmd[0] = "a";
                becmd[1] = src.getAbsolutePath();
                becmd[2] = "-o";
                becmd[3] = fix.getAbsolutePath();
                org.jf.smali.Main.main(becmd);

            } catch (Exception e) {
                e.printStackTrace();
            }

        }
    }
}
