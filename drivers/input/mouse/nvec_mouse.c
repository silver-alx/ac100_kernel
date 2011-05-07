/*
 * drivers/input/mouse/nvec_mouse.c
 *
 * Mouse class input driver for mice and touchpads connected to an NvEc
 * compliant embedded controller
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
#include <linux/earlysuspend.h>
#include <linux/freezer.h>

#include "nvos.h"
#include "nvec.h"
#include "nvodm_services.h"
#include "nvodm_mouse.h"
#include "nvec_device.h"

#include "nvec_mouse.h"
//#include "elantech_nvec.h"

#include "nvodm_ec.h"

#include <linux/workqueue.h>   //henry++ add reinit

#define DRIVER_DESC "NvEc mouse driver"
#define DRIVER_LICENSE "GPL"

#define MOVE_RATE 2
#define MOUSE_RETRY 20
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

static NvBool resume_init_failed = NV_FALSE; // for F20 issue workaround
//static const NvU8 cmdDisable    = 0xF5;

//extern int enable_sending_scancode(void);
//extern int disable_sending_scancode(void);

//*************************henry+disable/enable wheel scroll
unsigned touchpad_wheel_enable=1;
//set elan wheel mode; mode=1 turn on wheel function ;mode=0 turn off wheel function
void set_elan_wheel_mode(unsigned mode)
{
	touchpad_wheel_enable=mode;
}

//get elan wheel mode; mode=1 turn on wheel function ;mode=0 turn off wheel function
unsigned get_elan_wheel_mode(void)
{
	return touchpad_wheel_enable;
}
EXPORT_SYMBOL_GPL(set_elan_wheel_mode);
EXPORT_SYMBOL_GPL(get_elan_wheel_mode);
//*************************henry+

int touchpad_status = 0;

struct nvec_mouse *ec_odm_touchpad;

int nvec_mouse_cmd(struct nvec_mouse *mouse, unsigned int cmd, int resp_size);

int check_touchpad_status(ec_odm_touchpad_params *touchpad_params)
{
    touchpad_params->read_buf = touchpad_status;

    return 0;    
}

int enable_touchpad(ec_odm_touchpad_params *touchpad_params)
{
    /*
    if (nvec_mouse_cmd(ec_odm_touchpad, cmdEnable, 1)) {
        printk("**touchpad operation: disable touchpad fail!\n");   
        return -1;
    }
    */
    touchpad_status = 1;

    return 0;    
}

int disable_touchpad(ec_odm_touchpad_params *touchpad_params)
{
    /*
    if (nvec_mouse_cmd(ec_odm_touchpad, cmdDisable, 1)) {
        printk("**touchpad operation: disable touchpad fail!\n");   
        return -1;
    }
    */
    touchpad_status = 0;

    return 0;    
}

int nvec_mouse_cmd(struct nvec_mouse *mouse, unsigned int cmd,
	int resp_size)
{
  #if 0
	volatile int cnt = 3;
    int retry_time = 3;

	while (cnt--) {
        /* Retry 3 times when send mouse command failed */
        while (retry_time) {
		    if (!NvOdmMouseSendRequest(mouse->hDevice, cmd, resp_size,
			    &mouse->valid_data_size, mouse->data)) {
                if (retry_time == 1) {
			        pr_err("**nvec_mouse_cmd: SendRequest: fail\n");
                    return -1;
                }
                retry_time--;	
                continue;	    
		    } 

            break;
        }

        if (mouse->valid_data_size != resp_size) {
			pr_err("**nvec_mouse_cmd: not valid data size\n");
			continue;
		}
		if (mouse->valid_data_size == 0 ||
			mouse->data[0] == responseAck) {
			return 0;
		}
	}
	pr_err("**nvec_mouse_cmd error\n");

	return -1;
#endif
	volatile int cnt = 3;
	while (cnt--) {
		if (!NvOdmMouseSendRequest(mouse->hDevice, cmd, resp_size,
			&mouse->valid_data_size, mouse->data)) {
			pr_err("**nvec_mouse_cmd: SendRequest: fail\n");
			continue;
		} if (mouse->valid_data_size != resp_size) {
			pr_err("**nvec_mouse_cmd: not valid data size\n");
			continue;
		}
		if (mouse->valid_data_size == 0 ||
			mouse->data[0] == responseAck) {
			return 0;
		}
	}
	pr_err("**nvec_mouse_cmd error\n");

	return -1;
}

