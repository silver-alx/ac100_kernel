#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/tegra_devices.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>

#include "nvos.h"
#include "nvec.h"
#include "nvodm_services.h"
#include "nvec_device.h"

MODULE_AUTHOR("John Ye");
MODULE_DESCRIPTION("Compal EC Event Driver");
MODULE_LICENSE("GPL");

extern NvEcHandle s_NvEcHandle;
static NvEcEventType EventTypes[] = {NvEcEventType_System};  // get system event
static NvEcEvent ec_event_packet = {0};
static NvOdmOsSemaphoreHandle s_ec_event_sema = NULL;
static NvEcEventRegistrationHandle s_ec_event_registration = NULL;
static NvBool s_ec_eventDeinit = NV_FALSE;

//static int lcd_status = 1;
//static int lid_event_value = 2;//set lid open status as default


static int usb_current_status = 0;//set no USB over current as default status


static NvU8 ac_event_value=0; 
static NvU8 lid_event_value = 0;
static NvU8 usb_event_value=0;

#define CONTROL_LCD         0
#define CONTROL_SUSPEND     1

int lid_switch_function_flag = CONTROL_LCD;
int power_button_function_flag = CONTROL_LCD;

struct ec_odm_event
{
	struct input_dev		*input_dev;
	struct task_struct		*task;
	char				name[128];
	int					shutdown;
	NvEcHandle			hNvec;
	NvEcEventRegistrationHandle	hEvent;
};

NvBool ec_odm_event_register(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    s_ec_event_sema = NvOdmOsSemaphoreCreate(0);
    
    /* register for system events */
    NvStatus = NvEcRegisterForEvents(
                    s_NvEcHandle,       // nvec handle
                    &s_ec_event_registration,
                    (NvOsSemaphoreHandle)s_ec_event_sema,
                    sizeof(EventTypes)/sizeof(NvEcEventType),
                    EventTypes, //
                    1,          // currently buffer only 1 packet from ECI at a time
                    sizeof(NvEcEvent));
    

    /* success */
    return NV_TRUE;
}

int lid_switch_control_lcd(void)
{
    //printk("%s\n", __FUNCTION__);
    lid_switch_function_flag = CONTROL_LCD;
    return 0;
}

int lid_switch_control_suspend(void)
{
    //printk("%s\n", __FUNCTION__);
    lid_switch_function_flag = CONTROL_SUSPEND;
    return 0;
}

extern int paz00_set_lcd_output(int mode);
extern int nvec_keyboard_report_key(unsigned int code, int value);
extern NvBool nvec_keyboard_suppend_flag(void);
extern void report_AC_change(void);
extern NvU32 get_systemevent_status(void);
static int lid_switch(int event_value)
{
    //printk("lid switch\n");   
    if (event_value == 0x00) {    
        if (lid_switch_function_flag == CONTROL_LCD) {
	   //modify by henry 2010.7.7, open lip need turn on lcd or wakeup system
	   // paz00_set_lcd_output(1);  2010.7.14  remove turn on/off screen,report scancode let's android turn on/off screen
           
            nvec_keyboard_report_key(KEY_OPEN, 2);
            nvec_keyboard_report_key(KEY_OPEN, 0);
        } else   
	{	//modify by henry 2010.7.7, open lip need turn on lcd or wakeup system   	  
	    // paz00_set_lcd_output(1); 2010.7.14  remove turn on/off screen,report scancode let's android turn on/off screen
	        nvec_keyboard_report_key(KEY_OPEN, 2);
            nvec_keyboard_report_key(KEY_OPEN, 0);	    
	}
	
        
    } else if (event_value == 0x02) {
        if (lid_switch_function_flag == CONTROL_LCD) { 
            //paz00_set_lcd_output(0);    2010.7.14  remove turn on/off screen,report scancode let's android turn on/off screen
        
            nvec_keyboard_report_key(KEY_CLOSE, 2);
            nvec_keyboard_report_key(KEY_CLOSE, 0);
        } else if (lid_switch_function_flag == CONTROL_SUSPEND) {
          #if 0
            nvec_keyboard_report_key(KEY_SLEEP, 2);
	        nvec_keyboard_report_key(KEY_SLEEP, 0); 
          #endif
            nvec_keyboard_report_key(KEY_HP,2);
            nvec_keyboard_report_key(KEY_HP,0);
        }
    }
	
   return 0;
}

static int read_usb_current(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len = sprintf(page, "%d\n", usb_current_status);
    return len;
}

static int usb_current_update(int event_value)
{
    if (event_value == 0x40) {
        usb_current_status = 1;
    } else if (event_value == 0x00) {
        usb_current_status = 0;
    }

    //printk("usb_current_status = %d\n", usb_current_status);
    return 0;
}

