/*
 *@company : yingzhou . oasis
 *
 *@author  : ivan . xiaofei
 *
 *@date    : 2017 . 08 .26
 *
 *@describe: for sounder
 *
 */

#include <hardware/sound.h>
#include <hardware/hardware.h>

#include <cutils/log.h>

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>


static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

const char *const NPCA110_DIR = "/sys/class/npca110_sound";
const char *const NTP8230_DIR = "/sys/class/ntp8230_sound";

int fd = 0;

#define NPCA110_DEV_NAME   "/dev/npca110_sound"


int SoundWaySwitch(struct sound_device_t * sound, int way)
{
    int ret = 0;
    unsigned int args;
    unsigned int command;

    pthread_mutex_lock(&s_lock);

    args = 0x06;
    command = way;    
    
    ALOGE("HARDWARE : @function -> SoundWaySwitch !!! \n");
    fd = TEMP_FAILURE_RETRY(open(NPCA110_DEV_NAME,O_RDONLY));
    ALOGE("HARDWARE : @open -> NPCA110_DEV_NAME, fd : %d !!! \n",fd);
    if(fd < 0){
	ALOGE("HARDWARE : @open device fail !!!");
        return -1;
    }
    
    ret = TEMP_FAILURE_RETRY(ioctl(fd, command, args));
    ALOGE("HARDWARE : @ioctl -> NPCA110_DEV_NAME, ret : %d !!! \n",ret);

    close(fd);
    pthread_mutex_unlock(&s_lock);
    return ret;
}


static int sound_close(struct sound_device_t *dev)
{
    if(fd!=0){close(fd);}
    if(dev){free(dev);}
    return 0;
}

static int sound_open(const hw_module_t* module, const char* id __unused,
                      hw_device_t** device __unused) {


    pthread_mutex_init(&s_lock, NULL);
    struct sound_device_t*sound_dev = malloc(sizeof(struct sound_device_t));

    if (!sound_dev) {
        ALOGE("Can not allocate memory for the sound device");
        return -ENOMEM;
    }

    memset(sound_dev, 0, sizeof(*sound_dev));

    sound_dev->common.tag = HARDWARE_DEVICE_TAG;
    sound_dev->common.module = (hw_module_t *) module;
    sound_dev->common.version = HARDWARE_DEVICE_API_VERSION(1,0);
    sound_dev->common.close = sound_close;

    sound_dev->AuxSwitch = SoundWaySwitch;


    *device = (hw_device_t *) sound_dev;

    return 0;
}

//=========================== sound hw module init ==========================

static struct hw_module_methods_t sound_module_methods = {
    .open = sound_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = SOUND_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = SOUND_HARDWARE_MODULE_ID,
    .name = "sound",
    .author = "ivan.xiaofei",
    .methods = &sound_module_methods,
};
