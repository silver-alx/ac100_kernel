#ifndef _NVEC_MOUSE_H
#define _NVEC_MOUSE_H
#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/tegra_devices.h>
#include <linux/earlysuspend.h>
#include <linux/freezer.h>

#include "nvos.h"
#include "nvec.h"
#include "nvodm_services.h"
#include "nvodm_mouse.h"
#include "nvec_device.h"
#include <linux/tegra_devices.h>


static const NvU8 cmdReadId		= 0xF2;
static const NvU8 cmdSetSampleRate	= 0xF3;
static const NvU8 cmdEnable		= 0xF4;
static const NvU8 cmdDisable		= 0xF5;
static const NvU8 comEtdCustom		= 0xF8;
static const NvU8 cmdReset		= 0xFF;
static const NvU8 cmdSetResolution	= 0xE8;
static const NvU8 cmdSetScaling1_1	= 0xE6;
static const NvU8 cmdSetResolution2_1	= 0xE7;
static const NvU8 cmdGetINFO		= 0xE9;
static const NvU8 cmdSetStreamMode	= 0xEA;

//
// Mouse Responses
//
static const NvU8 responseAck			= 0xFA;
static const NvU8 responseResend		= 0xFE;
static const NvU8 responseStandardMouseId	= 0x00;
static const NvU8 responseIntelliMouseId	= 0x03;
static const NvU8 responseIntelli5buttonMouseId	= 0x04;
static const NvU8 responseBatSuccess		= 0xAA;
static const NvU8 responseBatError		= 0xFC;



enum nvec_mouse_type {
	nvec_mouse_type_none,
	nvec_mouse_type_standard,
	nvec_mouse_type_intellimouse,
	nvec_mouse_type_intelli5buttonmouse,
};

#define NVEC_PAYLOAD	32

struct nvec_mouse
{
	struct input_dev	*input_dev;
	struct task_struct	*task;
	NvOdmOsSemaphoreHandle	semaphore;
	char			name[128];
	NvOdmMouseDeviceHandle	hDevice;
	enum nvec_mouse_type	type;
	unsigned char		data[NVEC_PAYLOAD];
	int			valid_data_size;
	int			enableWake;
	int			packetSize;
	unsigned char		previousState;
	int			shutdown;
	void 			*private;
	struct early_suspend early_suspend; // add in 06/20/2010
};

int nvec_mouse_cmd(struct nvec_mouse *mouse, unsigned int cmd,
	int resp_size);



#endif
