package com.example.androidnetclient;

import android.support.v7.app.ActionBarActivity;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;

public class MainActivity extends ActionBarActivity {
	
	private Handler handerObj;
	private HandlerThread handlerThreadObj;
	private SendMsgRunnable sendMessageRunnableObj;
	private EditText etObj;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		

		sendMessageRunnableObj=new SendMsgRunnable();
		handlerThreadObj=new HandlerThread("hnandlerThread001");
		handlerThreadObj.start();
		
		handerObj=new Handler(handlerThreadObj.getLooper());
		
		etObj=(EditText)findViewById(R.id.inputContentId);
		
		
		
	}
	
	public void sendMessageOnClickListener(View view)
	{
		handerObj.post(sendMessageRunnableObj);
	}
	
	class SendMsgRunnable implements Runnable
	{

		@Override
		public void run() {
			
			String inputMessage=etObj.getText().toString();
			
			try {
				Socket socket=new Socket("192.168.1.203", 30000);
				
				OutputStream outputstream=socket.getOutputStream();
				
				outputstream.write(inputMessage.getBytes(),0,inputMessage.getBytes().length);
				
				
			} catch (UnknownHostException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
		}
		
	}

}
