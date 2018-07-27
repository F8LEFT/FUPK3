package f8left.fupk3;

import android.content.Intent;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.support.design.widget.TextInputLayout;
import android.util.Log;
import android.view.ViewGroup;
import android.view.ViewStructure;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import f8left.fupk3.core.FRefInvoke;
import f8left.fupk3.core.Fupk;
import f8left.fupk3.util.FileUtil;
import f8left.fupk3.util.RootUtil;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("Fupk3");
    }

    ImageView mIcon;
    TextView mPackageTv;
    TextView mApplicationTv;
    TextView mActivityTv;


    Button mUnpack;
    Button mCopy;
    Button mDelete;

    TableLayout mInfoTable;
    Button mModule;

    Button mClear;

    UpkConfig mConfig = new UpkConfig();

    // Used to load the 'native-lib' library on application startup.
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mIcon = (ImageView) findViewById(R.id.mIcon);
        mPackageTv = (TextView) findViewById(R.id.mPackage);
        mApplicationTv = (TextView) findViewById(R.id.mApplication);
        mActivityTv = (TextView) findViewById(R.id.mActivity);

        mUnpack = (Button) findViewById(R.id.mUnpack);
        mDelete = (Button) findViewById(R.id.mDelete);
        mCopy = (Button) findViewById(R.id.mCopy);
        mClear = (Button) findViewById(R.id.mClear);

        mInfoTable = (TableLayout) findViewById(R.id.mInfoTable);
        mModule = (Button) findViewById(R.id.mModule);

        mIcon.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, ProcSelActivity.class);
                startActivityForResult(intent, 0);
            }
        });

        mUnpack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mConfig.mTargetPackage.isEmpty()) {
                    Toast.makeText(MainActivity.this, "Application not valid", Toast.LENGTH_SHORT).show();
                    return;
                }
                if (mConfig.mTargetActivity.isEmpty()) {
                    Toast.makeText(MainActivity.this, "no application entry is found, please start the app yourself", Toast.LENGTH_SHORT).show();
                    return;
                }

                RootUtil rootUtil = RootUtil.getInstance();
                if (!rootUtil.startShell()) {
                    return;
                }

                rootUtil.execute("am force-stop " + mConfig.mTargetPackage, null);         //restart
                rootUtil.execute("am start -n " + mConfig.mTargetPackage + "/" + mConfig.mTargetActivity, null);
            }
        });
        mDelete.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                RootUtil rootUtil = RootUtil.getInstance();
                if (!rootUtil.startShell()) {
                    return;
                }
                String unpack = "/data/data/" + mConfig.mTargetPackage + "/.fupk3";
                rootUtil.execute("rm -rf " + unpack, null);

                mInfoTable.removeAllViews();

            }
        });
        mCopy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(!isUnpackConfigExit()) {
                    Toast.makeText(MainActivity.this, "Unpack config data not found", Toast.LENGTH_SHORT).show();
                    return;
                }

                RootUtil rootUtil = RootUtil.getInstance();
                if (!rootUtil.startShell()) {
                    return;
                }
                String unpack = "/data/data/" + mConfig.mTargetPackage + "/.fupk3";
                String unpackTmpPath = "/data/local/tmp/.fupk3";
                rootUtil.execute("rm -rf " + unpackTmpPath, null);
                rootUtil.execute("cp -rf " + unpack + " " + unpackTmpPath, null);
                rootUtil.execute("chmod -R 777 " + unpackTmpPath, null);

                // decrypt file
                int fileNum = 0, folderNum = 0;
                File file = new File(unpackTmpPath);
                if (file.exists()) {
                    LinkedList<File> list = new LinkedList<File>();
                    File[] files = file.listFiles();
                    for (File dexFile : files) {
                        if(dexFile.isDirectory()) {
                            continue;
                        }
                        if (dexFile.getName().equals("information.json")) {
                            continue;
                        }
                        try {
                            Long fileLeng = dexFile.length();
                            byte []buf = new byte[fileLeng.intValue()];

                            FileInputStream is = new FileInputStream(dexFile);
                            if (is.read(buf) > 0) {
                                for(int j = 0; j < buf.length; j++) {
                                    buf[j] ^= 0xf8;
                                }
                            }
                            is.close();

                            FileOutputStream os = new FileOutputStream(dexFile, false);
                            os.write(buf);
                            os.close();

                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
                Toast.makeText(MainActivity.this, "All file has been release into /data/local/tmp/.fupk3", Toast.LENGTH_LONG).show();
            }
        });

        mModule.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mConfig.mHookModel.equals(Global.XposedModule)) {
                    mConfig.mHookModel = Global.CydiaModule;
                } else {
                    mConfig.mHookModel = Global.XposedModule;
                }
                mModule.setText(mConfig.mHookModel);
                mConfig.save();
            }
        });

        mClear.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mConfig.clear();
                mConfig.save();
                updateUi();
            }
        });

        makeDirectoryAvaliable();
        releaseSoFile();

        mConfig.load();
        updateUi();

