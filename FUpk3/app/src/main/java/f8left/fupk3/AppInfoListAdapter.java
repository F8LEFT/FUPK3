package f8left.fupk3;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SectionIndexer;
import android.widget.TextView;

import java.util.List;

/**
 * Created by f8left on 2015/12/23.
 */
public class AppInfoListAdapter extends BaseAdapter implements SectionIndexer {
    private String LogTag = "ListAdapter";
    private LayoutInflater inflater;

    private Activity mActivity;

    private List<PackageInfo> list;

    public AppInfoListAdapter(Activity mActivity, List<PackageInfo> list) {
        this.mActivity = mActivity;
        this.list = list;                //需要显示的所有联系人信息
    }

    public void updateListView(List<PackageInfo> list) {
        this.list = list;
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        // TODO Auto-generated method stub
        return list.size();
    }

    @Override
    public Object getItem(int position) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public long getItemId(int positon) {
        // TODO Auto-generated method stub
        return positon;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        // TODO Auto-generated method stub
        ViewHolder holder = null;
        if (convertView == null) {
            inflater = (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(R.layout.item_persion_list, null);
            holder = new ViewHolder();
            holder.ivHead = (ImageView) convertView.findViewById(R.id.head);
            holder.tvTitle = (TextView) convertView.findViewById(R.id.title);
            holder.tvLetter = (TextView) convertView.findViewById(R.id.catalog);
            holder.tvLine = (TextView) convertView.findViewById(R.id.line);
            holder.tvContent = (LinearLayout) convertView.findViewById(R.id.content);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        final PackageInfo dat = list.get(position);

        if (dat != null) {
            int section = getSectionForPosition(position);
            if (position == getPositionForSection(section)) {
                holder.tvLetter.setVisibility(View.VISIBLE);
                holder.tvLetter.setText(dat.packageName.toUpperCase().substring(0, 1));
                holder.tvLine.setVisibility(View.VISIBLE);
            } else {
                holder.tvLetter.setVisibility(View.GONE);
                holder.tvLine.setVisibility(View.GONE);
            }
            holder.tvTitle.setText(dat.packageName);
            holder.ivHead.setImageDrawable(dat.applicationInfo.loadIcon(mActivity.getPackageManager()));
        }

        return convertView;
    }

    class ViewHolder {
        ImageView ivHead;               //小图标
        TextView tvLetter;
        TextView tvTitle;               //显示的title
        TextView tvLine;                //上面的分割行
        LinearLayout tvContent;
    }

    @Override
    public int getPositionForSection(int section) {
        // TODO Auto-generated method stub
        for (int i = 0; i < getCount(); i++) {
            char firstChar = list.get(i).packageName.toUpperCase().charAt(0);
            if (firstChar == section) {
                return i;
            }
        }
        return -1;

    }

    @Override
    public int getSectionForPosition(int position) {
        // TODO Auto-generated method stub
        String py = list.get(position).packageName.toUpperCase();
        return py.charAt(0);
    }

    @Override
    public Object[] getSections() {
        // TODO Auto-generated method stub
        return null;
    }


}
