/*
 *@company : yingzhou.oasis
 *@author  : xiaofei.ivan
 *@date    : 2017.8.28
 *@describe: for sounder
*/


#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/sound.h>

#include <stdio.h>

namespace android
{

	struct sound_device_t * sound_device = NULL;

	static jint AuxSwitch(JNIEnv*env , jobject clazz, jint way)
	{
		int auxWay = way,ret=0;
		
		ALOGI("JNI : Interface function <AuxSwitch> !!! \n");

		if (auxWay > 1){auxWay=1;}
		if(sound_device==NULL)
		{
		    //LOGI("SOUND DEVICE : device is not open , invalid !!! \n");
		    ALOGI("SOUND DEVICE : device is not open , invalid !!! \n");
		    return -1;
		}

		ret=sound_device->AuxSwitch(sound_device,auxWay);

		if(ret<0)
		{
		    ALOGI("JNI : Interface function <AuxSwitch> return error !!! \n");
		}

		return ret;
	}


	static inline int sound_device_open(const hw_module_t*module,struct sound_device_t**device)
	{
		return module->methods->open(module,SOUND_HARDWARE_MODULE_ID,(struct hw_device_t**)device);
	}

	static jboolean sound_init(JNIEnv*env,jclass clazz)
	{
		sound_module_t *module;
		//LOGI("SOUND DEVICE : JNI init . \n");
		ALOGI("SOUND DEVICE : JNI init . \n");
		if(hw_get_module(SOUND_HARDWARE_MODULE_ID,(const struct hw_module_t**)&module)==0)
		{ 
			//LOGI("SOUND DEVICE : sound stub found .\n");
			ALOGI("SOUND DEVICE : sound stub found .\n");
			if(sound_device_open(&(module->common),&sound_device) == 0)
			{
				//LOGI("SOUND DEVICE : sound device is open .");
				ALOGI("SOUND DEVICE : sound device is open .");
				return 0;
			}
			
			return -1;
		}

		return -1;
	}

	static const JNINativeMethod method_table[] =
	{
		{"SoundNativeInit","()Z",(void*)sound_init},
		{"SoundAuxSwitch","(I)I",(void*)AuxSwitch},
	};

	int register_android_server_SoundService(JNIEnv*env)
	{
		jclass clazz = env->FindClass("com/android/server/sound/SoundService");

		if (clazz == NULL) {
			ALOGI("Can't find @@@@@@@@@@ com/android/server/sound/SoundService");
			return -1;
		}

		return jniRegisterNativeMethods(env,"com/android/server/sound/SoundService",method_table,NELEM(method_table));
	}
};
