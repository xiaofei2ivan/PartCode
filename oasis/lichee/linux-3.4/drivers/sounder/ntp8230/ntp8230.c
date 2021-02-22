/**************************************************************************************************************************************************
* @company  : ying zhou
* @author   : ivan.xiaofei
* @date	    : 2017.08.08
* @describe : 
*           The NTP-8230 is a single chip full digital audio amplifier including power stage for stereo amplifier system. NTP-8230 is 
*           integrated with versatile digital audio signal processing functions, highperformance, high-fidelity fully digital PWM modulator, 
*           a stereo headphone amplifier and two high-power full-bridge MOSFET power stages. The NTP-8230 receives digital serial audio 
*           data with sampling frequency from 8kHz through 96kHz. It delivers 2 x 30 watts in stereo mode without heat sink. The NTP-8230 
*           has a mixer and Bi-Quad filters which can be used to implement the essential audio signal processing functions like loudness 
*           control, compensation of a loud speaker response and  parametric equalization. All the functions of the NTP-8230 can be 
*           controlled by internal register values via I2C host interface bus.
***************************************************************************************************************************************************/

#include "ntp8230.h"
#include "../config/sounder_config.h"

static __u32 twi_id = 1;

struct i2c_client * ntp8230_client=NULL;
static struct class * ntp8230_class=NULL;
static struct ntp8230_sound_dev * ntp8230_dev=NULL;

#define I2C_DEV_NAME	"ntp8230_sound"
#define I2C_DEV_NAME_LENGTH 15

#define NTP8230_PRINTK_EN	1

extern bool getEarInjectFlag(void);

static DEFINE_MUTEX(dev_lock);

static int ntp8230_major = 0;
static int ntp8230_minor = 0;

static void ntp8230_init_events(struct work_struct *work);
struct workqueue_struct *ntp8230_wq;
static DECLARE_WORK(ntp8230_init_work,ntp8230_init_events);


static void ntp8230_resume_event(struct work_struct *work);
static DECLARE_WORK(ntp8230_resume_work,ntp8230_resume_event);

static void ntp8230_suspend_event(struct work_struct *work);
static DECLARE_WORK(ntp8230_suspend_work,ntp8230_suspend_event);

//static const unsigned short device_address_tab[2]={0x54,I2C_CLIENT_END};
static const unsigned short device_address_tab[2]={0x2A,I2C_CLIENT_END};

/*==================================================== I2C INTERFACE FUNCTION ==============================================================*/

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

	type = script_get_item("sound","ntp_pa_ctrl",&item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){		
		goto  fetch_sys_err;		
	}

	pin->ntp_pin = item.gpio;

	printk(KERN_DEBUG"SOUND : fetch sound system gpio completed . \n");

	return 0;

fetch_sys_err:

	printk(KERN_DEBUG"SOUND : fetch sound system gpio error \n");
	return ret;
}

//static s32 ntp8230_i2c_read(struct ntp8230_sound_dev*ntp8230_i2c_dev)
static s32 ntp8230_i2c_read(struct i2c_client *client, uint8_t *data, uint16_t len)
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
	msgs[1].buf   = &data[1];

	ret = i2c_transfer(client->adapter,msgs, 2);
	printk("[ntp8230] ntp8230_i2c_read Data:0x%X , Return:0x%X \n",data[0], data[1]);

	return ret;
}

//static s32 ntp8230_i2c_write(struct ntp8230_sound_dev*ntp8230_i2c_dev)
static s32 ntp8230_i2c_write(struct i2c_client *client, uint8_t *data, uint16_t len)
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


