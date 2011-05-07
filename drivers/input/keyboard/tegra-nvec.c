/*
 * drivers/input/keyboard/tegra-nvec.c
 *
 * Keyboard class input driver for keyboards connected to an NvEc compliant
 * embedded controller
 *
 * Copyright (c) 2009, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/tegra_devices.h>
#include <linux/freezer.h>

#include "nvos.h"
#include "nvec.h"
#include "nvodm_services.h"
#include "nvodm_keyboard.h"
#include "nvec_device.h"
#include <linux/earlysuspend.h>

#define DRIVER_DESC "NvEc keyboard driver"
#define DRIVER_LICENSE "GPL"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define NVEC_PAYLOAD    32
#define KEYBOARD_SCANNING_DISABLED_IN_SUSPEND 0

/* The total number of scan codes will be (first - last) */
#define EC_FIRST_CODE 0x00
//#define EC_LAST_CODE 0x58
#define EC_LAST_CODE 0x7d
#define EC_TOTAL_CODES (EC_LAST_CODE - EC_FIRST_CODE + 1)

int power_button_event = 0;
int lid_switch_event = 0;
int lcd_status = 1;
static NvBool   b_flag_keyboard_suspend = NV_FALSE;

/**
 * @brief This is the actual Scan-code-to-VKey mapping table. For new layouts
 *        this is the only structure which needs to be modified to return the
 *        proper vkey depending on the scan code.
 */
NvU32 code_tab_102us[EC_TOTAL_CODES] = {
	KEY_GRAVE,	// 0x00
	KEY_BACK,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_Q,		// 0x10
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_T,
	KEY_Y,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_LEFTBRACE,
	KEY_RIGHTBRACE,
	KEY_ENTER,
	KEY_LEFTCTRL,
	KEY_A,
	KEY_S,
	KEY_D,		// 0x20
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_GRAVE,
	KEY_LEFTSHIFT,
	KEY_BACKSLASH,
	KEY_Z,
	KEY_X,
	KEY_C,
	KEY_V,
	KEY_B,		// 0x30
	KEY_N,
	KEY_M,
	KEY_COMMA,
	KEY_DOT,
	KEY_SLASH,
	KEY_RIGHTSHIFT,
	KEY_KPASTERISK,
	KEY_LEFTALT,
	KEY_SPACE,
	KEY_CAPSLOCK,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,		// 0x40
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_FN,
	0,		//VK_SCROLL
	KEY_KP7,
	KEY_KP8,
	KEY_KP9,
	KEY_KPMINUS,
	KEY_KP4,
	KEY_KP5,
	KEY_KP6,
	KEY_KPPLUS,
	KEY_KP1,
	KEY_KP2,	// 0x50
	KEY_KP3,
	KEY_KP0,
	KEY_KPDOT,
	KEY_MENU,		//VK_SNAPSHOT
	KEY_POWER,
	KEY_102ND,		//VK_OEM_102   henry+ 0x2B (43) BACKSLASH have been used,change to use 0X56 (86)
	KEY_F11,		//VK_F11
	KEY_F12,		//VK_F12
	0, 
	0, 
	0, 
	0, 
	0, 
	0, 
	0, 
	0, // 60 
	0,
	0,
	KEY_SEARCH, // add search key map 
	0,		
	0,
	0,
	0,	
	0,		
	0, 
	0, 
	0, 
	0, 
	0, 
	0, 
	0, 
	0, // 70 
	0,
	0,
	KEY_KP5,  //73 for JP keyboard '\' key, report 0x4c
	0,		
	0,
	0,
	0,	
	0,		
	0, 
	0, 
    0, 
	0, 
	KEY_KP9, //7d  for JP keyboard '|' key, report 0x49
};

/* The total number of scan codes will be (first - last) */
#define EC_EXT_CODE_FIRST 0xE010
#define EC_EXT_CODE_LAST 0xE06D
#define EC_EXT_TOTAL_CODES (EC_EXT_CODE_LAST - EC_EXT_CODE_FIRST + 1)

