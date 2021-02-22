/**************************************************************************************************************************
/* @company  : ying zhou
/* @author   : ivan.xiaofei
/* @date     : 2017.08.08
/* @describe :
/*	       sounder system sys-config control sys
/**************************************************************************************************************************/

#include "sounder_config.h"



int sound_fetch_sys_config(enum pin_use_type * gpio_ctrl){

	int   ret = -1;
	script_item_u	item;
	script_item_value_type_e type;
	struct  pin_config_info * pin = container_of(gpio_ctrl,struct pin_config_info, pin_type);

	type = script_get_item("sound","sound_used",&item);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){		
		goto  fetch_sys_err;
	}

	pin->sound_used  = item.val;
	if(1 != pin->sound_used){goto fetch_sys_err;}

	type = script_get_item("sound","aux_pa_ctrl",&item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){		
		goto  fetch_sys_err;		
	}

	pin->pa_pin = item.gpio;

	printk(KERN_DEBUG"SOUND : fetch sound system gpio completed . \n");

	return 0;

fetch_sys_err:

	printk(KERN_DEBUG"SOUND : fetch sound system gpio error \n");
	return ret;
}

//EXPORT_SYMBOL(sound_fetch_sys_config);