int nvec_IsStandardMouse(struct nvec_mouse *mouse)
{
	if (!nvec_mouse_cmd(mouse, cmdReset, 3) &&
		(mouse->data[1] == responseBatSuccess) &&
		(mouse->data[2] == responseStandardMouseId)) {
		return 0;
	}
	return -ENODEV;
}

static int nvec_setSampleRate(struct nvec_mouse *mouse,
	unsigned int SamplesPerSecond)
{
	if (!nvec_mouse_cmd(mouse, cmdSetSampleRate, 1) &&
		!nvec_mouse_cmd(mouse, SamplesPerSecond, 1)) {
		return 0;
	}
	return -EINVAL;
}

int nvec_IsIntellimouse(struct nvec_mouse *mouse)
{
	/*
	if (((!nvec_setSampleRate(mouse, 0xC8)) &&
		(!nvec_setSampleRate(mouse, 0x64)) &&
		(!nvec_setSampleRate(mouse, 0x50)))
		&& ((!nvec_mouse_cmd(mouse, cmdReadId, 2)) &&
			(mouse->data[1] == responseIntelliMouseId))) {

		printk("mouse->data[1]=0x%x\n", mouse->data[1]);
		return 0;
	}
	return -ENODEV;
	*/
	//PAZ00 touchpad is 4byte mouse.
	return 0;
}

int nvec_IsIntelli5buttonmouse(struct nvec_mouse *mouse)
{
	/*
	if (((!nvec_setSampleRate(mouse, 0xC8)) &&
		 (!nvec_setSampleRate(mouse, 0xC8)) &&
		 (!nvec_setSampleRate(mouse, 0x50)))
		&& ((!nvec_mouse_cmd(mouse, cmdReadId, 2)) &&
		mouse->data[1] == responseIntelli5buttonMouseId)) {
		printk("mouse->data[1]=0x%x\n", mouse->data[1]);
		return 0;
	}
	return -ENODEV;
	*/
	//PAZ00 touchpad is not 5buttonmouse. 
	return -ENODEV;
}

//henry++ re-init while touchpad fail after resume from suspend
#define REPORT_COUNT_ZERO_ERROR 20
#if 0
static int i_count_y=0,i_count_x=0;
static int i_bypass_y=0,i_bypass_x=0;
static NvBool b_check_x=false,b_check_y=false;
#endif

static NvBool b_check_data1=false;
static int i_count_data1=0;

void re_init_tp(void);

struct delayed_work mouse_work;

static void work_mouse_init(struct work_struct *data)
{
        re_init_tp();
}