static int ntp8230_i2c_check(struct i2c_client *client)
{
	int ret=0;		
	unsigned char data_tab[2] ={0x7f} , test_tab[2]={0x1f};	

	printk("[ntp8230] ======================== i2c read/write test ================================= \n");
	//printk("[ntp8230] ,  ntp8230_i2c_write : reg = 0x1f , data = 0x68 \n");
	//ntp8230_i2c_write(client, test_tab, 2);
	
	//test_tab[0]=0x1f;
	//test_tab[1]=0xff;

	//printk("[ntp8230] ,  ntp8230_i2c_read : reg = 0x1f  \n");
	ret = ntp8230_i2c_read(client, test_tab, 1);
	printk("[ntp8230] ,  ntp8230_i2c_read : reg = 0x1f , test_tab[1]=%d \n",test_tab[1]);	
        printk("[ntp8230] ========================== i2c read/write end =============================== \n");


	printk("[ntp8230] , read chip id , reg=0x7f \n");
	ret = ntp8230_i2c_read(client, data_tab, 1);
	printk("[ntp8230] , chip id is 0x%X \n" , data_tab[1]);

	if(data_tab[1]==0x98)
	{
	   ret=1;
	   printk("[ntp8230]  ntp8230_i2c_check read && write successful !!! \n");
	}
	else if(ret<0)
	{
	    printk("[ntp8230]  ntp8230_i2c_check   ntp8230_i2c_read error !!! \n");
	}

	return ret;
}


int ntp8230_SendCmd(unsigned char *CmdList, int size, struct i2c_client *i2c_client)
{
	int index;
	int ret = 0;	
	
	for (index = 0; index < (size / 2); index++)
	{
		ret = ntp8230_i2c_write(i2c_client, CmdList, 2);
		CmdList += 2;

		if (ret < 0)
		{
			printk("### ntp8230_SendCmd, ,ret =%d,index=%d\n\n",ret ,index);
			return -1;
		}  
	}
	return 0;
}

int ntp8230_Send5Cmd(unsigned char *CmdList, int size, struct i2c_client *i2c_client)
{
	int index;
	int ret = 0;	
	
	for (index = 0; index < (size / 5); index++)
	{
		ret = ntp8230_i2c_write(i2c_client, CmdList, 5);
		CmdList += 5;

		if (ret < 0)
		{
			printk("### ntp8230_Send5Cmd, ,ret =%d,index=%d\n\n",ret ,index);
			return -1;
		}  
	}
	return 0;
}


/*=================================================== DEVICE FILE OPERATION ============================================================*/


