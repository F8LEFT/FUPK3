package f8left.fupk3;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import f8left.fupk3.widget.SideBar;


/**
 * Created by f8left on 2015/12/23.
 */
public class ProcSelActivity extends Activity implements SideBar.OnTouchingLetterChangedListener, TextWatcher {
    ///////////////////////界面功能数据///////////////////////
    private SideBar mSideBar;

    private TextView mDialog;

    private ListView mListView;

    private EditText mSearchInput;

    private List<PackageInfo> sortDataList = new ArrayList<PackageInfo>();

    private AppInfoListAdapter mAdapter;

    private String LogTag = "SVService";

    ////////////////////////////////////////////////////

    Comparator SortComparator = new Comparator<PackageInfo>() {        //按#ABCD等顺序排序

        public int compare(PackageInfo a1, PackageInfo a2) {
            // TODO Auto-generated method stub
            return a1.packageName.toUpperCase().substring(0, 1).compareTo(a2.packageName.toUpperCase().substring(0, 1));
        }
    };
    /////////////////////////////////////////////////////

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_friendsel);

        mListView = (ListView) findViewById(R.id.secret_friend_member);
        mSideBar = (SideBar) findViewById(R.id.secret_friend_sidrbar);
        mDialog = (TextView) findViewById(R.id.secret_friend_dialog);
        mSearchInput = (EditText) findViewById(R.id.friend_search_input);

        mSideBar.setTextView(mDialog);
        mSideBar.setOnTouchingLetterChangedListener(this);


        initData();
        mListView.setOnItemClickListener(mListViewOnClick);
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
    }


    private void initData() {
        List<PackageInfo> allApp = getPackageManager().getInstalledPackages(0);
        sortDataList.addAll(allApp);
        Collections.sort(sortDataList, SortComparator);

        mAdapter = new AppInfoListAdapter(this, sortDataList);
        mListView.setAdapter(mAdapter);
        mSearchInput.addTextChangedListener(this);
    }

    @Override
    public void onTouchingLetterChanged(String s) {
        int position = 0;
        if (mAdapter != null) {
            position = mAdapter.getPositionForSection(s.charAt(0));
        }
        if (position != -1) {
            mListView.setSelection(position);
        }
    }

    @Override
    public void afterTextChanged(Editable s) {
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        filterData(s.toString().toUpperCase(), sortDataList);        //进行筛选, 大写选择

    }

    private void filterData(String filterStr, List<PackageInfo> list) {
        List<PackageInfo> filterDataList = new ArrayList<PackageInfo>();
        if (TextUtils.isEmpty(filterStr)) {
            filterDataList = list;
        } else {
            filterDataList.clear();
            for (PackageInfo sortModel : list) {
                String name = sortModel.packageName.toUpperCase();
                if (name.indexOf(filterStr) != -1) {
                    filterDataList.add(sortModel);
                }
            }
        }
        Collections.sort(filterDataList, SortComparator);
        mAdapter.updateListView(filterDataList);
    }

    private AdapterView.OnItemClickListener mListViewOnClick = new AdapterView.OnItemClickListener() {

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position,
                                long id) {
            final PackageInfo pi = sortDataList.get(position);

            ApplicationInfo appinfo = pi.applicationInfo;

            if (appinfo.packageName.equals(Global.MainPackageName)) {
                Toast.makeText(ProcSelActivity.this, "You can not select the main process", Toast.LENGTH_SHORT).show();
                return;
            }

            Intent intent = new Intent();
            intent.putExtra("packagename", appinfo.packageName);
            intent.putExtra("appclass", appinfo.className);
            // get entry activity
            Intent  resolveIntent = new Intent(Intent.ACTION_MAIN, null);
            resolveIntent.addCategory(Intent.CATEGORY_LAUNCHER);
            resolveIntent.setPackage(pi.packageName);
            List<ResolveInfo> apps = getPackageManager().queryIntentActivities(resolveIntent, 0);
            boolean noEntry = true;
            if (apps.size() != 0) {
                ResolveInfo ri = apps.iterator().next();
                if (ri != null) {
                    intent.putExtra("appentry", ri.activityInfo.name);
                    noEntry = false;
                }
            }
            if (noEntry) {
                intent.putExtra("appentry", "");
            }

            setResult(RESULT_OK, intent);
            finish();
        }
    };


}
