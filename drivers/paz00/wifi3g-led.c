#include <linux/dmi.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "nvec.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvodm_services.h"


#define	WLAN_SHUTDOWN_PIN	0
#define	WLAN_RESET_PIN		1
#define	WLAN_LED_PIN		2


static NvOdmServicesGpioHandle s_hGpio;
static const NvOdmGpioPinInfo *s_gpioPinTable;

static NvOdmGpioPinHandle s_hWlanShutdownPin;
//static NvOdmGpioPinHandle s_hWlanResetPin;
static NvOdmGpioPinHandle s_hWlanLedPin;

unsigned char gWlanLedStatus = 0;


void set_wlan_led(unsigned mode)
{
	NvU32 gpioPinCount;
//printk(KERN_INFO "set wifi led mode=%i begin==== \n",mode);
	if( gWlanLedStatus == mode )
	{
		return;
	}
	else
	{
		gWlanLedStatus = mode;
	}

	if( !s_hGpio )
	{
		s_hGpio = NvOdmGpioOpen();
		if( !s_hGpio )
		{
			return;
		}
	}

	s_gpioPinTable = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_Wlan, 0, &gpioPinCount);
	if( gpioPinCount <= 0 )
	{
		return;
	}

	if (!s_hWlanLedPin){
	s_hWlanLedPin = NvOdmGpioAcquirePinHandle(s_hGpio,
		s_gpioPinTable[WLAN_LED_PIN].Port,
		s_gpioPinTable[WLAN_LED_PIN].Pin);
		NvOdmGpioConfig(s_hGpio, s_hWlanLedPin, NvOdmGpioPinMode_Output);
	}

	if (!s_hWlanShutdownPin){
	s_hWlanShutdownPin = NvOdmGpioAcquirePinHandle(s_hGpio,
		s_gpioPinTable[WLAN_SHUTDOWN_PIN].Port,
		s_gpioPinTable[WLAN_SHUTDOWN_PIN].Pin);
		NvOdmGpioConfig(s_hGpio, s_hWlanShutdownPin, NvOdmGpioPinMode_Output);
	}
	// enable or disable WLAN led

	NvOdmGpioSetState(s_hGpio, s_hWlanLedPin, mode);

	NvOdmGpioSetState(s_hGpio, s_hWlanShutdownPin, mode);
	NvOdmOsWaitUS(300);
//printk(KERN_INFO "set wifi led end===== \n");

    return;
}

