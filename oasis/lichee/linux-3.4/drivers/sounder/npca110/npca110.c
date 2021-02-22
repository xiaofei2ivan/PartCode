/**************************************************************************************************************************************************
* @company  : ying zhou
* @author   : ivan.xiaofei
* @date	    : 2017.08.08
* @describe : 
*
***************************************************************************************************************************************************/

#include "npca110.h"
#include "../config/sounder_config.h"


static __u32 twi_id = 1;

struct i2c_client * npca110_client=NULL;
static struct class * npca110_class=NULL;
static struct npca110_sound_dev * npca110_dev=NULL;



#define I2C_DEV_NAME	"npca110_sound"
#define I2C_DEV_NAME_LENGTH 15

#define NPCA110_PRINTK_EN	1

static DEFINE_MUTEX(dev_lock);

static int npca110_major = 0;
static int npca110_minor = 0;

static void npca110_init_events(struct work_struct *work);
struct workqueue_struct *npca110_wq;
static DECLARE_WORK(npca110_init_work,npca110_init_events);

static void npca110_suspend_resume(struct work_struct *work);
//struct workqueue_struct *npca110_sr_wq;
static DECLARE_WORK(npca110_sure_work,npca110_suspend_resume);

static const unsigned short device_address_tab[2]={0x73,I2C_CLIENT_END};

/*==================================================== Fetch system IO ==============================================================*/

static int sound_fetch_sys_config(enum pin_use_type * gpio_ctrl){

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

	type = script_get_item("sound","reset_pin",&item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){		
		goto  fetch_sys_err;		
	}

	pin->rs_pin = item.gpio;

	type = script_get_item("sound","ps_pin",&item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){		
		goto  fetch_sys_err;		
	}

	pin->ps_pin = item.gpio;

	printk(KERN_DEBUG"SOUND : fetch sound system gpio completed . \n");

	return 0;

fetch_sys_err:

	printk(KERN_DEBUG"SOUND : fetch sound system gpio error \n");
	return ret;
}

/*==================================================== I2C INTERFACE FUNCTION ==============================================================*/

//static s32 npca110_i2c_read(struct npca110_sound_dev*npca110_i2c_dev)
static s32 npca110_i2c_read(struct i2c_client *client, uint8_t *data, uint16_t len)
{	
	struct i2c_msg msgs[2];
	int ret = -1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = len;
	msgs[0].buf   = &data[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = &data[3];

	ret = i2c_transfer(client->adapter,msgs, 2);
	printk("npca110_i2c_read Data:0x%X 0x%X 0x%X, Return:0x%X 0x%X 0x%X\n",data[0], data[1], data[2], data[3], data[4], data[5]);

	return ret;
}

//static s32 npca110_i2c_write(struct npca110_sound_dev*npca110_i2c_dev)
static s32 npca110_i2c_write(struct i2c_client *client, uint8_t *data, uint16_t len)
{	
	struct i2c_msg msg;
	int ret   = -1;
	int retry = 3;

	msg.flags = !I2C_M_RD;
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = data;		
	do
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		retry--;
		if(ret < 0)
		{
			mdelay(30);
			printk("####retry =%d\n",retry);
		}
	} while((ret < 0) && (retry > 0));

	return ret;
}


static int npca110_i2c_check(struct i2c_client *client)
{
	int ret=0;		
	unsigned char version[6] ={0x00, 0xB8, 0x00};
	ret = npca110_i2c_read(client, version, 3);
	//printk(" npca110_i2c_check : version[3]=%d,version[4]=%d,version[5]=%d \n",version[3],version[4],version[5]);	
	if((version[3]==0x00)&&(version[4]==0x60)&&(version[5]==0x00))
	{
	   ret=1;
	   printk("[NPCA110]  npca110_i2c_check read && write successful !!! \n");
	}
	else if(ret<0)
	{
	    printk("[NPCA110]  npca110_i2c_check   npca110_i2c_read error !!! \n");
	}

	return ret;
}


