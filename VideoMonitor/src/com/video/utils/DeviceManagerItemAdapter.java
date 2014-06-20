package com.video.utils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.video.R;

public class DeviceManagerItemAdapter extends BaseAdapter {

	private Context context;
	private ArrayList<HashMap<String, String>> list;
	private File thumbnailsFile = null;

	public DeviceManagerItemAdapter(Context context, File thumbnailsFile, ArrayList<HashMap<String, String>> list) {
		this.context = context;
		this.list = list;
		this.thumbnailsFile = thumbnailsFile;
	}

	@Override
	public int getCount() {
		return list.size();
	}

	@Override
	public Object getItem(int position) {
		return list.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public View getView(final int position, View convertView, ViewGroup viewGroup) {
		ViewHolder holder;

		if (convertView == null) {
			convertView = LayoutInflater.from(context).inflate(R.layout.device_manager_item, null);
			holder = new ViewHolder();
			convertView.setTag(holder);
			holder.device_bg = (RelativeLayout) convertView.findViewById(R.id.rl_device_bg);
			holder.device_net_state = (ImageView) convertView.findViewById(R.id.iv_device_net_state);
			holder.device_name = (TextView) convertView.findViewById(R.id.tv_device_name);
			holder.device_id = (TextView) convertView.findViewById(R.id.tv_device_mac);
		} else {
			holder = (ViewHolder) convertView.getTag();
		}
		String path = list.get(position).get("deviceBg");
		if (!path.equals("null")) {
			AsyncImageTask task = new AsyncImageTask(holder.device_bg, path);
			task.execute();
		} else {
			holder.device_bg.setBackgroundResource(R.drawable.device_item_bg);
		}
		if (Utils.getOnlineState(list.get(position).get("isOnline"))) {
			holder.device_net_state.setBackgroundResource(R.drawable.icon_online);
		} else {
			holder.device_net_state.setBackgroundResource(R.drawable.icon_offline);
		}
		holder.device_name.setText(list.get(position).get("deviceName"));
		holder.device_id.setText(list.get(position).get("deviceID"));
		
		return convertView;
	}

	static class ViewHolder {
		RelativeLayout device_bg;
		ImageView device_net_state;
		TextView device_name;
		TextView device_id;
	}
	
	/**
	 * 异步加载图片类
	 */
	private final class AsyncImageTask extends AsyncTask<String, Integer, String> {
		private RelativeLayout deviceBg;
		private String imagePath;
		
		public AsyncImageTask(RelativeLayout device_bg, String path) {
			this.deviceBg = device_bg;
			this.imagePath = path;
		}

		//后台运行的子线程
		@Override
		protected String doInBackground(String... params) {
			try {
				return getCacheImageLocalPath(imagePath, thumbnailsFile);
			} catch (Exception e) {
				e.printStackTrace();
				System.out.println("MyDebug: getCacheImageLocalPath()异常！");
			}
			return null;
		}

		//更新界面显示
		@Override
		protected void onPostExecute(String path) {
			super.onPostExecute(path);
			if (deviceBg != null && path != null) {
				BitmapFactory.Options opts = new BitmapFactory.Options();
				opts.inJustDecodeBounds = true;
				BitmapFactory.decodeFile(path, opts);
				opts.inJustDecodeBounds = false;
				opts.inSampleSize = Utils.computeSampleSize(opts, -1, 128*128);
				try {
					Bitmap bm = BitmapFactory.decodeFile(path, opts);
					Drawable drawable =new BitmapDrawable(bm);
					deviceBg.setBackgroundDrawable(drawable);
				} catch (OutOfMemoryError e) {
					e.printStackTrace();
				}
			}
		}
	}
	
	/**
	 * 获得图片缓存的本地路径
	 * @param path 网络上图片的路径
	 * @param cache 本地缓存文件夹
	 * @return 返回缓存的本地路径
	 * @throws Exception 
	 */
	public String getCacheImageLocalPath(String path, File cache) throws Exception {
		String name = path.substring(path.lastIndexOf("/")+1);
		File file = new File(cache, name);
		if (file.exists()) {
			return file.getPath();
		} else {
			URL url = new URL(path);
			HttpURLConnection conn = (HttpURLConnection) url.openConnection();
			conn.setConnectTimeout(5000);
			conn.setRequestMethod("GET");
			conn.setDoInput(true);
			if (conn.getResponseCode() == 200) {
				InputStream is = conn.getInputStream();
				FileOutputStream fos = new FileOutputStream(file);
				byte[] buffer = new byte[1024*2];
				int len = 0;
				while ((len = is.read(buffer)) != -1) {
					fos.write(buffer, 0, len);
				}
				is.close();
				fos.close();
				return file.getPath();
			}
		}
		return null;
	}
}