static ssize_t ntp8230_read(struct file*filp, char __user *buf, size_t count, loff_t*f_pos){
	
	ssize_t ret = 0;
	unsigned long missing;	
	//unsigned char index=3;

	struct ntp8230_sound_dev * dev = filp->private_data;

	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"-------- ntp8230_read 1-------- \n");
	#endif

	if(down_interruptible(&(dev->sem))){
		return -ERESTARTSYS;
	}

	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"-------- ntp8230_read 2-------- \n");
	#endif

	/*while(index>0){
	    ret = ntp8230_i2c_read(dev);
	    if(ret==1) break;
	    index--;
	}*/

	if(count>=6 || count<1)  goto READ_OUT;

	if(ret>0){
	    #ifdef NTP8230_PRINTK_EN
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


static int ntp8230_write(struct file*filp, char __user *buf,size_t count, loff_t*f_pos){
	ssize_t err = 0;
	struct ntp8230_sound_dev * dev = filp->private_data;

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

static long ntp8230_ioctl(struct file*filp, unsigned int cmd, unsigned long arg){
	return 0;
}


static int ntp8230_open(struct inode*inode,struct file*filp){

	//int ret= -ENXIO;

	struct ntp8230_sound_dev * dev;
	dev = container_of(inode->i_cdev, struct ntp8230_sound_dev, dev);

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
	#ifdef NTP8230_PRINTK_EN
	printk(KERN_ALERT"ntp8230_open , file device open operation...\n");
	#endif

	return 0;
}

static int ntp8230_release(struct inode*inode,struct file*filp){

	int ret = 0;
	struct ntp8230_sound_dev * dev = filp->private_data;	
	filp->private_data = NULL;
	//kfree(dev->buffer);
	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"---- ntp8230_release ----- \n");	
	#endif
	return ret;	
}

static const struct file_operations ntp8230_fops = {
	.owner = THIS_MODULE,
	.write = ntp8230_write,
	.read  = ntp8230_read,
	.unlocked_ioctl = ntp8230_ioctl,
	.open  = ntp8230_open,
	.release = ntp8230_release
};


/*=================================================== DEVICE INIT ==========================================================*/


static void ntp8230_init(struct ntp8230_sound_dev * ntp8230)
{
	unsigned char spec_cmd[2]={0x00};
	printk("[ntp8230] ntp8230_init_events , ntp8230 chip init start ... \n");
	// reset pin to reset

	// config chip
	printk("[ntp8230] ntp8230_init_events , sizeof(ntp8230_init_part1) : %d \n",sizeof(ntp8230_init_part1));
	ntp8230_SendCmd(ntp8230_init_part1, sizeof(ntp8230_init_part1), ntp8230->client);
	
	printk("[ntp8230] ntp8230_init_events , ==== frequence divider start ==== \n");

	//=== DRC
	spec_cmd[0] = 0x7E;
	spec_cmd[1] = 0x03;
	ntp8230_i2c_write(ntp8230->client, spec_cmd, 2);	
	ntp8230_Send5Cmd(ntp8230_drc_part0, sizeof(ntp8230_drc_part0), ntp8230->client);

	spec_cmd[0] = 0x7E;
	spec_cmd[1] = 0x00;
	ntp8230_i2c_write(ntp8230->client, spec_cmd, 2);
	ntp8230_SendCmd(ntp8230_drc_part1, sizeof(ntp8230_drc_part1), ntp8230->client);

	//=== Freq divider
	spec_cmd[0] = 0x7E;
	spec_cmd[1] = 0x03;
	ntp8230_i2c_write(ntp8230->client, spec_cmd, 2);
	ntp8230_Send5Cmd(ntp8230_highfreq_div, sizeof(ntp8230_highfreq_div), ntp8230->client);

	spec_cmd[0] = 0x7E;
	spec_cmd[1] = 0x04;
	ntp8230_i2c_write(ntp8230->client, spec_cmd, 2);
	ntp8230_Send5Cmd(ntp8230_lowfreq_div, sizeof(ntp8230_lowfreq_div), ntp8230->client);

	printk("[ntp8230] ntp8230_init_events , ==== frequence divider end ==== \n");

	spec_cmd[0] = 0x7E;
	spec_cmd[1] = 0x00;
	ntp8230_i2c_write(ntp8230->client, spec_cmd, 2);

	ntp8230_SendCmd(ntp8230_init_part4, sizeof(ntp8230_init_part4), ntp8230->client);		

	// check write data and read data

	printk("[ntp8230] ntp8230_init_events , ntp8230 chip init end ... \n");
}


static void ntp8230_init_events(struct work_struct *work)
{
	ntp8230_init(ntp8230_dev);
	return;
}


static void ntp8230_resume_event(struct work_struct *work)
{
	ntp8230_init(ntp8230_dev);
	return;
}

static void ntp8230_suspend_event(struct work_struct *work)
{
	unsigned char spec_cmd[2]={0x00};

	if(!getEarInjectFlag()){
		spec_cmd[0] = 0x26;
		spec_cmd[1] = 0x07;
		ntp8230_i2c_write(ntp8230_dev->client, spec_cmd, 2);

		spec_cmd[0] = 0x28;
		spec_cmd[1] = 0x02;
		ntp8230_i2c_write(ntp8230_dev->client, spec_cmd, 2);

		spec_cmd[0] = 0x27;
		spec_cmd[1] = 0x07;
		ntp8230_i2c_write(ntp8230_dev->client, spec_cmd, 2);

	}
	return;
}

/*=================================================== I2C DEVICE DRIVER ==========================================================*/


int ntp8230_detect(struct i2c_client*client, struct i2c_board_info*info){

	struct i2c_adapter * adapter = client->adapter;

	if(!i2c_check_functionality(adapter,I2C_FUNC_SMBUS_BYTE_DATA)){
	     printk(KERN_DEBUG"------- adapter fail  -----\n");
	     return -ENODEV;
	}

	#ifndef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"------ ntp8230 i2c detect , adapter->nr : %d \n", adapter->nr);
	#endif

	if(twi_id == adapter->nr){
	     strlcpy(info->type,I2C_DEV_NAME,I2C_DEV_NAME_LENGTH);
	     return 0;
	}else{
	     return -ENODEV;
	}
}


static int ntp8230_probe(struct i2c_client * client, const struct i2c_device_id * id){

        s32 ret = -1;

	#ifdef NTP8230_PRINTK_EN
        printk(KERN_DEBUG"-------------- ntp8230 i2c probe function  --------------\n");
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		printk(KERN_DEBUG"i2c check functionality failed ... \n");
		return -ENODEV;
	}

	printk("[ntp8230] ntp8230_probe , client->adapter->nr : %d \n",client->adapter->nr);
	client->adapter->nr=twi_id;
	ret = ntp8230_i2c_check(client);
	if(ret!=1){
		printk(KERN_DEBUG"-------- I2C check failed --------- \n");
		goto I2C_CHK_FAIL;
	}
	ntp8230_dev->client=client;	
	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"-------  I2C check successful,client->addr -------- :%d \n",ntp8230_dev->client->addr);
	printk(KERN_DEBUG"-------  I2C check successful --------\n");
	#endif

	ntp8230_wq = create_singlethread_workqueue("ntp8230_init");
	if (ntp8230_wq == NULL) {
	  printk("create ntp8230_wq fail!\n");
	  return -ENOMEM;
	}
	queue_work(ntp8230_wq, &ntp8230_init_work);

	return 0;