int Npca110_SendCmd(unsigned char *CmdList, int size, struct i2c_client *i2c_client)
{
	int index;
	int ret = 0;
	bool pll_cmd_done = false;
	
	for (index = 0; index < (size / 3); index++)
	{
		ret = npca110_i2c_write(i2c_client, CmdList, 3);
		if (pll_cmd_done)
		{
			udelay(250);
			pll_cmd_done = false;
		}

		if((CmdList[0]==0xFF)&&(CmdList[1]==0xAB)&&((CmdList[2]==0xFB)||(CmdList[2]==0xFA)))
		{
			pll_cmd_done = true;
			printk("Npca110_SendCmd , CmdList[0] == 0xFF && CmdList[1] == 0xAB && (CmdList[2] == 0xFB || CmdList[2] == 0xFA) \n");
		}
		CmdList += 3;

		if (ret < 0)
		{
			printk("### Npca110_SendCmd, ,ret =%d,index=%d\n\n",ret ,index);
			return -1;
		}  
	}
	return 0;
}


/*=================================================== DEVICE FILE OPERATION ============================================================*/


static ssize_t npca110_read(struct file*filp, char __user *buf, size_t count, loff_t*f_pos){
	
	ssize_t ret = 0;
	unsigned long missing;	
	//unsigned char index=3;

	struct npca110_sound_dev * dev = filp->private_data;

	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"-------- npca110_read 1-------- \n");
	#endif

	if(down_interruptible(&(dev->sem))){
		return -ERESTARTSYS;
	}

	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"-------- npca110_read 2-------- \n");
	#endif

	/*while(index>0){
	    ret = npca110_i2c_read(dev);
	    if(ret==1) break;
	    index--;
	}*/

	if(count>=6 || count<1)  goto READ_OUT;

	if(ret>0){
	    #ifdef NPCA110_PRINTK_EN
	    printk(KERN_DEBUG"----------------count : %d-----  sizeof(dev->buffer) : %d\n",count,sizeof(dev->buffer));
	    printk(KERN_DEBUG"--------  : 0,1,2,3 : %d,%d,%d,%d \n",dev->buffer[0],dev->buffer[1],dev->buffer[2],dev->buffer[3]);
	    #endif
	    missing = copy_to_user(buf,&(dev->buffer),sizeof(dev->buffer));
	    ret=missing;
	}
	
READ_OUT:
	up(&(dev->sem));
	return ret;
}


static ssize_t npca110_write(struct file*filp, char __user *buf,size_t count, loff_t*f_pos){
	ssize_t err = 0;
	struct npca110_sound_dev * dev = filp->private_data;

	printk(KERN_ALERT"file device write operation...\n");

	if(down_interruptible(&(dev->sem))){
		return -ERESTARTSYS;
	}

	/*
	if(count != sizeof(dev->val)){
		goto OUT;
	}

	if(copy_from_user(&(dev->val), buf, sizeof(dev->val))){
		err = -EFAULT;
		goto OUT;
	}

	err = sizeof(dev->val);
	*/
//OUT:
	up(&(dev->sem));
	return err;
}

static long npca110_ioctl(struct file*filp, unsigned int cmd, unsigned long arg){

	ssize_t err = 0;
	struct npca110_sound_dev * npca110_local_dev = filp->private_data;

	#ifdef NPCA110_PRINTK_EN
	printk(KERN_ALERT"[NPCA110B] , user send command [%d] to device . \n",cmd);
	#endif

	if(down_interruptible(&(npca110_local_dev->sem))){
		return -ERESTARTSYS;
	}

	switch(cmd)
	{
		case AUX1_TO_AUX2:// audio input from aux1 to aux2 , connect , injected		     
		     Npca110_SendCmd(AdcWay1to2, sizeof(AdcWay1to2),npca110_local_dev->client);
		     printk("[NPCA110B] , audio input from aux1 to aux2 \n");		     
		     //__gpio_set_value(config_info.pa_pin.gpio, 1);
		     __gpio_set_value(config_info.ps_pin.gpio, 1);
		     npca110_dev->earInject = true;
		     //msleep(5);
		     break;
		case AUX2_TO_AUX1:// audio input from aux2 to aux1
		     //__gpio_set_value(config_info.pa_pin.gpio, 0);
		    // msleep(5);
		     Npca110_SendCmd(AdcWay2to1, sizeof(AdcWay2to1),npca110_local_dev->client);
		     printk("[NPCA110B] , audio input from aux2 to aux1 \n");		
		     npca110_dev->earInject = false;     
		     break;
		default:break;
	}

	up(&(npca110_local_dev->sem));
	return err;
}