/**
 * @brief This table consists of the scan codes which were added after the
 *		original scan codes were designed.
 *        To avoid moving the already designed buttons to accomodate these
 *        new buttons, the new scan codes are preceded by 'E0'.
 */
NvU32 extcode_tab_us102[EC_EXT_TOTAL_CODES] = {
	0,		// 0xE0 0x10
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,		//VK_MEDIA_NEXT_TRACK,
	0,
	0,
	0,		//VK_RETURN,
	KEY_RIGHTCTRL,		//VK_RCONTROL,
	0,
	0,
	KEY_MUTE,	// 0xE0 0x20
	0,		//VK_LAUNCH_APP1
	0,		//VK_MEDIA_PLAY_PAUSE
	0,
	0,		//VK_MEDIA_STOP
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	KEY_VOLUMEUP,	// 0xE0 0x30
	0,
	0,		//VK_BROWSER_HOME
	0,
	0,
	KEY_KPSLASH,	//VK_DIVIDE
	0,
	KEY_SYSRQ,		//VK_SNAPSHOT
	KEY_RIGHTALT,		//VK_RMENU
	0,		//VK_OEM_NV_BACKLIGHT_UP
	0,		//VK_OEM_NV_BACKLIGHT_DN
	0,		//VK_OEM_NV_BACKLIGHT_AUTOTOGGLE
	0,		//VK_OEM_NV_POWER_INFO
	0,		//VK_OEM_NV_WIFI_TOGGLE
	0,		//VK_OEM_NV_DISPLAY_SELECT
	0,		//VK_OEM_NV_AIRPLANE_TOGGLE
	0,		//0xE0 0x40
	KEY_LEFT,		//VK_OEM_NV_RESERVED    henry+ for JP keyboard
	0,		//VK_OEM_NV_RESERVED
	0,		//VK_OEM_NV_RESERVED
	0,		//VK_OEM_NV_RESERVED
	0,		//VK_OEM_NV_RESERVED
	KEY_CANCEL,
	KEY_HOME,
	KEY_UP,
	KEY_PAGEUP,		//VK_PRIOR
	0,
	KEY_LEFT,
	0,
	KEY_RIGHT,
	0,
	KEY_END,
	KEY_DOWN,	// 0xE0 0x50
	KEY_PAGEDOWN,		//VK_NEXT
	KEY_INSERT,
	KEY_DELETE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	KEY_LEFTMETA,	//VK_LWIN
	0,		//VK_RWIN
	KEY_BACK,	//VK_APPS
	KEY_KPMINUS, //for power button workaround
	0, 
	0,
	0,
	0,
	0,
	0,
	0,		//VK_BROWSER_SEARCH
	0,		//VK_BROWSER_FAVORITES
	0,		//VK_BROWSER_REFRESH
	0,		//VK_BROWSER_STOP
	0,		//VK_BROWSER_FORWARD
	0,		//VK_BROWSER_BACK
	0,		//VK_LAUNCH_APP2
	0,		//VK_LAUNCH_MAIL
	0,		//VK_LAUNCH_MEDIA_SELECT
};

struct nvec_keyboard
{
	struct input_dev		*input_dev;
	struct task_struct		*task;
	char				name[128];
	int					shutdown;
	unsigned short			keycode[512];
	NvEcHandle			hNvec;
	NvEcEventRegistrationHandle	hEvent;
};

//extern int paz00_set_lcd_output(int mode);

struct input_dev *nvec_input_dev;
NvBool nvec_keyboard_suppend_flag(void)
{
	return b_flag_keyboard_suspend;
}
int nvec_keyboard_report_key(unsigned int code, int value)
{   
    if (!nvec_input_dev){
	pr_info("NVEC keyboard driver no ready!\n");
	return -1;
    }
//*********henry+ enter suspend,disable report data*******************
//Henry+ when suspend only close lip report report KEY_SLEEP ,so need bypass it.
if (b_flag_keyboard_suspend==NV_TRUE && code!=KEY_OPEN) {
	pr_info("NVEC keyboard driver suspend,bypass the report code=x%0x!\n",code);
	return -2;
    }

//*********henry+ enter suspend,disable report data*******************
    input_report_key(nvec_input_dev, code, value); 
    return 0;
}