I2C_CHK_FAIL:
	return -1;
}

static int ntp8230_i2c_remove(struct i2c_client * client){
	printk(KERN_DEBUG"----- ntp8230 i2c remove ------\n");
	//mutex_lock(&dev_lock);
	i2c_set_clientdata(client, NULL);
	cancel_work_sync(&ntp8230_init_work);
	cancel_work_sync(&ntp8230_suspend_work);
	cancel_work_sync(&ntp8230_resume_work);
	destroy_workqueue(ntp8230_wq);
	//mutex_unlock(&dev_lock);
	return 0;
}


static int ntp8230_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct ntp8230_sound_dev*ntp8230 = i2c_get_clientdata(client);

	#ifdef NTP8230_PRINTK_EN
        printk(KERN_DEBUG"NTP8230 : ntp8230_suspend . \n");
	#endif
	
	//TODO
	cancel_work_sync(&ntp8230_resume_work);
	flush_workqueue(ntp8230_wq);

	queue_work(ntp8230_wq, &ntp8230_suspend_work);
	return 0;
}


static int ntp8230_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct ntp8230_sound_dev *ntp8230 = i2c_get_clientdata(client);

	#ifdef NTP8230_PRINTK_EN
        printk(KERN_DEBUG"NTP8230 : ntp8230_resume . \n");
	#endif

	cancel_work_sync(&ntp8230_suspend_work);
	flush_workqueue(ntp8230_wq);
	//TODO

	//power on, VDD
	//reset
	//init
	queue_work(ntp8230_wq, &ntp8230_resume_work);
	return 0;		
}

static UNIVERSAL_DEV_PM_OPS(ntp8230_pm_ops,ntp8230_suspend,ntp8230_resume,NULL);
#define NTP8230_PM_OPS (&ntp8230_pm_ops)


static const struct i2c_device_id ntp8230_i2c_id[]={
	{I2C_DEV_NAME,0},
	{}
};

static struct i2c_driver ntp8230_i2c_driver = {
	.class    = I2C_CLASS_HWMON,
	.probe    = ntp8230_probe,
	.remove   = ntp8230_i2c_remove,	
	.id_table = ntp8230_i2c_id,
	.driver   ={
	    .name = I2C_DEV_NAME,
	    .owner= THIS_MODULE,
	    .pm   = NTP8230_PM_OPS,
	},
	.address_list=device_address_tab,	
};