static int nvec_mouse_receive_thread(void *arg)
{
	struct input_dev *input_dev = (struct input_dev *)arg;
	struct nvec_mouse *mouse = input_get_drvdata(input_dev);
	NvU8	buttonState;
	NvU8	updatedState;
	NvS8	dataBuffer[4] = {0};
	NvU32   x,y;

	/* mouse event thread should be frozen before suspending the
	 * mouse and NVEC drivers */
	set_freezable_with_signal();

	if (!NvOdmMouseEnableInterrupt(mouse->hDevice, mouse->semaphore)) {
		pr_err("**nvec_mouse_receive_thread: EnableInterrupt: fail\n");
		return -1;
	}

	while (!mouse->shutdown) {
		unsigned char data[4];
		int size;

		NvOdmOsSemaphoreWait(mouse->semaphore);
		if (mouse->shutdown)
			break;

		if (!NvOdmMouseGetEventInfo(mouse->hDevice, &size, data))
			continue;

		/* Short packets are not valid? */
		if (size < 3) continue;

		NvOsMemcpy(&dataBuffer, data, 4);

//pr_info("INFO>>data[0]=0x%x,data[1]=0x%x,data[2]=0x%x,data[3]=0x%x\n",data[0],data[1],data[2],data[3]);

//add check data1 begin ********
if (b_check_data1){
	if( (data[0] & 0x08) == 0){
	   i_count_data1++;
	   //pr_info("INFO>>check i_count_data1=%i\n",i_count_data1);
	   if (i_count_data1>REPORT_COUNT_ZERO_ERROR){
		pr_info("INFO>>data1 is wrong,reinit TP\n");
		//re_init_tp();
		b_check_data1=false;
#if 0
		b_check_x=false;
		b_check_y=false;
#endif
		schedule_delayed_work(&mouse_work,0.5 * HZ);
		//pr_info("INFO>>call reinit TP done\n");
	   }
	}
	else
	{
	  //pr_info("INFO>>data1 check bypass\n");
	  b_check_data1=false;
	}
}

//add check data1 end *******
		updatedState = dataBuffer[0] ^ mouse->previousState;
		if (updatedState) {
			buttonState = dataBuffer[0];

			if (updatedState & 0x01)
				input_report_key(input_dev, BTN_LEFT,
					buttonState & 0x01);
			if (updatedState & 0x02)
				input_report_key(input_dev, BTN_RIGHT,
					(buttonState>>1) & 0x01);
                        if (updatedState & 0x04)
				input_report_key(input_dev, BTN_MIDDLE,
					(buttonState>>2) & 0x01);

			mouse->previousState = dataBuffer[0];
		} else {
			x = dataBuffer[1];
			y = -dataBuffer[2];
                      //pr_info("\n\nINFO>>x=%d,y=%d\n\n",x,y);
//add check x data is right begin ***
#if 0
			if (b_check_x){
			   //pr_info("INFO>>check x data\n");
			   if(x==0){
			     i_count_x++;
			     //pr_info("INFO>>x i_count_x=%i\n",i_count_x);
			     if(i_count_x>REPORT_COUNT_ZERO_ERROR){
				pr_info("INFO>>x is report zero more time,reinit TP\n");
				//re_init_tp();
				b_check_data1=false;
				b_check_x=false;
				b_check_y=false;
				schedule_delayed_work(&mouse_work,0.5 * HZ);
				//pr_info("INFO>>call reinit TP done\n");

			     }
			   }
			   else{
				i_bypass_x++;
				if(i_bypass_x>2){
					//pr_info("INFO>>x check bypass\n");				
					b_check_x=false;
				}
				else{
					i_count_x=0;
				}
			   }
			}
#endif
//add check x data is right end ***

//add check y data is right begin ********
#if 0
			if (b_check_y){
			   //pr_info("INFO>>check y data\n");
			   if(y==0){
			     i_count_y++;
		             //pr_info("INFO>>i_count_y=%i\n",i_count_y);
			     if(i_count_y>REPORT_COUNT_ZERO_ERROR){
				pr_info("INFO>>y is report zero more time,reinit TP\n");
				//re_init_tp();
				b_check_data1=false;
				b_check_x=false;
				b_check_y=false;
				schedule_delayed_work(&mouse_work, 0.5 * HZ);
				//pr_info("INFO>>call reinit TP done\n");
				}
			   }
			   else{
				i_bypass_y++;
				if(i_bypass_y>2){	
					//pr_info("INFO>>y check bypass\n");			
					b_check_y=false;
				}
				else{
					i_count_y=0;
				}
			   }
			}
#endif
//add check y data is right end ********

			input_report_rel(input_dev, REL_X, MOVE_RATE * x);
			input_report_rel(input_dev, REL_Y, MOVE_RATE * y);

			input_sync(input_dev);
		}

		if (mouse->packetSize == 4 && dataBuffer[3] && touchpad_wheel_enable){
		    //henry+ support synaptics
		    if (data[3]>127 && data[3]<160)   
			input_report_rel(input_dev, REL_WHEEL, -1); //down
		   else if (data[3]>160)
		        input_report_rel(input_dev, REL_WHEEL, 1 );  //up
		   else
		       input_report_rel(input_dev, REL_WHEEL, -1 * dataBuffer[3]);  //henry input_report_rel(input_dev, REL_WHEEL, dataBuffer[3]);
		}

		input_sync(input_dev);
	}
	return 0;
}

static NvBool nvec_mouse_setting(struct nvec_mouse *mouse)
{
	// change to absolute mode! 
	// Why need three setSampleRate command ???
	if (nvec_setSampleRate(mouse, 0xC8)) {
        pr_err("**mouse_resume: setsamplerate fail, 0xC8\n");
		return NV_FALSE;
	}
	if (nvec_setSampleRate(mouse, 0x64)) {
        pr_err("**mouse_resume: setsamplerate fail, 0x64\n");
		return NV_FALSE;
	}
	if (nvec_setSampleRate(mouse, 0x50)) {
        pr_err("**mouse_resume: setsamplerate fail, 0x50\n");
		return NV_FALSE;
	}
	
	/* Set the resolution */
    if (nvec_mouse_cmd(mouse, cmdSetResolution, 1) ||
        nvec_mouse_cmd(mouse, 2, 1)) {
        pr_err("**mouse_resume: cmd fail\n");
		return NV_FALSE;
	}

	/* Set scaling */
    if (nvec_mouse_cmd(mouse, cmdSetScaling1_1, 1)) {
        pr_err("**mouse_resume: cmdsetscaling fail\n");
		return NV_FALSE;
	}

    //Henry:add run set auto-receive byte command before enable touchpad. 2010.7.13
    if (!NvOdmMouseStartStreaming(mouse->hDevice, mouse->packetSize)) {
		pr_err("**mouse_resume: streaming fail\n");		
		return NV_FALSE;
	}

	if (nvec_mouse_cmd(mouse, cmdEnable, 1)) {
        pr_err("**mouse_resume: mouse cmd fail\n");
		return NV_FALSE;
	}

    if (!NvOdmMousePowerResume(mouse->hDevice)) {
	    pr_err("**mouse_resume: NvOdmMousePowerResume fail\n");
		return NV_FALSE;
	}

	return NV_TRUE;
}