static int nvec_keyboard_recv(void *arg)
{
	struct input_dev *input_dev = (struct input_dev *)arg;
	struct nvec_keyboard *keyboard = input_get_drvdata(input_dev);        
	/* keyboard event thread should be frozen before suspending the
	 * keyboard and NVEC drivers */
	set_freezable_with_signal();

	while (!keyboard->shutdown) {
		unsigned int pressed;
		NvU32 code;
		NvU8 flags;

		if (!NvOdmKeyboardGetKeyData(&code, &flags, 0)) {

	    	//*********henry+ enter suspend,disable report data*******************
	    	if (b_flag_keyboard_suspend==NV_TRUE && code!=0xe05e){
			pr_info("NVEC keyboard driver suspend,bypass the report code=x%0x!\n",code);
			continue;
	    	}
/* 2010.7.14 remove by Henry,EC behavior have changed, press power button <2sec report event,lid close/open report event
            if (power_button_event == 1) {
                if (lcd_status == 1) {
                    paz00_set_lcd_output(0);  
                    lcd_status = 0;   
                } else if (lcd_status == 0) {
                    paz00_set_lcd_output(1);  
                    lcd_status = 1;                
                }          
            }

            power_button_event = 0;

            if (lid_switch_event == 1) {
                if (lcd_status == 0) {
                    paz00_set_lcd_output(0);  
                } else if (lcd_status == 1) {
                    paz00_set_lcd_output(1);  
                }
            } else if (lid_switch_event == 2) {
                paz00_set_lcd_output(0);  
            }

            lid_switch_event = 0;
*/            
			printk(KERN_INFO "nvec_keyboard: unhandled "
				"scancode %x\n", code);
			continue;
		}

		if (keyboard->shutdown)
			break;
		//*********henry+ enter suspend,disable report data*******************


		if (b_flag_keyboard_suspend==NV_TRUE && code!=0xe05e) {
			pr_info("NVEC keyboard driver suspend,bypass the report code=x%0x!\n",code);
			continue;
	    }
			
		if(code==0x4343)
		{   //henry+ specially handle for F9 key,
		    //report make code and break code.
			input_report_key(keyboard->input_dev, KEY_F9, 2);
			input_report_key(keyboard->input_dev, KEY_F9, 0);			
			continue;
		}

		pressed = (flags & NV_ODM_SCAN_CODE_FLAG_MAKE);
//pr_info(" code=0x%x\n",code);
//pr_info(" pressed=0x%x\n",pressed);
        //power_button_event = 0;

		if ((code >= EC_FIRST_CODE) && (code <= EC_LAST_CODE)) {
			code -= EC_FIRST_CODE;
			code = code_tab_102us[code];
			input_report_key(keyboard->input_dev, code, pressed);
		}
		else if ((code >= EC_EXT_CODE_FIRST) &&
			(code <= EC_EXT_CODE_LAST)) {

			code -= EC_EXT_CODE_FIRST;
			code = extcode_tab_us102[code];
			input_report_key(keyboard->input_dev, code, pressed);
		}
	}

	return 0;
}

static int nvec_keyboard_open(struct input_dev *dev)
{
	return 0;
}

static void nvec_keyboard_close(struct input_dev *dev)
{
	return;
}

