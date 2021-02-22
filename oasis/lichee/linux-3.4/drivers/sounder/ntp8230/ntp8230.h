#ifndef  NTP8230_DEVICE_H
#define  NTP8230_DEVICE_H

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <mach/sys_config.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>


struct ntp8230_sound_dev{
	struct i2c_client * client;
	struct cdev dev;

	struct semaphore sem;
//	spinlock_t  ivan_lock;
	struct mutex imute;

	unsigned char buffer[5]; // register byte
};

unsigned char ntp8230_init_part1[][2]=
{
	{0x02,0x00},// MCLK 12.288 MHz//设置晶振（MCLK）频率，12.288 MHz，具体的值需根据实际板上的频率修改
	{0x00,0x00},// I2S Slave Mode
	{0x52,0x00},// BTL PWM soft start off
	{0x62,0x16},// 2.1CH Mode / fault glicth on
	{0x61,0x81}, // I2S Glitch filter Setting
	{0x55,0x0A}, // MINIMUM PW for ch12 and sw
	{0x2E,0xCF},// Master Volume
	{0x2D,0x06},// Master Volume
	{0x2F,0xCF},// CH1 Volume
	{0x30,0xCF},// CH2 Volume
	{0x31,0xCF},// CH3 Volume
	{0x38,0x00},// SW Polarity Normal
	{0x15,0xFF},// CH1/CH2 Prescaler    //输出功率限制，具体值视实际需要的功率来定，此时，当PVDD=20V，喇叭=8欧时，功率输出为4W*2 1% THD
	{0x16,0xD3},// CH3 Prescaler//输出功率限制，具体值视实际需要的功率来定，此时，当PVDD=20V，喇叭=8欧时，功率输出为16W 1% THD
	{0x03,0x4E}, // Mixer 00
	{0x04,0x00}, // Mixer 01
	{0x05,0x00}, // Mixer 10
	{0x06,0x4E}, // Mixer 11
	{0x07,0x36}, // Mixer 20
	{0x08,0x36}, // Mixer 21
	{0x2C,0x43},// POE Enable
	{0x24,0x00},// DRC Delay 0
	{0x25,0x00},// 1 Band DRC, 0x11 2Band DRC
	{0x4A,0x01},// SE PWM soft start on
	{0x4B,0x01},//SE PWM soft start Control
	{0x1C,0xA8}, // LR DRC Low Side Enable wit
	{0x20,0xA8}, // SW DRC Enable with +0dB

};

//====== DRC ==========

//0x7e,0x03
unsigned char ntp8230_drc_part0[][5]=
{
  {0x46,0x0c,0x04,0x9a,0xcd},
  {0x47,0x0c,0x04,0x9a,0xcd},
  {0x48,0x20,0x00,0x00,0x00},
  {0x49,0x10,0x6f,0x6c,0xa6},
  {0x4a,0x20,0x00,0x00,0x00},
  {0x4b,0x07,0x0d,0xd1,0x50},
  {0x4c,0x08,0x0d,0xd1,0x50},
  {0x4d,0x07,0x0d,0xd1,0x50},
  {0x4e,0x11,0x77,0x2a,0xce},
  {0x4f,0x10,0xef,0x71,0x3e},
  {0x50,0x10,0x77,0xb6,0x53},
  {0x51,0x10,0xf7,0xb6,0x53},
  {0x52,0x20,0x00,0x00,0x00},
  {0x53,0x10,0x6f,0x6c,0xa6},
  {0x54,0x20,0x00,0x00,0x00},
  {0x55,0x10,0x77,0x71,0xb6},
  {0x56,0x11,0xf7,0x71,0xb6},
  {0x57,0x10,0x77,0x71,0xb6},
  {0x58,0x11,0x77,0x2a,0xce},
  {0x59,0x10,0xef,0x71,0x3e},
};

//0x7e,0x00
unsigned char ntp8230_drc_part1[][2]=
{
  {0x1c,0xa8},
  {0x1d,0x01},
  {0x1e,0xa8},
  {0x1f,0x01},
  {0x25,0x11},
  {0x22,0xa8},
  {0x23,0x41},
  {0x20,0xa8},
  {0x21,0x01},
};


//====== H/L frequence divider ======


//==================== 210hz =======================

