#ifndef _SOUNDER_PIN_H
#define _SOUNDER_PIN_H



#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
//#include <linux/module.h>
//#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
//#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init-input.h>
#include <linux/pinctrl/consumer.h>
//#include <linux/cdev.h>
//#include <linux/semaphore.h>



#include <linux/mutex.h>
#include <linux/slab.h>
//#include <linux/fs.h>

#include <linux/types.h>
//#include <linux/device.h>
#include <asm/uaccess.h>

enum pin_use_type{
	GPIO_CTRL,
	GPIO_EINT,
};

struct  pin_config_info{

	enum  pin_use_type  pin_type;
	int  sound_used;
	char * name;

	u32 int_number;

	struct gpio_config  pa_pin;// for aux_pa_ctrl
	struct gpio_config  ntp_pin;// for ntp_pa_ctrl
	struct gpio_config  rs_pin;// for chip reset pin
	struct gpio_config  ps_pin;// for chip reset pin

	//struct gpio_config  irq_gpio;
};

struct pin_config_info config_info = {
	.pin_type   = GPIO_CTRL,
	.name       = NULL,
	.int_number = 0,	
};


//extern int sound_fetch_sys_config(enum pin_use_type * gpio_ctrl);



#endif