bool getEarInjectFlag(void)
{
	return npca110_dev->earInject;
}

EXPORT_SYMBOL(getEarInjectFlag);

static int npca110_open(struct inode*inode,struct file*filp){

	//int ret= -ENXIO;

	struct npca110_sound_dev * dev;
	dev = container_of(inode->i_cdev, struct npca110_sound_dev, dev);

	//if(!dev->buffer){
	//   dev->buffer = kmalloc(6,GFP_KERNEL);
	 //  if(!dev->buffer){
	//     printk(KERN_DEBUG"-- alloc memory error ---  \n");
	 //     ret = -ENOMEM;  
         //  }else{
	      filp->private_data = dev;
            //  ret = 0;
	  // }	
	//}	
	#ifdef NPCA110_PRINTK_EN
	printk(KERN_ALERT"npca110_open , file device open operation...\n");
	#endif

	return 0;
}

static int npca110_release(struct inode*inode,struct file*filp){

	int ret = 0;
	//struct npca110_sound_dev * dev = filp->private_data;	
	filp->private_data = NULL;
	//kfree(dev->buffer);
	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"---- npca110_release ----- \n");	
	#endif
	return ret;	
}

static const struct file_operations npca110_fops = {
	.owner = THIS_MODULE,
	.write = npca110_write,
	.read  = npca110_read,
	.unlocked_ioctl = npca110_ioctl,
	.open  = npca110_open,
	.release = npca110_release
};


/*=================================================== DEVICE INIT ==========================================================*/

static void npca110_init_events(struct work_struct *work)
{
	printk("[NPCA110] npca110_init_events , npca110 chip init start ... \n");
	// reset pin to reset

	// config chip
	//Npca110_SendCmd(MaxxAudio_Cmd_FULL_Init, sizeof(MaxxAudio_Cmd_FULL_Init), npca110_dev->client);
	//Npca110_SendCmd(MaxxAudio_Cmd_FULL_Alg, sizeof(MaxxAudio_Cmd_FULL_Alg), npca110_dev->client);
	//Npca110_SendCmd(MaxxAudio_Cmd_FULL_Patch, sizeof(MaxxAudio_Cmd_FULL_Patch),npca110_dev->client);	

	//Npca110_SendCmd(bMaxxDSPCommands, sizeof(bMaxxDSPCommands),npca110_dev->client);
	// check write data and read data

	Npca110_SendCmd(MaxxAudio_Cmd_music1_FULL, sizeof(MaxxAudio_Cmd_music1_FULL),npca110_dev->client);
		
	//Npca110_SendCmd(MaxxAudio_Cmd_movie_FULL, sizeof(MaxxAudio_Cmd_movie_FULL),npca110_dev->client);

	printk("[NPCA110] npca110_init_events , npca110 chip init end ... \n");
	return;
}


static void npca110_suspend_resume(struct work_struct *work)
{
	__gpio_set_value(config_info.rs_pin.gpio, 0);
	msleep(5);
	__gpio_set_value(config_info.rs_pin.gpio, 1);
	msleep(5);

	if(npca110_dev->earInject){
	    Npca110_SendCmd(AdcWay1to2, sizeof(AdcWay1to2),npca110_dev->client);
	}else{
	    Npca110_SendCmd(AdcWay2to1, sizeof(AdcWay2to1),npca110_dev->client);
	}

	return;
}

