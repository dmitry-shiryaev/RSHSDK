#ifndef __USB_DRIVER_H__
#define __USB_DRIVER_H__

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
//our private ioctl calls
#include "USB_IOCTL_LINUX.h"
/*RSH USB devices' VID & PIDs*/
#define RSH_VID		0x534b
#define LA2USB_PID	0xc371
#define LA20USB_PID	0xc373
#define LA50USB_PID	0xc376
#define SIRIUS_PID	0xc389
#define LAN10_12USB_PID 0xc379
#define LAI24USB_PID	0xc372
#define GSPF053USB_PID	0xc35f

/*RSH devices' IN/OUT endpoints*/
#define LAUSB_CMD_EP  0x02  // EP to send command packets to device

#define GSPF053USB_CMD_EP  0x06  // write, EP6 out: to send command packets to device
#define GSPF053USB_DATA_EP 0x85  // read, EP5 in

#define LA50USB_DATA_EP 0x81
#define LAUSB_DATA_EP 0x86  // 
#define	LAUSB_STAT_EP 0x88  //
/* I actually don't know what's this for but it's better to be different for each device */
#define LAUSB_MINOR_BASE	199
/* MAX_TRANSFER is chosen so because the largest possible packet (tuning packet) is 8 bytes */
#define MAX_TRANSFER            8
/* arbitrarily chosen */
#define WRITES_IN_FLIGHT        8

#endif
