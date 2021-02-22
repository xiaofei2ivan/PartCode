/*
 *@company : yingzhou.oasis
 *@author  : xiaofei.ivan
 *@date    : 2017.8.28
 *@describe: for sounder
*/


package com.android.server.sound;  

import android.content.Context;  
import android.os.ISoundService;  
import android.util.Log; 

public class SoundService extends ISoundService.Stub {  

	private final Context mContext;

	public SoundService(Context context) {  
	   mContext = context;	   
	   SoundNativeInit();  
	}  

	public int AuxSwitch(int way) {  
	   Log.i("Sound Service","@@@ AuxSwitch @@@");
	   return SoundAuxSwitch(way);  
	}  

	private static native boolean SoundNativeInit();  
	private static native int SoundAuxSwitch(int way); 	
};  
