#ifndef _HARDWARE_SOUND_H
#define _HARDWARE_SOUND_H


#include <hardware/hardware.h>

__BEGIN_DECLS


#define SOUND_API_VERSION HARDWARE_MODULE_API_VERSION(1,0)

/**
 * The id of this module
 */
#define SOUND_HARDWARE_MODULE_ID "sound"


struct sound_module_t{
     struct hw_module_t common;
};


struct sound_device_t{

    struct hw_device_t common;

    //to npca110
    int(*AuxSwitch)(struct sound_device_t * sound, int way);// audio input way

    //to ntp8230
    

};


__END_DECLS


#endif