int power_button_control_lcd(void)
{
    //printk("%s\n", __FUNCTION__);
    power_button_function_flag = CONTROL_LCD;
    return 0;
}

int power_button_control_suspend(void)
{
    //printk("%s\n", __FUNCTION__);
    power_button_function_flag = CONTROL_SUSPEND;
    return 0;
}

static int power_button_fun(void)
{
    if (power_button_function_flag == CONTROL_LCD) {
	//2010.7.14  remove turn on/off screen,report new scan code let's android turn on/off screen,android need change,have confirmed with Dig.
            nvec_keyboard_report_key(KEY_F23, 2);
            nvec_keyboard_report_key(KEY_F23, 0);

    } else if (power_button_function_flag == CONTROL_SUSPEND) {
	    //henry update
	    NvBool suspend_flag=nvec_keyboard_suppend_flag();
	    if(suspend_flag==NV_TRUE){   //if enter early suspend,press power button wake up system
			nvec_keyboard_report_key(KEY_OPEN, 2);
			nvec_keyboard_report_key(KEY_OPEN, 0);
	    }
	    else{			
	        nvec_keyboard_report_key(KEY_SLEEP, 2);
	        nvec_keyboard_report_key(KEY_SLEEP, 0);
	    }
    }  
    return 0;
} 

NvBool get_ec_odm_event_data()
{
    NvError NvStatus = NvError_Success;

    NvOdmOsSemaphoreWait(s_ec_event_sema);

    NvStatus = NvEcGetEvent(s_ec_event_registration, &ec_event_packet, sizeof(NvEcEvent));

   //henry+2010.7.27 add AC enent handle,modify judge event type rule
    if ( (ec_event_packet.Payload[2] & 0x80) == 0x80 ) {	
        power_button_fun();
    } else if ( (ec_event_packet.Payload[2] & 0x02) != lid_event_value ) {
	lid_event_value = ec_event_packet.Payload[2] & 0x02;
        lid_switch(lid_event_value); 
    } else if ( (ec_event_packet.Payload[0] & 0x01) !=ac_event_value ) {	
	ac_event_value=ec_event_packet.Payload[0] & 0x01;
        report_AC_change();   
    } else if ( (ec_event_packet.Payload[2] & 0x40) != usb_event_value ) {	
	usb_event_value=ec_event_packet.Payload[2] & 0x40;
        usb_current_update(usb_event_value);         
    } else {
        pr_info("Err:system event,don't be here\n");
	//usb_event_value=ec_event_packet.Payload[2] & 0x40;
        //usb_current_update(usb_event_value); 

	//lid_event_value = (ec_event_packet.Payload[2] & 0x02);
        //lid_switch(lid_event_value);  

	//ac_event_value=ec_event_packet.Payload[0] & 0x01;
        //report_AC_change(); 
	
   }


    return NV_TRUE;   
}

static int ec_odm_event_recv(void *arg)
{
	struct input_dev *input_dev = (struct input_dev *)arg;
	struct ec_odm_event *ec_event = input_get_drvdata(input_dev);

	set_freezable_with_signal();

	while (1) {

		get_ec_odm_event_data();
		
	}

	return 0;
}

static int ec_odm_event_open(struct input_dev *dev)
{
	return 0;
}

static void ec_odm_event_close(struct input_dev *dev)
{
	return;
}

void ec_odm_event_unregister(void)
{
    (void)NvEcUnregisterForEvents(s_ec_event_registration);
    s_ec_event_registration = NULL;

    NvOdmOsSemaphoreSignal(s_ec_event_sema);
    NvOdmOsSemaphoreDestroy(s_ec_event_sema);
    s_ec_event_sema = NULL;
}