/*=================================================== I2C DEVICE DRIVER ==========================================================*/


int npca110_detect(struct i2c_client*client, struct i2c_board_info*info){

	struct i2c_adapter * adapter = client->adapter;

	if(!i2c_check_functionality(adapter,I2C_FUNC_SMBUS_BYTE_DATA)){
	     printk(KERN_DEBUG"------- adapter fail  -----\n");
	     return -ENODEV;
	}

	#ifndef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"------ npca110 i2c detect , adapter->nr : %d \n", adapter->nr);
	#endif

	if(twi_id == adapter->nr){
	     strlcpy(info->type,I2C_DEV_NAME,I2C_DEV_NAME_LENGTH);
	     return 0;
	}else{
	     return -ENODEV;
	}
}


static int npca110_probe(struct i2c_client * client, const struct i2c_device_id * id){

        s32 ret = -1;

	#ifdef NPCA110_PRINTK_EN
        printk(KERN_DEBUG"-------------- npca110 i2c probe function  --------------\n");
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		printk(KERN_DEBUG"i2c check functionality failed ... \n");
		return -ENODEV;
	}

	npca110_dev->earInject = false;

	ret = npca110_i2c_check(client);
	if(ret!=1){
		printk(KERN_DEBUG"-------- I2C check failed --------- \n");
		goto I2C_CHK_FAIL;
	}
	npca110_dev->client=client;	
	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"-------  I2C check successful,client->addr -------- :%d \n",npca110_dev->client->addr);
	printk(KERN_DEBUG"-------  I2C check successful --------\n");
	#endif

	npca110_wq = create_singlethread_workqueue("npca110_init");
	if (npca110_wq == NULL) {
	  printk("create npca110_wq fail!\n");
	  return -ENOMEM;
	}
	queue_work(npca110_wq, &npca110_init_work);

	return 0;

I2C_CHK_FAIL:
	return -1;
}

static int npca110_i2c_remove(struct i2c_client * client){
	#ifdef NPCA110_PRINTK_EN
        printk(KERN_DEBUG"NPCA110 : npca110 i2c remove . \n");
	#endif
	//mutex_lock(&dev_lock);
	i2c_set_clientdata(client, NULL);
	cancel_work_sync(&npca110_init_work);
	cancel_work_sync(&npca110_sure_work);
	destroy_workqueue(npca110_wq);
	//mutex_unlock(&dev_lock);
	return 0;
}


static int npca110_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct npca110_sound_dev*npca = i2c_get_clientdata(client);

	#ifdef NPCA110_PRINTK_EN
        printk(KERN_DEBUG"NPCA110 : npca110_suspend . \n");
	#endif
	
	if(npca110_dev->earInject){
	 __gpio_set_value(config_info.ps_pin.gpio, 1);	
	__gpio_set_value(config_info.pa_pin.gpio, 1);
	}

	//TODO
	cancel_work_sync(&npca110_sure_work);
	flush_workqueue(npca110_wq);

	return 0;
}


static int npca110_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct npca110_sound_dev *npca110 = i2c_get_clientdata(client);

	#ifdef NPCA110_PRINTK_EN
        printk(KERN_DEBUG"NPCA110 : npca110_resume . \n");
	#endif

	//TODO

	//power on, VDD
	//reset
	//init
	queue_work(npca110_wq, &npca110_sure_work);

	return 0;		
}


static const struct dev_pm_ops npca110_pm_ops = {
	.suspend = npca110_suspend,
	.resume = npca110_resume,
};


static const struct i2c_device_id npca110_i2c_id[]={
	{I2C_DEV_NAME,0},
	{}
};

static struct i2c_driver npca110_i2c_driver = {
	.class    = I2C_CLASS_HWMON,
	.probe    = npca110_probe,
	.remove   = npca110_i2c_remove,	
	.id_table = npca110_i2c_id,
	.driver   ={
	    .name = I2C_DEV_NAME,
	    .owner= THIS_MODULE,
	    .pm   = &npca110_pm_ops,
	},
	.address_list=device_address_tab,	
};


