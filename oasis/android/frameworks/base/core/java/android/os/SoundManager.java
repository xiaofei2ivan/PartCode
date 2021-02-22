/*
 *@company : yingzhou.oasis
 *@author  : xiaofei.ivan
 *@date    : 2017.8.28
 *@describe: for sounder
*/


package android.os;   

import android.os.ISoundService;  
import android.content.Context;   
import android.os.RemoteException;  
import android.util.Log;  

public class SoundManager  
{  
	//Basic Member   
	android.os.ISoundService mService;  
	private static final String TAG="SoundManager";  

	//Constructor  
	public SoundManager(Context ctx,android.os.ISoundService service)  
	{  
		mService=service;  
		Log.i(TAG,"Sound Manager Constructor method");  
	}  

	public int aux_switch(int way){  
		try{  
			Log.i(TAG,"Sound Manager Constructor mService.aux_switch \n");  
			return mService.AuxSwitch(way);  
		}
		catch(RemoteException e)  
		{  
			//return e.getMessage();
			Log.i(TAG,"Sound Manager Constructor mService.aux_switch error !!!! %s \n"+e.getMessage()); 
			return -1;  
		}  
	}  
}  