static int __devinit nvec_keyboard_probe(struct nvec_device *pdev)
{
	int error;
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	NvError nverr;
#endif
	struct nvec_keyboard *keyboard;
	struct input_dev *input_dev;
	int i;

	keyboard = kzalloc(sizeof(struct nvec_keyboard), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!keyboard || !input_dev) {
		error = -ENOMEM;
		goto fail;
	}

	keyboard->input_dev = input_dev;
	input_set_drvdata(input_dev, keyboard);
	nvec_set_drvdata(pdev, input_dev);

	if (!NvOdmKeyboardInit()) {
		error = -ENODEV;
		pr_err("tegra_keyboard_probe: no keyboard\n");
		goto fail_keyboard_init;
	}

	keyboard->task = kthread_create(nvec_keyboard_recv, input_dev,
		"nvec_keyboard_thread");
	if (keyboard->task == NULL) {
		error = -ENOMEM;
		goto fail_thread_create;
	}
	wake_up_process(keyboard->task);

	if (!strlen(keyboard->name))
		snprintf(keyboard->name, sizeof(keyboard->name),
			 "nvec keyboard");

	input_dev->name = keyboard->name;
	input_dev->open = nvec_keyboard_open;
	input_dev->close = nvec_keyboard_close;

	__set_bit(EV_KEY, input_dev->evbit);
	for (i=1; i<EC_TOTAL_CODES; i++) {
		__set_bit(code_tab_102us[i], input_dev->keybit);
	}
	for (i=1; i<EC_EXT_TOTAL_CODES; i++) {
		__set_bit(extcode_tab_us102[i], input_dev->keybit);
	}

//henry+  press power button time=1sec report sleep scancode
	__set_bit(74, input_dev->keybit);
	__set_bit(KEY_SLEEP, input_dev->keybit);//press power button <2sec for goto sleep
	__set_bit(KEY_F23, input_dev->keybit); //press power button <2sec for turn on/off screen
    __set_bit(KEY_HP, input_dev->keybit);
	nvec_input_dev=keyboard->input_dev;

    /* Report lid switch scan code to OS */
    __set_bit(KEY_OPEN, input_dev->keybit);
    __set_bit(KEY_CLOSE, input_dev->keybit);

	keyboard->hNvec = NULL;
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	/* get EC handle */
	nverr = NvEcOpen(&keyboard->hNvec, 0 /* instance */);
	if (nverr != NvError_Success) {
		error = -ENODEV;
		goto fail_input_register;
	}
#endif

	error = input_register_device(keyboard->input_dev);
	if (error)
		goto fail_input_register;

	return 0;

fail_input_register:
	(void)kthread_stop(keyboard->task);
fail_thread_create:
	NvOdmKeyboardDeInit();
fail_keyboard_init:
fail:
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	NvEcClose(keyboard->hNvec);
	keyboard->hNvec = NULL;
#endif
	input_free_device(input_dev);
	kfree(keyboard);

	return error;
}

static int nvec_keyboard_remove(struct nvec_device *dev)
{
	struct input_dev *input_dev = nvec_get_drvdata(dev);
	struct nvec_keyboard *keyboard = input_get_drvdata(input_dev);

	(void)kthread_stop(keyboard->task);
	NvOdmKeyboardDeInit();
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	NvEcClose(keyboard->hNvec);
	keyboard->hNvec = NULL;
#endif
	keyboard->shutdown = 1;
	input_free_device(input_dev);
	kfree(keyboard);

	return 0;
}

static int nvec_keyboard_suspend(struct nvec_device *pdev, pm_message_t state)
{
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	NvEcRequest Request = {0};
	NvEcResponse Response = {0};
	NvError err = NvError_Success;
#endif
	struct input_dev *input_dev = nvec_get_drvdata(pdev);
	struct nvec_keyboard *keyboard = input_get_drvdata(input_dev);

	if (!keyboard) {
		printk("%s: device handle is NULL\n", __func__);
		return -1;
	}

#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	/* disable keyboard scanning */
	Request.PacketType = NvEcPacketType_Request;
	Request.RequestType = NvEcRequestResponseType_Keyboard;
	Request.RequestSubtype =
		(NvEcRequestResponseSubtype)NvEcKeyboardSubtype_Disable;
	Request.NumPayloadBytes = 0;

	err = NvEcSendRequest(
					keyboard->hNvec,
					&Request,
					&Response,
					sizeof(Request),
					sizeof(Response));
	if (err != NvError_Success) {
		printk("%s: scanning disable request send fail\n", __func__);
		return -1;
	}

	if (Response.Status != NvEcStatus_Success) {
		printk("%s: scanning could not be disabled\n", __func__);
		return -1;
	}
#endif
	/* power down hardware */
	if (!NvOdmKeyboardPowerHandler(NV_TRUE)) {
		printk("%s: hardware power down fail\n", __func__);
		return -1;
	}

	return 0;
}