static int __devinit ec_odm_event_probe(struct nvec_device *pdev)
{
	int error;
	NvError nverr;
	struct ec_odm_event *ec_event;
	struct input_dev *input_dev;
	int i;
//henry+2010.7.27  add system event current status begin******
	NvU32 sys_event_status;
	sys_event_status=get_systemevent_status();	
        if(sys_event_status>0){	
		ac_event_value = (NvU8) (sys_event_status & 0x01);	
		lid_event_value = (NvU8)((sys_event_status >>16) & 0x02);
                usb_event_value = (NvU8)((sys_event_status >>16) & 0x40);   
	        //pr_info("henry>>ac_event_value=0x%x,lid_event_value=0x%x,usb_event_value=0x%x\n",ac_event_value,lid_event_value,usb_event_value);         
	}
//henry+2010.7.27 add system event current status end******

	ec_event = kzalloc(sizeof(struct ec_odm_event), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ec_event || !input_dev) {
		error = -ENOMEM;
		goto fail;
	}

	ec_event->input_dev = input_dev;
	input_set_drvdata(input_dev, ec_event);
	nvec_set_drvdata(pdev, input_dev);

	if (!ec_odm_event_register()) {
		error = -ENODEV;
		pr_err("ec_event_probe: no EC system event\n");
		goto fail_ec_event_init;
	}

	ec_event->task = kthread_create(ec_odm_event_recv, input_dev,
		"ec_odm_event_thread");
	if (ec_event->task == NULL) {
		error = -ENOMEM;
		goto fail_thread_create;
	}
	wake_up_process(ec_event->task);

	//if (!strlen(ec_event->name))
	//	snprintf(ec_event->name, sizeof(ec_event->name),
	//		 "nvec ec_event");

	input_dev->name = "nvec ec_event";
	input_dev->open = ec_odm_event_open;
	input_dev->close = ec_odm_event_close;

	error = input_register_device(ec_event->input_dev);
	if (error)
		goto fail_input_register;

	return 0;

fail_input_register:
	(void)kthread_stop(ec_event->task);
fail_thread_create:
	ec_odm_event_unregister();
fail_ec_event_init:
fail:
	input_free_device(input_dev);
	kfree(ec_event);

	return error;
}

static int ec_odm_event_remove(struct nvec_device *dev)
{
	struct input_dev *input_dev = nvec_get_drvdata(dev);
	struct ec_odm_event *ec_event = input_get_drvdata(input_dev);

	(void)kthread_stop(ec_event->task);
	ec_odm_event_unregister();
	ec_event->shutdown = 1;
	input_free_device(input_dev);
	kfree(ec_event);

	return 0;
}

static int ec_odm_event_suspend(struct nvec_device *pdev, pm_message_t state)
{
    //pr_info("henry>>%s\n",__FUNCTION__);         
    return 0;
}

static int ec_odm_event_resume(struct nvec_device *pdev)
{
//henry+2010.7.30  add system event current status begin******
	NvU32 sys_event_status;
        NvU8 ac_status;
        NvU8 usb_status;
	//pr_info("henry>>%s\n",__FUNCTION__);

	sys_event_status=get_systemevent_status();	        
	ac_status = (NvU8) (sys_event_status & 0x01);	
        if(ac_event_value!=ac_status){
	   ac_event_value=ac_status;
	   report_AC_change();  
	}

	lid_event_value = (NvU8)((sys_event_status >>16) & 0x02);

        usb_status=(NvU8)((sys_event_status >>16) & 0x40); 
        if(usb_event_value!=usb_status){
	   usb_event_value=usb_status;
           usb_current_update(usb_event_value);
	}
          
	//pr_info("henry>>ac_event_value=0x%x,lid_event_value=0x%x,usb_event_value=0x%x\n",ac_event_value,lid_event_value,usb_event_value);         	
//henry+2010.7.30 add system event current status end******

    return 0;
}

static struct nvec_driver ec_odm_event_driver = {
	.name		= "ec_odm_event",
	.probe		= ec_odm_event_probe,
	.remove		= ec_odm_event_remove,
        .suspend	= ec_odm_event_suspend,
	.resume		= ec_odm_event_resume,
};

static struct nvec_device ec_odm_event_device = {
	.name	= "ec_odm_event",
	.driver	= &ec_odm_event_driver,
};

static int __init ec_odm_event_init(void)
{
	int err;
    struct proc_dir_entry *ec_odm_dir;
    struct proc_dir_entry *usb_current_file;

	err = nvec_register_driver(&ec_odm_event_driver);
	if (err)
	{
		pr_err("**ec_odm_event_init: nvec_register_driver: fail\n");
		return err;
	}

	err = nvec_register_device(&ec_odm_event_device);
	if (err)
	{
		pr_err("**ec_odm_event_init: nvec_device_add: fail\n");
		nvec_unregister_driver(&ec_odm_event_driver);
		return err;
	}

    ec_odm_dir = proc_mkdir("ec_odm", NULL);

    usb_current_file = create_proc_entry("usb_current", 0666, ec_odm_dir);
    if(usb_current_file == NULL) {
        goto err_use_current_file;
    }
    
    usb_current_file->read_proc = read_usb_current;

	return 0;

err_use_current_file:
    remove_proc_entry("usb_current", ec_odm_dir);

    return -ENODEV;
}

static void __exit ec_odm_event_exit(void)
{
	nvec_unregister_device(&ec_odm_event_device);
	nvec_unregister_driver(&ec_odm_event_driver);
}

module_init(ec_odm_event_init);
module_exit(ec_odm_event_exit);

