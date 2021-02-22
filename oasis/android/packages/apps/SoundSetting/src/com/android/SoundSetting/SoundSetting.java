package com.android.SoundSetting;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.os.SoundManager;
import android.view.View;
import android.widget.Button;
import android.util.Log;
import com.android.SoundSetting.R;  
import android.app.Activity;  
//import android.os.ServiceManager;
import android.os.ISoundService;  
import android.os.RemoteException;
import android.view.View.OnClickListener;  


public class SoundSetting extends Activity {

    private SoundManager iSoundManager;
    private Button AUX1=null;
    private Button AUX2=null;

    private int ret=6;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sound_setting);

        iSoundManager = (SoundManager)getSystemService("sound");
        AUX1 = (Button)findViewById(R.id.button_AUX1);
        AUX2 = (Button)findViewById(R.id.button_AUX2);

        AUX1.setOnClickListener(new OnClickListener(){
	    @Override
	    public void onClick(View v){

		if(v.equals(AUX1)){
		    ret=iSoundManager.aux_switch(1);
		    Log.v("ivan","Button AUX1, ret : %d "+ret+"\n");		   
		}
	    }
	});

        AUX2.setOnClickListener(new OnClickListener(){
	    @Override
	    public void onClick(View v){
		if(v.equals(AUX2)){
		    ret=iSoundManager.aux_switch(0);
		    Log.v("ivan","Button AUX2, ret : %d "+ret+"\n");		   
		}
	    }
	});
    }


}