static int nvec_mouse_open(struct input_dev *dev)
{
	struct nvec_mouse *mouse = input_get_drvdata(dev);
	int retry = MOUSE_RETRY;
    	ec_odm_touchpad = input_get_drvdata(dev);

	// do mouse settings!
	
	do {
        if (!nvec_mouse_setting(mouse))
			NvOsWaitUS(50000);
		else
			break;
	} while (retry-- > 0);


	return 0;
}

static void nvec_mouse_close(struct input_dev *dev)
{
	return;
}

static int mouse_resume(struct nvec_mouse *mouse)
{
	if (!mouse || !mouse->hDevice)
		return -1;

    	resume_init_failed = NV_TRUE;


	return 0;
}

static int mouse_suspend(struct nvec_mouse *m)
{
	if (!m || !m->hDevice || !NvOdmMousePowerSuspend(m->hDevice))
		return -1;

	return 0;
}

static int nvec_mouse_suspend(struct nvec_device *pdev, pm_message_t state)
{
	if (pdev==NULL)
		return 0;        
	struct input_dev *input_dev = nvec_get_drvdata(pdev);
	if (input_dev==NULL)
		return 0;
	struct nvec_mouse *mouse = input_get_drvdata(input_dev);
	if (mouse==NULL)
		return 0;

    //disable_sending_scancode();
	return mouse_suspend(mouse);
}

static int nvec_mouse_resume(struct nvec_device *pdev)
{
	if (pdev==NULL)
		return 0;  
	struct input_dev *input_dev = nvec_get_drvdata(pdev);
	if (input_dev==NULL)
		return 0;
	struct nvec_mouse *mouse = input_get_drvdata(input_dev);
	if (mouse==NULL)
		return 0;

    //enable_sending_scancode();
	return mouse_resume(mouse);
}

static void nvec_mouse_early_suspend(struct early_suspend *h)
{
	struct nvec_mouse *mouse;
	mouse = container_of(h, struct nvec_mouse, early_suspend);
	nvec_mouse_cmd(mouse, cmdDisable, 1);
	//pr_info("Henry: disable mouse in early suspend ====\n");
}

