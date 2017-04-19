package com.example.dump;

import android.os.Handler;
import android.os.Message;
import android.widget.TextView;

public class DumpJni {
	
	public Handler handler = new Handler()
	{
		@Override
		public void handleMessage(Message msg)
		{
			super.handleMessage(msg);
			textView.setText(textView.getText() + msg.obj.toString());
		}
	};
	
	public TextView textView;
	
	DumpJni(TextView view)
	{
		textView = view;
	}
	
	public void SetText(String str)
	{
		Message msg = new Message();
		msg.obj = str;
		handler.sendMessage(msg);
	}
	public native void dump(String str);
	
	static {  
        System.loadLibrary("Dump");
    }  

}