static int nvec_keyboard_resume(struct nvec_device *pdev)
{
#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	NvEcRequest Request = {0};
	NvEcResponse Response = {0};
	NvError err = NvError_Success;
#endif
	struct input_dev *input_dev = nvec_get_drvdata(pdev);
	struct nvec_keyboard *keyboard = input_get_drvdata(input_dev);

	if (!keyboard) {
		printk("%s: device handle is NULL\n", __func__);
		return -1;
	}

	/* power up hardware */
	if (!NvOdmKeyboardPowerHandler(NV_FALSE)) {
		printk("%s: hardware power up fail\n", __func__);
		return -1;
	}

#if KEYBOARD_SCANNING_DISABLED_IN_SUSPEND
	/* re-enable keyboard scanning */
	Request.PacketType = NvEcPacketType_Request;
	Request.RequestType = NvEcRequestResponseType_Keyboard;
	Request.RequestSubtype =
		(NvEcRequestResponseSubtype)NvEcKeyboardSubtype_Enable;
	Request.NumPayloadBytes = 0;

	err = NvEcSendRequest(
				keyboard->hNvec,
				&Request,
				&Response,
				sizeof(Request),
				sizeof(Response));
	if (err != NvError_Success) {
		printk("%s: scanning enable request send fail\n", __func__);
		return -1;
	}

	if (Response.Status != NvEcStatus_Success) {
		printk("%s: scanning could not be enabled\n", __func__);
		return -1;
	}
#endif

	/* fake key event to turn on display */
	input_report_key(keyboard->input_dev, KEY_MENU, true);
	input_report_key(keyboard->input_dev, KEY_MENU, false);
    //paz00_set_lcd_output(1);  //Henry+ turn on display 2010.6.30  
                                //2010.7.14 remove the code,for close lip driver don't turn off screen,so open lip don't trun on screen.
	return 0;
}

static struct nvec_driver nvec_keyboard_driver = {
	.name		= "nvec_keyboard",
	.probe		= nvec_keyboard_probe,
	.remove		= nvec_keyboard_remove,
	.suspend	= nvec_keyboard_suspend,
	.resume		= nvec_keyboard_resume,
};

//*********henry+ enter suspend,disable report data*******************

static void nvec_keyboard_early_suspend(struct early_suspend *h)
{
	//pr_info("\tNV-KEYBOARD: stop report key code in early suspend --->>>\n");	
	b_flag_keyboard_suspend=NV_TRUE;
}

static void nvec_keyboard_early_resume(struct early_suspend *h)
{
	//pr_info("\tNV-KEYBOARD: restart report key in early resume --->>>\n");
	b_flag_keyboard_suspend=NV_FALSE;		
	
}

static struct early_suspend nvec_keyboard_early_suspend_handler = {
    .suspend = nvec_keyboard_early_suspend,
    .resume = nvec_keyboard_early_resume,
};
//*********henry+ enter suspend,disable report data*******************

static struct nvec_device nvec_keyboard_device = {
	.name	= "nvec_keyboard",
	.driver	= &nvec_keyboard_driver,
};

static int __init nvec_keyboard_init(void)
{
	int err;
	err = nvec_register_driver(&nvec_keyboard_driver);
	if (err)
	{
		pr_err("**nvec_keyboard_init: nvec_register_driver: fail\n");
		return err;
	}

	err = nvec_register_device(&nvec_keyboard_device);
	if (err)
	{
		pr_err("**nvec_keyboard_init: nvec_device_add: fail\n");
		nvec_unregister_driver(&nvec_keyboard_driver);
		return err;
	}

	//henry+ enter suspend,disable report data
	register_early_suspend(&nvec_keyboard_early_suspend_handler);
	return 0;
}

static void __exit nvec_keyboard_exit(void)
{
	//henry+ enter suspend,disable report data
	unregister_early_suspend(&nvec_keyboard_early_suspend_handler);
	nvec_unregister_device(&nvec_keyboard_device);
	nvec_unregister_driver(&nvec_keyboard_driver);
}

module_init(nvec_keyboard_init);
module_exit(nvec_keyboard_exit);

