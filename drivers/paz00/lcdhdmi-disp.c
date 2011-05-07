#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dmi.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "nvec.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvodm_services.h"

#include "nvcommon.h"
#include "nvodm_pmu.h"
#include "nvodm_query_discovery.h"
#include "../../arch/arm/mach-tegra/odm_kit/adaptations/pmu/pmu_hal.h"
#include "nvodm_services.h"
#include "../../arch/arm/mach-tegra/odm_kit/adaptations/pmu/tps6586x/nvodm_pmu_tps6586x.h"
#include "../../arch/arm/mach-tegra/odm_kit/adaptations/pmu/tps6586x/nvodm_pmu_tps6586x_supply_info_table.h"


static NvOdmServicesGpioHandle s_hGpio;
static NvOdmGpioPinHandle s_hLVDSEnableGpioPin;
static NvOdmGpioPinHandle s_hBacklightEnableGpioPin;
static NvOdmGpioPinHandle s_hPanelEnableGpioPin;

static const NvOdmGpioPinInfo *s_gpioPinTable;

#define LVDSEnableGpioPin                   (0)
#define BacklightPwmGpioPin                 (1)
#define BacklightEnableGpioPin              (2)
#define PanelEnableGpioPin                  (3)


int paz00_set_lcd_output(int mode)
{
    NvU32 gpioPinCount;

    if( !s_hGpio )
    {
        s_hGpio = NvOdmGpioOpen();
        if( !s_hGpio )
        {
            return NV_FALSE;
        }
    }

    s_gpioPinTable = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_Display, 0, &gpioPinCount);
    if( gpioPinCount <= 0 )
    {
        return NV_FALSE;
    }

    s_hLVDSEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
        s_gpioPinTable[LVDSEnableGpioPin].Port,
        s_gpioPinTable[LVDSEnableGpioPin].Pin);
    NvOdmGpioConfig(s_hGpio, s_hLVDSEnableGpioPin, NvOdmGpioPinMode_Output);	//Robert 2010/03/07

    s_hBacklightEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
        s_gpioPinTable[BacklightEnableGpioPin].Port,
        s_gpioPinTable[BacklightEnableGpioPin].Pin);
    NvOdmGpioConfig(s_hGpio, s_hBacklightEnableGpioPin, NvOdmGpioPinMode_Output);	//Robert 2010/03/07

    s_hPanelEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
        s_gpioPinTable[PanelEnableGpioPin].Port,
        s_gpioPinTable[PanelEnableGpioPin].Pin);
    NvOdmGpioConfig(s_hGpio, s_hPanelEnableGpioPin, NvOdmGpioPinMode_Output);	//Robert 2010/03/07

	if( mode )
	{
	    // Enable LVDS Panel Power
	    NvOdmGpioSetState(s_hGpio, s_hPanelEnableGpioPin, mode);
	    NvOdmOsWaitUS(30000);

	    // Enable LVDS Transmitter
	    NvOdmGpioSetState(s_hGpio, s_hLVDSEnableGpioPin, mode);
	    NvOdmOsWaitUS(300000);

	    // Enable Backlight enable
	    NvOdmGpioSetState(s_hGpio, s_hBacklightEnableGpioPin, mode);
	    NvOdmOsWaitUS(10000);
	}
	else
	{
	    // Enable Backlight enable
	    NvOdmGpioSetState(s_hGpio, s_hBacklightEnableGpioPin, mode);
	    NvOdmOsWaitUS(10000);

	    // Enable LVDS Transmitter
	    NvOdmGpioSetState(s_hGpio, s_hLVDSEnableGpioPin, mode);
	    NvOdmOsWaitUS(300000);

	    // Enable LVDS Panel Power
	    NvOdmGpioSetState(s_hGpio, s_hPanelEnableGpioPin, mode);
	    NvOdmOsWaitUS(30000);
	}

	return NV_TRUE;
}


int paz00_set_hdmi_power(int ldo, int power)
{
	int waitus;
	NvOdmServicesPmuHandle hHdmiPmu;
	
    hHdmiPmu = NvOdmServicesPmuOpen();
	if( hHdmiPmu == NULL )
	{
		printk("------ %s   NULL pointer------\n", __FUNCTION__);
		return NV_FALSE;
	}

    NvOdmServicesPmuSetVoltage( hHdmiPmu, ldo, power, &waitus );

	if( waitus )
	{
		NvOdmOsWaitUS( waitus );
	}
	
	NvOdmServicesPmuClose( hHdmiPmu );

	return NV_TRUE;
}

void paz00_set_hdmi_output(int mode)
{
	if( mode == 0 )
	{
		paz00_set_hdmi_power( TPS6586xPmuSupply_LDO7, 0 );
		paz00_set_hdmi_power( TPS6586xPmuSupply_LDO8, 0 );
	}
	else
	{
		paz00_set_hdmi_power( TPS6586xPmuSupply_LDO7, 3300 );
		paz00_set_hdmi_power( TPS6586xPmuSupply_LDO8, 1800 );
	}
}