//0x7e,0x03
unsigned char ntp8230_highfreq_div[][5]=
{	
  {0x00,0x10,0x7d,0x7f,0xd1},
  {0x01,0x11,0xfd,0x7f,0xd1},
  {0x02,0x10,0x7d,0x7f,0xd1},
  {0x03,0x11,0x7d,0x7c,0xc0},
  {0x04,0x10,0xfb,0x05,0xc1},
  {0x05,0x10,0x7d,0xa9,0xe7},
  {0x06,0x11,0xfd,0xa9,0xe7},
  {0x07,0x10,0x7d,0xa9,0xe7},
  {0x08,0x11,0x7d,0xa6,0xd5},
  {0x09,0x10,0xfb,0x59,0xef},
  {0x14,0x10,0x7d,0xa9,0xe7},
  {0x15,0x11,0xfd,0xa9,0xe7},
  {0x16,0x10,0x7d,0xa9,0xe7},
  {0x17,0x11,0x7d,0xa6,0xd5},
  {0x18,0x10,0xfb,0x59,0xef},
};

//0x7e,0x04
unsigned char ntp8230_lowfreq_div[][5]=
{	
  {0x00,0x02,0x44,0x53,0x7d},
  {0x01,0x03,0x44,0x53,0x7d},
  {0x02,0x02,0x44,0x53,0x7d},
  {0x03,0x11,0x7d,0xa6,0xd5},
  {0x04,0x10,0xfb,0x59,0xef},
  {0x05,0x02,0x44,0x32,0xeb},
  {0x06,0x03,0x44,0x32,0xeb},
  {0x07,0x02,0x44,0x32,0xeb},
  {0x08,0x11,0x7d,0x7c,0xc0},
  {0x09,0x10,0xfb,0x05,0xc1},
  {0x0a,0x02,0x44,0x53,0x7d},
  {0x0b,0x03,0x44,0x53,0x7d},
  {0x0c,0x02,0x44,0x53,0x7d},
  {0x0d,0x11,0x7d,0xa6,0xd5},
  {0x0e,0x10,0xfb,0x59,0xef},
};


//==================== 180hz =======================
/*
//0x7e,0x03
unsigned char ntp8230_highfreq_div[][5]=
{	
  {0x00,0x10,0x7d,0xda,0xe1},
  {0x01,0x11,0xfd,0xda,0xe1},
  {0x02,0x10,0x7d,0xda,0xe1},
  {0x03,0x11,0x7d,0xd8,0xa0},
  {0x04,0x10,0xfb,0xba,0x43},
  {0x05,0x10,0x7d,0xff,0x0d},
  {0x06,0x11,0xfd,0xff,0x0d},
  {0x07,0x10,0x7d,0xff,0x0d},
  {0x08,0x11,0x7d,0xfc,0xcc},
  {0x09,0x10,0xfc,0x02,0x9d},
  {0x14,0x10,0x7d,0xff,0x0d},
  {0x15,0x11,0xfd,0xff,0x0d},
  {0x16,0x10,0x7d,0xff,0x0d},
  {0x17,0x11,0x7d,0xfc,0xcc},
  {0x18,0x10,0xfc,0x02,0x9d},
};

//0x7e,0x04
unsigned char ntp8230_lowfreq_div[][5]=
{	
  {0x00,0x02,0x10,0x5d,0xbe},
  {0x01,0x03,0x10,0x5d,0xbe},
  {0x02,0x02,0x10,0x5d,0xbe},
  {0x03,0x11,0x7d,0xfc,0xcc},
  {0x04,0x10,0xfc,0x02,0x9d},
  {0x05,0x02,0x10,0x49,0x2f},
  {0x06,0x03,0x10,0x49,0x2f},
  {0x07,0x02,0x10,0x49,0x2f},
  {0x08,0x11,0x7d,0xd8,0xa0},
  {0x09,0x10,0xfb,0xba,0x43},
  {0x0a,0x02,0x10,0x5d,0xbe},
  {0x0b,0x03,0x10,0x5d,0xbe},
  {0x0c,0x02,0x10,0x5d,0xbe},
  {0x0d,0x11,0x7d,0xfc,0xcc},
  {0x0e,0x10,0xfc,0x02,0x9d},
};
*/

//======= open PWM SPK ==
unsigned char ntp8230_init_part4[][2]=
{
	{0x0c,0x3f},
	{0x17,0x1f},
	{0x18,0x1f},
	{0x19,0x15},
	{0x1a,0x15},

	{0x28,0x04}, //PWM MASK
	{0x27,0x00}, //打开PWM
	{0x26,0x00}, //关掉静音
};

#endif