static void nvec_mouse_late_resume(struct early_suspend *h)
{	
	struct nvec_mouse *mouse;
	NvBool ret;
        int    retry;
//pr_info("INFO>>%s enter...\n",__FUNCTION__);
	mouse = container_of(h, struct nvec_mouse, early_suspend);
	
	if (resume_init_failed == NV_FALSE) {
	    nvec_mouse_cmd(mouse, cmdEnable, 1);
	    //pr_info("Henry: enable mouse in late resume ====\n");
		return;
    }

#if 0
      b_check_x=false;
      b_check_y=false;
#endif
      b_check_data1=false;

	resume_init_failed = NV_FALSE;
        
	// reset again!
	retry = MOUSE_RETRY;
    do {
		if ((ret = NvOdmMouseReset(mouse->hDevice)) == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);
	if (retry == 0) goto tp_failed;

	// do mouse settings again!
	retry = MOUSE_RETRY;
	do {
	    if ((ret = nvec_mouse_setting(mouse)) == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);
#if 0
	if (retry == 0) goto tp_failed;
     
	// enable mouse!
	retry = MOUSE_RETRY;
	do {
	    ret = (nvec_mouse_cmd(mouse, cmdEnable, 1) == 0) ? 
		          NV_TRUE : NV_FALSE;
		if (ret == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);
#endif 

     // pr_info("INFO>>%s end...\n",__FUNCTION__);
tp_failed:
    if (retry == 0)
	    pr_info("\tIf F20 still happend, then the workaround is failed!\n");
      //init re-check
#if 0
      i_count_y=0;
      i_count_x=0;     
      i_bypass_x=0;
      i_bypass_y=0;   
#endif   
      i_count_data1=0;

      b_check_data1=true;
#if 0
      b_check_x=true;
      b_check_y=true;
#endif

}

// re-init touchpad function
void re_init_tp(void)
{	
	struct nvec_mouse *mouse;
	NvBool ret;
        int    retry;
	//pr_info("\tINFO>>%s enter...\n",__FUNCTION__);
if (ec_odm_touchpad==NULL){
	pr_info("\tINFO>>touchpad is not ready!\n");
	return;
}
#if 0
      b_check_x=false;
      b_check_y=false;
#endif
      b_check_data1=false;

      mouse = ec_odm_touchpad;
	
      nvec_mouse_cmd(mouse, cmdDisable, 1);
	   
        
	// reset again!
	retry = MOUSE_RETRY;
    do {
		if ((ret = NvOdmMouseReset(mouse->hDevice)) == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);
	if (retry == 0) goto tp_failed;

	// do mouse settings again!
	retry = MOUSE_RETRY;
	do {
	    if ((ret = nvec_mouse_setting(mouse)) == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);
#if 0
	if (retry == 0) goto tp_failed;
     
	// enable mouse!
	retry = MOUSE_RETRY;
	do {
	    ret = (nvec_mouse_cmd(mouse, cmdEnable, 1) == 0) ? 
		          NV_TRUE : NV_FALSE;
		if (ret == NV_FALSE)
			NvOsWaitUS(50000);
	} while (ret == NV_FALSE && retry-- > 0);

#endif
	//pr_info("\tINFO>>%s end...\n",__FUNCTION__);
tp_failed:
    if (retry == 0)
	    pr_info("\tIf F20 still happend, then the workaround is failed!\n");

      //init re-check
#if 0
      i_count_y=0;
      i_count_x=0;     
      i_bypass_x=0;
      i_bypass_y=0;   
#endif   
      i_count_data1=0;

      b_check_data1=true;
#if 0
      b_check_x=true;
      b_check_y=true;
#endif

}


static int __devinit nvec_mouse_probe(struct nvec_device *pdev)
{
	int error;
	struct nvec_mouse *mouse;
	struct input_dev *input_dev;
    int    retry;

    INIT_DELAYED_WORK(&mouse_work, work_mouse_init);
	

	mouse = kzalloc(sizeof(struct nvec_mouse), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!mouse || !input_dev) {
		pr_err("**nvec_mouse_probe: input_allocate_device: fail\n");
		error = -ENOMEM;
		goto fail;
	}

    // must be successful...
	retry = MOUSE_RETRY;
    do {
	    if (!NvOdmMouseDeviceOpen(&mouse->hDevice)) {
		    pr_err("NvOdmMouseDeviceOpen failed\n");
		    //error = -ENODEV;
		    //goto fail;
			NvOsWaitUS(10000);
	    } else
		    break;
	} while (retry-- > 0);
         
        if(retry <= 0){
		error = -ENODEV;
		goto fail;
        }
	

	mouse->input_dev = input_dev;
	input_set_drvdata(input_dev, mouse);

	mouse->type = nvec_mouse_type_none;
	mouse->enableWake = 0;


#if 0
	/* Probe for the type of the mouse */
	if (!nvec_IsStandardMouse(mouse)) {
		if (!nvec_IsIntellimouse(mouse)) {
			mouse->packetSize = 4;
			if (!nvec_IsIntelli5buttonmouse(mouse))
				mouse->type = nvec_mouse_type_intelli5buttonmouse;
			else
				mouse->type = nvec_mouse_type_intellimouse;
		} else {
			mouse->packetSize = 3;
			mouse->type = nvec_mouse_type_standard;
		}
	} else {
		error = -ENODEV;
		goto fail_no_mouse_found;
	}
#endif
    //add retry 8/17
    retry = MOUSE_RETRY;
    do {
	    if (!nvec_IsStandardMouse(mouse)) {
			if (!nvec_IsIntellimouse(mouse)) {
				mouse->packetSize = 4;
				if (!nvec_IsIntelli5buttonmouse(mouse))
					mouse->type = nvec_mouse_type_intelli5buttonmouse;
				else
					mouse->type = nvec_mouse_type_intellimouse;
			} else {
				mouse->packetSize = 3;
				mouse->type = nvec_mouse_type_standard;
			}
		  break;
		} else {
			NvOsWaitUS(10000);
			pr_err("nvec_mouse: nvec_IsStandardMouse fail,will retry!\n");			
		}

	} while (retry-- > 0);
        if(retry <= 0){
		error = -ENODEV;
		goto fail_no_mouse_found;
        }
     //add retry 8/17

	nvec_set_drvdata(pdev, input_dev);

	mouse->semaphore = NvOdmOsSemaphoreCreate(0);
	if (mouse->semaphore == NULL) {
		error = -1;
		pr_err("nvec_mouse: Semaphore creation failed\n");
		goto fail_semaphore_create;
	}

	mouse->task = kthread_create(nvec_mouse_receive_thread,
		input_dev, "nvec_mouse");

	if (!mouse->task) {
		pr_err("**nvec_mouse_probe: kthread_create: fail\n");
		error = -ENOMEM;
		goto fail_thread_create;
	}

	wake_up_process(mouse->task);

	if (!strlen(mouse->name))
		snprintf(mouse->name, sizeof(mouse->name), "nvec mouse");

	input_dev->name = mouse->name;
	input_dev->open = nvec_mouse_open;
	input_dev->close = nvec_mouse_close;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);

	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
	BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

	error = input_register_device(mouse->input_dev);
	if (error) {
		pr_err("**nvec_mouse_probe: input_register_device: fail\n");
		goto fail_input_register;
	}
    
	mouse->early_suspend.suspend = nvec_mouse_early_suspend;
	//for reducing resume time! don't set the early suspend level!
	//mouse->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB; 
	mouse->early_suspend.resume = nvec_mouse_late_resume; 
	register_early_suspend(&mouse->early_suspend);

	mouse->previousState = 0;
    
	resume_init_failed = NV_FALSE; // for F20 issue workaround!

	printk(KERN_INFO DRIVER_DESC ": registered NVEC mouse driver\n");
	return 0;

fail_input_register:
	(void)kthread_stop(mouse->task);
fail_thread_create:
	NvOdmOsSemaphoreDestroy(mouse->semaphore);
	mouse->semaphore = NULL;
fail_semaphore_create:
fail_no_mouse_found:
	pr_err("**nvec_mouse_probe: fail_no_mouse_found fail\n");
	NvOdmMouseDeviceClose(mouse->hDevice);
	mouse->hDevice = NULL;

fail:
	input_free_device(input_dev);
	kfree(mouse);
	mouse = NULL;
	//return error; //S3 hang up issue workaround 8/17
	return 0;
}

static void nvec_mouse_remove(struct nvec_device *dev)
{
	struct input_dev *input_dev = nvec_get_drvdata(dev);
	struct nvec_mouse *mouse = input_get_drvdata(input_dev);

	unregister_early_suspend(&mouse->early_suspend);
	
	mouse->shutdown = 1;
	NvOdmOsSemaphoreSignal(mouse->semaphore);
	(void)kthread_stop(mouse->task);
	NvOdmOsSemaphoreDestroy(mouse->semaphore);
	NvOdmMouseDeviceClose(mouse->hDevice);
	input_free_device(input_dev);
	kfree(mouse);
	mouse = NULL;
}

static struct nvec_driver nvec_mouse = {
	.name		= "nvec_mouse",
	.probe		= nvec_mouse_probe,
	.remove		= nvec_mouse_remove,
	.suspend	= nvec_mouse_suspend,
	.resume		= nvec_mouse_resume,
};

static struct nvec_device nvec_mouse_device = {
	.name	= "nvec_mouse",
	.driver	= &nvec_mouse,
};

static int __init nvec_mouse_init(void)
{
	int err;

	err = nvec_register_driver(&nvec_mouse);
	if (err)
	{
		pr_err("**nvec_mouse_init: nvec_register_driver: fail\n");
		return err;
	}

	err = nvec_register_device(&nvec_mouse_device);
	if (err)
	{
		pr_err("**nvec_mouse_init: nvec_device_add: fail\n");
		nvec_unregister_driver(&nvec_mouse);
		return err;
	}

	return 0;
}

static void __exit nvec_mouse_exit(void)
{
	nvec_unregister_device(&nvec_mouse_device);
	nvec_unregister_driver(&nvec_mouse);
}

module_init(nvec_mouse_init);
module_exit(nvec_mouse_exit);