static int __devinit ntp8230_i2c_init(void){

	int ret = -1;

	dev_t dev=0,dev_no=0;

	struct device * dev_temp=NULL;

	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"--------  ntp8230_i2c_init ------- \n");
	#endif

	if(0!=sound_fetch_sys_config(&(config_info.pin_type)))
	{
		#ifdef NPCA110_PRINTK_EN
		printk(KERN_DEBUG"NTP8230 : ntp8230 sound fetch sys_config \n");
		#endif	
		return 0;
	}

	twi_id = 0x01;
	ntp8230_i2c_driver.detect = ntp8230_detect;	


	ret = alloc_chrdev_region(&dev,0,1,I2C_DEV_NAME);
	if(ret<0){
	   #ifdef NTP8230_PRINTK_EN
	   printk(KERN_DEBUG"---------------- ntp8230 i2c device allocate fail ----------------------\n");	
	   #endif	
	   goto  FAIL;
	}	

        ntp8230_major = MAJOR(dev); 
        ntp8230_minor = MINOR(dev); 

	ntp8230_dev = kmalloc(sizeof(struct ntp8230_sound_dev),GFP_KERNEL);
	if(!ntp8230_dev){ 
		goto FAIL1;
	}

        memset(ntp8230_dev, 0, sizeof(struct ntp8230_sound_dev));

	cdev_init(&(ntp8230_dev->dev),&ntp8230_fops);
	ntp8230_dev->dev.owner = THIS_MODULE;
	ntp8230_dev->dev.ops = &ntp8230_fops;

        dev_no = MKDEV(ntp8230_major, ntp8230_minor);  
	ret = cdev_add(&(ntp8230_dev->dev), dev_no, 1);
	if(ret){ 
		goto  FAIL2;
	}

	ntp8230_class = class_create(THIS_MODULE,I2C_DEV_NAME);
	if(IS_ERR(ntp8230_class)){		
		printk(KERN_ALERT"fail to create ntp8230_class...\n");
		goto FAIL3;
	}

	dev_temp = device_create(ntp8230_class,NULL,dev,"%s",I2C_DEV_NAME);
	if(IS_ERR(dev_temp)){		
		printk(KERN_ALERT"fail to create ntp8230 device...\n");
		goto FAIL4;
	}

	/*
	err = device_create_file(dev_temp, &dev_attr_val);
	if(err<0){
		printk(KERN_ALERT"fail to create attribute val .. \n");
		goto FAIL5;
	}

	dev_set_drvdata(dev_temp, ntp8230_dev);
	*/

	sema_init(&(ntp8230_dev->sem),6);

	ret = i2c_add_driver(&ntp8230_i2c_driver);
	
	if(ret!=0){
		printk(KERN_DEBUG"---- add ntp8230 i2c device fail ------- \n");
	        goto FAIL5;
	}
	
	return ret;

FAIL5:
	device_destroy(ntp8230_class,dev);

FAIL4:
	class_destroy(ntp8230_class);

FAIL3:
	cdev_del(&(ntp8230_dev->dev));

FAIL2:
	kfree(ntp8230_dev);

FAIL1:
	unregister_chrdev_region(MKDEV(ntp8230_major,ntp8230_minor),1);

FAIL:
	return ret;
}

static void __exit ntp8230_i2c_exit(void){

	#ifdef NTP8230_PRINTK_EN
	printk(KERN_DEBUG"-------- ntp8230 i2c exit -----------\n");
	#endif	

	dev_t dev_exit_no = MKDEV(ntp8230_major, ntp8230_minor);

	if(ntp8230_class){
	    device_destroy(ntp8230_class, dev_exit_no);
	    class_destroy(ntp8230_class);
	}

	if(ntp8230_dev){
	    cdev_del(&(ntp8230_dev->dev));
	    kfree(ntp8230_dev);
	}

	unregister_chrdev_region(dev_exit_no,1);
	i2c_del_driver(&ntp8230_i2c_driver);
}


module_init(ntp8230_i2c_init);
module_exit(ntp8230_i2c_exit);

MODULE_DESCRIPTION("ivan ntp8230 i2c series driver");
MODULE_LICENSE("GPL");