static int __devinit npca110_i2c_init(void){

	int ret = -1;

	dev_t dev=0,dev_no=0;

	struct device * dev_temp=NULL;

	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"--------  npca110_i2c_init ------- \n");
	#endif

	if(0!=sound_fetch_sys_config(&(config_info.pin_type)))
	{
		#ifdef NPCA110_PRINTK_EN
		printk(KERN_DEBUG"NPCA110 : npca110 sound fetch sys_config \n");
		#endif	
		return 0;
	}

	twi_id = 0x01;
	npca110_i2c_driver.detect = npca110_detect;

	ret = alloc_chrdev_region(&dev,0,1,I2C_DEV_NAME);
	if(ret<0){
	   #ifdef NPCA110_PRINTK_EN
	   printk(KERN_DEBUG"---------------- npca110 i2c device allocate fail ----------------------\n");	
	   #endif	
	   goto  FAIL;
	}	

        npca110_major = MAJOR(dev); 
        npca110_minor = MINOR(dev); 

	npca110_dev = kmalloc(sizeof(struct npca110_sound_dev),GFP_KERNEL);
	if(!npca110_dev){ 
		goto FAIL1;
	}

        memset(npca110_dev, 0, sizeof(struct npca110_sound_dev));

	cdev_init(&(npca110_dev->dev),&npca110_fops);
	npca110_dev->dev.owner = THIS_MODULE;
	npca110_dev->dev.ops = &npca110_fops;

        dev_no = MKDEV(npca110_major, npca110_minor);  
	ret = cdev_add(&(npca110_dev->dev), dev_no, 1);
	if(ret){ 
		goto  FAIL2;
	}

	npca110_class = class_create(THIS_MODULE,I2C_DEV_NAME);
	if(IS_ERR(npca110_class)){		
		printk(KERN_ALERT"fail to create npca110_class...\n");
		goto FAIL3;
	}

	dev_temp = device_create(npca110_class,NULL,dev,"%s",I2C_DEV_NAME);
	if(IS_ERR(dev_temp)){		
		printk(KERN_ALERT"fail to create npca110 device...\n");
		goto FAIL4;
	}

	/*
	err = device_create_file(dev_temp, &dev_attr_val);
	if(err<0){
		printk(KERN_ALERT"fail to create attribute val .. \n");
		goto FAIL5;
	}

	dev_set_drvdata(dev_temp, npca110_dev);
	*/

	sema_init(&(npca110_dev->sem),6);

	ret = i2c_add_driver(&npca110_i2c_driver);
	
	if(ret!=0){
		printk(KERN_DEBUG"---- add npca110 i2c device fail ------- \n");
	        goto FAIL5;
	}
	
	return ret;

FAIL5:
	device_destroy(npca110_class,dev);

FAIL4:
	class_destroy(npca110_class);

FAIL3:
	cdev_del(&(npca110_dev->dev));

FAIL2:
	kfree(npca110_dev);

FAIL1:
	unregister_chrdev_region(MKDEV(npca110_major,npca110_minor),1);

FAIL:
	return ret;
}

static void __exit npca110_i2c_exit(void){

	#ifdef NPCA110_PRINTK_EN
	printk(KERN_DEBUG"-------- npca110 i2c exit -----------\n");
	#endif	

	dev_t dev_exit_no = MKDEV(npca110_major, npca110_minor);

	if(npca110_class){
	    device_destroy(npca110_class, dev_exit_no);
	    class_destroy(npca110_class);
	}

	if(npca110_dev){
	    cdev_del(&(npca110_dev->dev));
	    kfree(npca110_dev);
	}

	unregister_chrdev_region(dev_exit_no,1);
	i2c_del_driver(&npca110_i2c_driver);
}


module_init(npca110_i2c_init);
module_exit(npca110_i2c_exit);

MODULE_DESCRIPTION("ivan npca110 i2c series driver");
MODULE_LICENSE("GPL");