//        testUnpackCore();

        new Timer().scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mInfoTable.removeAllViews();
                    }
                });

                if (isUnpackConfigExit()) {
                    reparseUnpackConfigFile();
                }
            }
        }, 1000, 10000);

    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case 0: {
                if (resultCode == RESULT_OK) {
                    mConfig.mTargetPackage = data.getStringExtra("packagename");
                    mConfig.mTargetApplication = data.getStringExtra("appclass");
                    mConfig.mTargetActivity = data.getStringExtra("appentry");
                    mConfig.save();
                    updateUi();
                }
                break;
            }
            default:
                break;
        }
    }

    boolean releaseSoFile() {
        File dataPath = new File(getFilesDir().getParentFile(), "lib");
        File soPath = new File(dataPath, "libFupk3.so");
        File hookPath = new File(Global.hookSo);
        if (soPath.lastModified() <= hookPath.lastModified()) {
            return true;
        }

        if (soPath.exists() && soPath.isFile()) {
            if (FileUtil.FileCopy(soPath.getAbsolutePath(), Global.hookSo)) {
                RootUtil rootUtil = RootUtil.getInstance();
                if (rootUtil.startShell()) {
                    rootUtil.execute("chmod 777 " + Global.hookSo, null);
                    Log.d("F8LEFT", "release target so file into " + Global.hookSo);
                }
            } else {
                Log.e("F8LEFT", "release target so file failed");
            }
        }
        return true;
    }

    boolean makeDirectoryAvaliable() {
        File tmpFolder = new File("data/local/tmp");
        if (!tmpFolder.exists()) {
            tmpFolder.mkdirs();
        }
        if (!tmpFolder.canWrite() || !tmpFolder.canRead() || !tmpFolder.canExecute()) {
            RootUtil rootUtil = RootUtil.getInstance();
            if (rootUtil.startShell()) {
                rootUtil.execute("chmod 777 " + tmpFolder.getAbsolutePath(), null);
            }
        }
        return true;
    }

    boolean updateUi() {
        if (mConfig.mTargetPackage == null || mConfig.mTargetPackage.isEmpty()) {
            return false;
        }
        List<PackageInfo> allApp = getPackageManager().getInstalledPackages(0);
        for (PackageInfo app : allApp) {
            if (app.packageName.equals(mConfig.mTargetPackage)) {
                mIcon.setImageDrawable(app.applicationInfo.loadIcon(getPackageManager()));
                mPackageTv.setText(mConfig.mTargetPackage);
                mApplicationTv.setText(mConfig.mTargetApplication);
                mActivityTv.setText(mConfig.mTargetActivity);
                break;
            }
        }
        mModule.setText(mConfig.mHookModel);
        return true;
    }

    void testUnpackCore() {
        Fupk upk = new Fupk(getPackageName());
        upk.unpackAfter(2000);
    }

    boolean isUnpackConfigExit() {
        if (mConfig.mTargetPackage.isEmpty()) {
            return false;
        }

        RootUtil rootUtil = RootUtil.getInstance();
        if (!rootUtil.startShell()) {
            return false;
        }
        List<String> output = new LinkedList<String>();

        String configpath = "/data/data/" + mConfig.mTargetPackage + "/.fupk3/information.json";
        rootUtil.execute("ls " + configpath, output);
        if (output.get(0).contains("No such file or directory")) {
            return false;
        }
        return true;
    }

    void reparseUnpackConfigFile() {
        RootUtil rootUtil = RootUtil.getInstance();
        if (!rootUtil.startShell()) {
            return;
        }
        String configpath = "/data/data/" + mConfig.mTargetPackage + "/.fupk3/information.json";
        String configTmpPath = "/data/local/tmp/information.json";
        rootUtil.execute("cp -f " + configpath + " " + configTmpPath, null);
        rootUtil.execute("chmod 666 " + configTmpPath, null);

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String configTmpPath = "/data/local/tmp/information.json";

                String jsondata = FileUtil.read(configTmpPath);
                try {
                    JSONArray jsonArray = new JSONArray(jsondata);
                    for(int i = 0, count = jsonArray.length(); i < count; i++) {
                        TableRow tableRow = new TableRow(MainActivity.this);

                        JSONObject jsonObject = jsonArray.getJSONObject(i);
                        int index = jsonObject.getInt("index");
                        String name = jsonObject.getString("name");
                        String signature = jsonObject.getString("signature");
                        int status = jsonObject.getInt("status");

                        TextView tv = new TextView(MainActivity.this);
                        tv.setText(Integer.toString(index) + ": " + name);
                        switch(status) {
                            case 2:
                                tv.setTextColor(getResources().getColor(R.color.unpack_success));
                                break;
                            default:
                                tv.setTextColor(getResources().getColor(R.color.unpack_error));
                                break;
                        }
                        tableRow.addView(tv);

                        mInfoTable.addView(tableRow, new TableLayout.LayoutParams(
                                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.FILL_PARENT));
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        });
    }
}
