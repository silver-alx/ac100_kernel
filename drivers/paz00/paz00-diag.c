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
#include <linux/notifier.h>
#include <linux/cpu.h>

#define DRV_VER "0.1"
#define MODULE_NAME "test_program"
#define DIR_NAME "test_program"
#define PROCFS_MAX_SIZE		256

#define R0_REG	0x0
#define R1_REG	0x1
#define R2_REG	0x2
#define R3_REG	0x3
#define R25_REG	0x19
#define R9_REG		0x9


struct platform_device *paz00_diag_dev;
static NvEcHandle s_NvEcHandle = NULL;
static struct proc_dir_entry *ec_dir, *ec_ver_file, *uuid_file, *usb_file, *lsensor_file, *button_file, *led_file, *light_file, *loopback_file, *ecctl_file, *coldboot_file, *hdmi_file, *gpio_file, *wifi3g_file, *suspend_file, *tpwheel_file, *lcd_edid_file, *ec_timer_file,*battery_file,*batteryManufacturer_file,*batteryCellNumber_file;
char strVersion[30] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };				// EC Version
NvU8 EcCtlResponse[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };	// Resoponse for EC utility
int NumEcResponse = 0;
NvU8 global_coldboot = 0;

// Reference EC Specification
#define COMPAL_EC_READ				0x00		// Read EC RAM space
#define COMPAL_EC_WRITE				0x01		// Write EC RAM space
#define COMPAL_EC_COMMAND			0x10		// Execute EC command
#define COMPAL_EC_COMMAND_READ_ADDRESS		0xb3		// Read specified address
#define COMPAL_EC_COMMAND_WRITE_ADDRESS		0xb4		// Write speicied address
#define COMPAL_EC_ADDRESS_READ_UUID		0x4e		// Read UUID
#define COMPAL_EC_ADDRESS_WRITE_UUID		0x4d		// Write UUID
#define COMPAL_EC_SET_DEVICE			0x45		// set device status
#define COMPAL_EC_GET_DEVICE_STATUS		0xd0		// get device status
#define COMPAL_EC_SET_USB_DEVICE		0xd3		// Set USB1 as device
#define COMPAL_EC_SET_USB_HOST			0xd4		// Set USb1 as host
#define COMPAL_EC_ENABLE_LIGHT_SENSOR		0xd5		// Enable light sensor
#define COMPAL_EC_DISABLE_LIGHT_SENSOR		0xd6		// Disable light sensor
#define COMPAL_EC_BLCTL_ADDRESS_H		0xf4		// Command for back light control
#define COMPAL_EC_BLCTL_ADDRESS_L		0xb9		// Command for back light control
#define COMPAL_EC_COMMAND_LED           0x47			// Command to set LED
#define COMPAL_EC_COMMAND_LED_NORMAL    0x80			// Set LED as normal
#define COMPAL_EC_COMMAND_LED_SYSTEM    0x81			// Litht on system LED (Green)
#define COMPAL_EC_COMMAND_LED_CHARGE    0x84			// Light on charging LED (Orange)
#define COMPAL_EC_COMMAND_LED_OFF	0x88			// Turn off LEDs (Green and Orange)
#define COMPAL_EC_ADDRESS_LIGHT_H       0xf6			// Command to get light sensor value
#define COMPAL_EC_ADDRESS_LIGHT_L       0x6b			// Command to get light sensor value
#define USB_HOST			1
#define USB_DEVICE			0


#define COMPAL_EC_COMMAND_PM			0x45		// EC power manage command
#define COMPAL_EC_COMMAND_PM_POWEROFF		0xf1		// Turn machine off
#define COMPAL_EC_COMMAND_PM_REBOOT		0xf2		// Reboot machine
#define COMPAL_EC_COMMAND_PM_COLDBOOT		0xf3		// Simulate cold boot

//Peng --- add test command for factory test tool
#define COMPAL_EC_COMMAND_FACTORY_TEST			0x40		// Just for test suspend and power off function
#define COMPAL_EC_COMMAND_AUTO_SUSPEND			0x11		// into auto suspend/resume mode
#define COMPAL_EC_COMMAND_AUTO_SUSPEND_TIME		10			// wait 10 seconds
#define COMPAL_EC_COMMAND_AUTO_COLDBOOT			0x12		// into auto cold boot mode
#define COMPAL_EC_COMMAND_AUTO_COLDBOOT_TIME	60			// wait 60 seconds
#define	COMPAL_EC_COMMAND_SLEEP					0x04		// sleep command --- refer NV EC spec page 77
#define	COMPAL_EC_COMMAND_SLEEP_SUB_POWERDOWN	0x01		// sleep command --- refer NV EC spec page 77

#define RETRY_COUNT 20
int paz00_auto_coldboot_test( NvU8 second );

//henry add get battery manufacturer for test app
static int proc_read_batteryManufacturer(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest EcRequest = {0};
    NvEcResponse EcResponse = {0};
    char Manufacturer[NVEC_MAX_RESPONSE_STRING_SIZE];
   
    int len = 0;

    Manufacturer[0] = '\0';

    /* Fill up request structure */
    EcRequest.PacketType = NvEcPacketType_Request;
    EcRequest.RequestType = NvEcRequestResponseType_Battery;
    EcRequest.RequestSubtype = NvEcBatterySubtype_GetManufacturer;
    EcRequest.NumPayloadBytes = 0;
    EcRequest.Payload[0] = 0;

    /* Request to EC */
    NvStatus = NvEcSendRequest(s_NvEcHandle, &EcRequest, &EcResponse,
                              sizeof(EcRequest), sizeof(EcResponse));
    if (NvSuccess != NvStatus)
    {
       	printk( KERN_ERR "Get Battery Manufacturer,EC request fail\n" );
        goto err1;
    }

    if (EcResponse.Status != NvEcStatus_Success)
    {
       	printk( KERN_ERR "Get Battery Manufacturer,EC response fail\n" );
        goto err1;
    }

    NvOdmOsMemcpy(Manufacturer, EcResponse.Payload,
                    EcResponse.NumPayloadBytes);
    if (EcResponse.NumPayloadBytes<NVEC_MAX_RESPONSE_STRING_SIZE){
		Manufacturer[EcResponse.NumPayloadBytes]='\0';
	}
	  
  err1: 
    len = sprintf(page, "%s",Manufacturer);
    return len;
}

//henry add get battery cell number for test app
static int proc_read_batteryCellNumber(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest EcRequest = {0};
    NvEcResponse EcResponse = {0};
        
    int len = 0;
    int val=0;

    /* Fill up request structure */
    EcRequest.PacketType = NvEcPacketType_Request;
    EcRequest.RequestType = NvEcRequestResponseType_Battery;
    EcRequest.RequestSubtype = 0x1e;
    EcRequest.NumPayloadBytes = 0;
    EcRequest.Payload[0] = 0;

    /* Request to EC */
    NvStatus = NvEcSendRequest(s_NvEcHandle, &EcRequest, &EcResponse,
                              sizeof(EcRequest), sizeof(EcResponse));
    if (NvSuccess != NvStatus)
    {
        printk( KERN_ERR "Get Battery Battery Cell Number,EC request fail\n" );
        goto err1;
    }
    if (EcResponse.Status != NvEcStatus_Success)
    {
       	printk( KERN_ERR "Get Battery Cell Number,EC response fail\n" );
        goto err1;
    }

    //Bit0~5: (0x01)3,(0x02)4,(0x04)6,(0x08)8,(0x10)9,(0x20)12
    if (EcResponse.NumPayloadBytes>0){
        if (EcResponse.Payload[0] & 0x1){
		val=3;
	}
	else if (EcResponse.Payload[0] & 0x1<<1){
		val=4;
	}
        else if (EcResponse.Payload[0] & 0x1<<2){
		val=6;
	}
        else if (EcResponse.Payload[0] & 0x1<<3){
		val=8;
	}
        else if (EcResponse.Payload[0] & 0x1<<4){
		val=9;
	}
        else if (EcResponse.Payload[0] & 0x1<<5){
		val=12;
	}
	else{
	    printk( KERN_ERR "Get Battery Cell Number error!\n" );
	}

    }
    

err1:
    len = sprintf(page, "%i",val);       
    return len;
}

extern void set_elan_wheel_mode(unsigned mode);
extern unsigned get_elan_wheel_mode(void);
static int proc_write_tpwheel(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '0':
         set_elan_wheel_mode(0); //turn off touchpad wheel
         break;
      default:
         set_elan_wheel_mode(1);  //turn on touchpad wheel
   }
   
   return procfs_buffer_size;
}

static int proc_read_tpwheel(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   unsigned status;
   int len = 0;
   status=get_elan_wheel_mode();
   len = sprintf(page, "%s\n", (status)?"touchpad wheel function turn on":"touchpad wheel function turn off" );       
   return len;
}

//*****henry+ support disable/enable battery command ******************************************************
extern void NvEcSetBatteryReqeustFlag(int ival);
extern int NvEcGetBatteryReqeustFlag(void);

static int proc_write_battery(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '1':
         NvEcSetBatteryReqeustFlag(1); //disable
         break;
      default:
         NvEcSetBatteryReqeustFlag(0);  //enable
   }
   
   return procfs_buffer_size;
}

static int proc_read_battery(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   unsigned status;
   int len = 0;
   status=NvEcGetBatteryReqeustFlag();
   len = sprintf(page, "%s\n", (status)?"battery request disable":"battery request enable" );       
   return len;
}
//**************************************************************************************************************

/* Turn machine off */
 int paz00_pm_power_off( void )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

    printk(KERN_INFO "------ %s   coldboot=%d ------\r\n", __FUNCTION__, global_coldboot);
   if( global_coldboot == 1)
   {
        // Robert 2010/04/08 --- send cold boot command to EC
        
        emergency_sync();
        /*
            blocking_notifier_call_chain(&reboot_notifier_list,SYS_POWER_OFF, NULL);
        	system_state = SYS_POWER_OFF;
        	disable_nonboot_cpus();
    	    sysdev_shutdown();
            device_shutdown();
        */printk(KERN_INFO "------ Travis coldboot ------\r\n");
        NvStatus = paz00_auto_coldboot_test( 10 );
        if( NvStatus != NvError_Success )
        {
        	return -1;
        }
   }
   //add by Travis @ 2010/05/25 -- work around for reboot
   else if(global_coldboot == 2)
   {
        printk(KERN_INFO "------ Travis reboot ------\r\n");
        NvStatus = paz00_auto_coldboot_test(10);
        if(NvStatus != NvError_Success)
        {
           return -1;
        }
   }

	//send sleep
   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_Sleep;
   Request.RequestSubtype = ((NvEcRequestResponseSubtype) 
                          NvEcSleepSubtype_GlobalConfigureEventReporting);
   Request.Payload[0] = NVEC_SLEEP_GLOBAL_REPORT_ENABLE_0_ACTION_DISABLE;

   Request.NumPayloadBytes = 1;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
	{
		printk(KERN_INFO "------ send sleep fail ------\r\n");
      return -1;
	}

	printk(KERN_INFO "------ send sleep success ------\r\n");

	//send power down
   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_Sleep;
   Request.RequestSubtype = ((NvEcRequestResponseSubtype) 
                          NvEcSleepSubtype_ApPowerDown);

   Request.NumPayloadBytes = 0;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
	{
		printk(KERN_INFO "------ send power down fail ------\r\n");
      return -1;
	}

	printk(KERN_INFO "------ send power down success ------\r\n");

   return 0;
}

/* Reboot machine */
static void paz00_pm_reset( void )
{
    global_coldboot = 2;
    paz00_pm_power_off();
   /*
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_COMMAND_PM;
   Request.Payload[1] = COMPAL_EC_COMMAND_PM_REBOOT;

   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
      return ;

   return ;
  */
}

/* Convert Hex to int */
static NvU8 my_htoi( char *tmpHex )
{
   NvU8 k = 0;
   
   if ( *tmpHex >= '0' && *tmpHex <= '9' )
      k = ( *tmpHex - '0' ) * 16;
   else if ( *tmpHex >= 'a' && *tmpHex <= 'f' )
      k = ( *tmpHex - 'a' + 10 ) * 16;
   else if ( *tmpHex >= 'A' && *tmpHex <= 'F' )
      k = ( *tmpHex - 'A' + 10 ) * 16;
   tmpHex++;
   if ( *tmpHex >= '0' && *tmpHex <= '9' )
      k += ( *tmpHex - '0' );
   else if ( *tmpHex >= 'a' && *tmpHex <= 'f' )
      k += ( *tmpHex - 'a' + 10 );
   else if ( *tmpHex >= 'A' && *tmpHex <= 'F' )
      k += ( *tmpHex - 'A' + 10 ) ;

   return k;
}

/* Get light sensor value */
static int paz00_light_get_level( NvU8 *light)
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_READ;
   Request.Payload[0] = COMPAL_EC_ADDRESS_LIGHT_H;
   Request.Payload[1] = COMPAL_EC_ADDRESS_LIGHT_L;
   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
      return -1;

   if (Response.Status != NvEcStatus_Success || Response.NumPayloadBytes != 1 )
      return -1;

   *light = Response.Payload[0];

   return 0;
}

/* /proc interface for Read light sensor */
static int proc_read_light(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len;
	NvU8 level = 0;
        paz00_light_get_level( &level );
        len = sprintf(page, "%d\n",
                        level );
        return len;
}

/* send auto suspend/resume command */
int paz00_auto_suspend_test( NvU8 second )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_COMMAND_FACTORY_TEST;
   Request.Payload[1] = COMPAL_EC_COMMAND_AUTO_SUSPEND;
   Request.Payload[2] = second;

   Request.NumPayloadBytes = 3;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
	if (NvStatus != NvError_Success)
	{
	    printk(KERN_INFO "------ send auto suspend/resume command fail ------\r\n");
		return -1;
	}
	else
	{
	    printk(KERN_INFO "------ send auto suspend/resume command success ------\r\n");
		return 0;
	}
}

/* send auto cold boot command */
int paz00_auto_coldboot_test( NvU8 second )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_COMMAND_FACTORY_TEST;
   Request.Payload[1] = COMPAL_EC_COMMAND_AUTO_COLDBOOT;
   Request.Payload[2] = second;

   Request.NumPayloadBytes = 3;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
	if (NvStatus != NvError_Success)
	{
	    printk(KERN_INFO "------ send auto cold boot command fail ------\r\n");
		return -1;
	}
	else
	{
	    printk(KERN_INFO "------ send auto cold boot command success ------\r\n");
		return 0;
	}
}

static int paz00_led_set_led( NvU8 led )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_COMMAND_LED;
   Request.Payload[1] = COMPAL_EC_COMMAND_LED_OFF;

   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
      return -1;

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_COMMAND_LED;

   switch( led )
   {
      case '0':
         Request.Payload[1] = COMPAL_EC_COMMAND_LED_NORMAL;
         break;
      case '1':
         Request.Payload[1] = COMPAL_EC_COMMAND_LED_SYSTEM;
         break;
      case '2':
         Request.Payload[1] = COMPAL_EC_COMMAND_LED_CHARGE;
         break;
      default:
         Request.Payload[1] = COMPAL_EC_COMMAND_LED_NORMAL;
   }

   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
      return -1;
   return 0;
}

static NvBool WriteWolfsonRegister(NvU32 RegIndex, NvU32 Data)
{
	NvU64 guid;
    const NvOdmPeripheralConnectivity *pConnectivity = NULL;
    NvU32 I2cInstance = 0;
    NvU32 i = 0;
    NvU32 DeviceAddr = 0;
    NvU8   Buffer[3] = {0};
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cStatus I2cTransStatus;    
    NvOdmI2cTransactionInfo TransactionInfo;

    guid = NV_ODM_GUID('w','o','l','f','8','9','6','1');

    /* get the connectivity info */
    pConnectivity = NvOdmPeripheralGetGuid( guid );
    if ( !pConnectivity )
         return NV_FALSE;
    
    for (i = 0; i < pConnectivity->NumAddress; i++)
    {
        switch (pConnectivity->AddressList[i].Interface)
        {
            case NvOdmIoModule_I2c:
                DeviceAddr = pConnectivity->AddressList[i].Address;
                I2cInstance = pConnectivity->AddressList[i].Instance;
                break;
            default:
                break;
        }
    }
    printk(KERN_INFO 
      	"Query Success (addr=0x%x)\r\n", DeviceAddr);
     
    hOdmI2c=NvOdmI2cOpen(NvOdmIoModule_I2c, I2cInstance);
    if (!hOdmI2c)
    {
//         DEBUG_VIBRATOR_TRACE(("Audio Codec Power Saving : NvOdmI2cOpen Error \r\n"));
        return NV_FALSE;
    }

    Buffer[0] =  RegIndex & 0xFF;
    Buffer[1] = (Data>>8) & 0xFF;
    Buffer[2] =  Data     & 0xFF;
    
    printk(KERN_INFO 
      	"Transation buffer[1] = 0x%x\r\n", Buffer[1]);
    printk(KERN_INFO 
      	"Transation buffer[2] = 0x%x\r\n", Buffer[2]);
    
    TransactionInfo.Address = DeviceAddr;
    TransactionInfo.Buf = (NvU8*)Buffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 3;

    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1, 100, 1000);

    // HW- BUG!! If timeout, again retransmit the data.                    
    if (I2cTransStatus == NvOdmI2cStatus_Timeout)
        I2cTransStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1, 100, 1000);

    if (I2cTransStatus)
        printk(KERN_INFO "\t --- Failed(0x%08x)\n", I2cTransStatus);
    else 
        printk(KERN_INFO "Write 0x%02x = 0x%04x\n", RegIndex, Data);

	NvOdmI2cClose(hOdmI2c);

    return (NvBool)(I2cTransStatus == 0);
}

/* HDMI analog switch -- register control interface */
static NvBool WritePericomRegister(char *ParsedCmd, int CmdLen)
{
    NvU64 guid;
    const NvOdmPeripheralConnectivity *pConnectivity = NULL;
    NvU32 I2cInstance = 0;
    NvU32 i = 0;
    NvU32 DeviceAddr = 0;
    NvU8   Buffer[2] = {0};
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cStatus I2cTransStatus;
    NvOdmI2cTransactionInfo TransactionInfo;

    guid = NV_ODM_GUID('p','i','3','-','h','d','m','i');

    /* get the connectivity info */
    pConnectivity = NvOdmPeripheralGetGuid( guid );
    if ( !pConnectivity )
         return NV_FALSE;

    for (i = 0; i < pConnectivity->NumAddress; i++)
    {
        switch (pConnectivity->AddressList[i].Interface)
        {
            case NvOdmIoModule_I2c:
                DeviceAddr = pConnectivity->AddressList[i].Address;
                I2cInstance = pConnectivity->AddressList[i].Instance;
                break;
            default:
                break;
        }
    }
    printk(KERN_INFO
        "Query Success (addr=0x%x)\r\n", DeviceAddr);

    hOdmI2c=NvOdmI2cOpen(NvOdmIoModule_I2c, I2cInstance);
    if (!hOdmI2c)
    {
        printk(KERN_INFO "Hdmi Switch: NvOdmI2cOpen Error \r\n");
        return NV_FALSE;
    }

    Buffer[0] =  ParsedCmd[0];
    Buffer[1] =  ParsedCmd[1];
    TransactionInfo.Address = DeviceAddr;
    TransactionInfo.Buf = (NvU8*)Buffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1, 100, 1000);

    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == NvOdmI2cStatus_Timeout)
    {
        printk(KERN_INFO "Hdmi Switch: If timeout, again retransmit the data.\r\n");
        I2cTransStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1, 100, 1000);
    }

    if (I2cTransStatus)
        printk(KERN_INFO "\t --- Failed(0x%08x)\n", I2cTransStatus);
    else
        printk(KERN_INFO "Hdmi Switch : Write 0x%02x, 0x%02x to switch hdmi\n", Buffer[0], Buffer[1]);

    NvOdmI2cClose(hOdmI2c);

    return (NvBool)(I2cTransStatus == 0);
}

static NvBool ReadPericomRegister(NvU8 *Data1, NvU8 *Data2)
{
    NvU64 guid;
    const NvOdmPeripheralConnectivity *pConnectivity = NULL;
    NvU32 I2cInstance = 0;
    NvU32 i = 0;
    NvU32 DeviceAddr = 0;
    NvU8   Buffer[2] = {0};
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cStatus I2cTransStatus;
	NvOdmI2cTransactionInfo TransactionInfo;
    guid = NV_ODM_GUID('p','i','3','-','h','d','m','i');

    /* get the connectivity info */
    pConnectivity = NvOdmPeripheralGetGuid( guid );
    if ( !pConnectivity )
         return NV_FALSE;

    for (i = 0; i < pConnectivity->NumAddress; i++)
    {
        switch (pConnectivity->AddressList[i].Interface)
        {
            case NvOdmIoModule_I2c:
                DeviceAddr = pConnectivity->AddressList[i].Address;
                I2cInstance = pConnectivity->AddressList[i].Instance;
                break;
            default:
                break;
        }
    }
    printk(KERN_INFO
        "Query Success (addr=0x%x)\r\n", (DeviceAddr| 0x1));

    hOdmI2c=NvOdmI2cOpen(NvOdmIoModule_I2c, I2cInstance);
    if (!hOdmI2c)
    {
        printk(KERN_INFO "Hdmi Switch: NvOdmI2cOpen Error \r\n");
        return NV_FALSE;
    }
      
    TransactionInfo.Address = (DeviceAddr | 0x1);
    TransactionInfo.Buf = Buffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 2;

    // Read data from PMU at the specified offset
    I2cTransStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1, 100, 1000);

    *Data1 = Buffer[0];
    *Data2 = Buffer[1];
    if (I2cTransStatus == 0) 
        printk(KERN_INFO "Hdmi Switch : Read byte1 is 0x%02x, byte2 is 0x%02x\n", Buffer[0], Buffer[1]);
    else 
        printk(KERN_INFO "Read Error: %08x\n", I2cTransStatus);
    
    NvOdmI2cClose(hOdmI2c);
    return (NvBool)(I2cTransStatus == 0);
}

static int paz00_set_lback( int enable )
{
	if(enable)
	{
		WriteWolfsonRegister(R25_REG, 254);
		WriteWolfsonRegister(R0_REG, 287);
		WriteWolfsonRegister(R1_REG, 287);
		WriteWolfsonRegister(R2_REG, 0x7f);
		WriteWolfsonRegister(R3_REG, 0x17f);
		WriteWolfsonRegister(R9_REG, 1);
	}
	else
	{
		WriteWolfsonRegister(R2_REG, 0x65);
		WriteWolfsonRegister(R3_REG, 0x165);
		WriteWolfsonRegister(R9_REG, 0);
	}
	return 0;
}

static int proc_write_lback(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '1':
         paz00_set_lback(1);
         break;
      default:
         paz00_set_lback(0);
   }
   
   return procfs_buffer_size;
}

static int proc_write_led(struct file *file, const char *buffer, unsigned long count, void *data)
{
        int len;
        NvU8 command;

        if(copy_from_user(&command, buffer, 1)) {
                return -EFAULT;
        }
        paz00_led_set_led( command ); 
        return len;
}


#if 1	//Robert 2010/04/12
extern void set_wlan_led(unsigned mode);

static int proc_write_wifi3g(struct file *file, const char *buffer, unsigned long count, void *data)
{
	NvU8 command;

	if(copy_from_user(&command, buffer, 1))
	{
		return -EFAULT;
	}

	if( command == '1' )
	{
		set_wlan_led( 1 );
	}
	else
	{
		set_wlan_led( 0 );
	}

	return 0;
}
#endif

int gTestSuspendFlag = 0;

static int proc_write_suspend(struct file *file, const char *buffer, unsigned long count, void *data)
{
	NvU8 command;

	if(copy_from_user(&command, buffer, 1))
	{
		return -EFAULT;
	}

	if( command == '1' )
	{
		gTestSuspendFlag = 1;
	}
	else
	{
		gTestSuspendFlag = 0;
	}

	return 0;
}

static NvU8 paz00_diag_write_ec_uuid_offset( NvU8 Uuid_Offset, NvU8 Uuid_Value )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_ADDRESS_WRITE_UUID;
   Request.Payload[1] = Uuid_Offset;
   Request.Payload[2] = Uuid_Value;
   Request.NumPayloadBytes = 3;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
/*
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
*/
   return 0;
}

static NvU8 paz00_diag_get_ec_uuid_offset( NvU8 Uuid_Offset, NvU8 *Uuid_Value )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_ADDRESS_READ_UUID;
   Request.Payload[1] = Uuid_Offset;
   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
   *Uuid_Value = Response.Payload[0];
   return( Response.Payload[0] );

}

static int paz00_diag_write_ec_uuid( NvU8 *strUuid )
{
   NvU8 i=0;

   for ( i = 0; i < 16; i++ )
   {
      paz00_diag_write_ec_uuid_offset( i, strUuid[i] );
   }
   return i;
}

static int paz00_diag_get_ec_uuid( NvU8 *strUuid )
{
   NvU8 i=0;

   for ( i = 0; i < 16; i++ )
   {
      paz00_diag_get_ec_uuid_offset( i, strUuid+i );
   }
   return 0;
}

extern int paz00_set_lcd_output(int mode);
extern int paz00_set_hdmi_output(int mode);

unsigned char gHdmiConnect = 'N';

static int proc_read_hdmiswitch(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;

	len = sprintf(page, "%c\n", gHdmiConnect);
	
	return len;
}

/**
*	Modify history:
*	2010/12/30; lance; for coverity check err 
*/
static int proc_write_hdmiswitch(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char *ReqCmd, *key, *ParsedCmd;
   int PosCmd = 0;
   int ErrFlg = 0;	//lance 

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE )
      procfs_buffer_size = PROCFS_MAX_SIZE;

   ReqCmd = kmalloc( procfs_buffer_size, GFP_KERNEL );
   ParsedCmd = kmalloc( procfs_buffer_size/2, GFP_KERNEL );
   
   if(copy_from_user(ReqCmd, buffer, procfs_buffer_size)) {
	  //lance
	  ErrFlg = 1;
	  goto cleanup;
   }
   
#if 1	//Robert 2010/04/11
	if( ReqCmd[0] == '1' )
	{
		// enable LCD and disable HDMI
		paz00_set_lcd_output(1);
		paz00_set_hdmi_output(0);
		paz00_set_hdmi_output(0);

		printk("------ LCD HDMI setting=%c ------\n", ReqCmd[0]);
	}
	else if( ReqCmd[0] == '2' )
	{
		// enable HDMI and disable LCD
		
		paz00_set_lcd_output(0);
		paz00_set_hdmi_output(1);

		printk("------ LCD HDMI setting=%c ------\n", ReqCmd[0]);
	}
	else if( ReqCmd[0] == 'Y' )
	{
		//HDMI connect
		gHdmiConnect = 'Y';
		printk("------ HDMI connect ------\n");
	}
	else if( ReqCmd[0] == 'N' )
	{
		//HDMI disconnect
		gHdmiConnect = 'N';
		paz00_set_lcd_output(1);
		paz00_set_hdmi_output(0);
		printk("------ HDMI disconnect ------\n");
	}
	else if( ReqCmd[0] == '0' )
	{
		// enable LCD and HDMI
		paz00_set_lcd_output(1);
		paz00_set_hdmi_output(1);

		printk("------ LCD HDMI setting=%c ------\n", ReqCmd[0]);
	}
	else
	{
		// enable LCD and disable HDMI
		paz00_set_lcd_output(1);
		paz00_set_hdmi_output(0);

		printk("------ LCD HDMI setting=%c ------\n", ReqCmd[0]);
	}
#else
   *(ReqCmd+count-1) = 0;
   while ((key = strsep(&ReqCmd, " :,-"))) {
      if (!*key)
         continue;

      ParsedCmd[PosCmd] = my_htoi( key );
      PosCmd++;
   }
   WritePericomRegister( ParsedCmd, PosCmd );
#endif

//lance: kmalloc resource free
cleanup:
   if (ReqCmd)
   {
		//printk(KERN_DEBUG "ReqCmd release\n");
		kfree(ReqCmd);
   }
   if (ParsedCmd)
   {
   		//printk(KERN_DEBUG "ParsedCmd release\n");
		kfree(ParsedCmd);
   }
   
   if (1 == ErrFlg)   
   		return -EFAULT;
   else
   		return procfs_buffer_size;
}
static int paz00_diag_set_light_sensor( int enable )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_SET_DEVICE;
   Request.Payload[1] = enable?COMPAL_EC_ENABLE_LIGHT_SENSOR:COMPAL_EC_DISABLE_LIGHT_SENSOR;
   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
   return 0;
}

static int paz00_diag_set_button( int status )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_SET_DEVICE;
   Request.Payload[1] = status?0xe0:0xe1;
   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
   return 0;
}

static int paz00_diag_set_usb( int host )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_SET_DEVICE;
   Request.Payload[1] = host?COMPAL_EC_SET_USB_HOST:COMPAL_EC_SET_USB_DEVICE;
   Request.NumPayloadBytes = 2;

   printk( KERN_INFO "Switch USB to %s\n", host?"host":"device" );
   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
   return 0;
}

static int paz00_diag_get_ec_external_device( NvU8 *status )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;
   Request.RequestSubtype = COMPAL_EC_COMMAND;
   Request.Payload[0] = COMPAL_EC_SET_DEVICE;
   Request.Payload[1] = COMPAL_EC_GET_DEVICE_STATUS;
   Request.NumPayloadBytes = 2;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "UUID NvEcSendRequest() Failed\n" );
      return -1;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "Response Failed\n" );
      return -1;
   }
   *status = Response.Payload[0];

   return 0;
}

static int paz00_diag_get_ec_version( char *strVersion )
{
   NvU8 unused = 0, major = 0, minor = 0, test = 0, project_num = 0;
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};
   NvEcControlGetFirmwareVersionResponsePayload EcVersion;

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_Control;
   Request.RequestSubtype = NvEcControlSubtype_GetFirmwareVersion;
   Request.Payload[0] = 0;
   Request.NumPayloadBytes = 0;

   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
      return -1;

   if (Response.Status != NvEcStatus_Success )
      return -1;

   NvOsMemcpy(&EcVersion, Response.Payload, Response.NumPayloadBytes);

    major = EcVersion.VersionMinor[0];
    minor = EcVersion.VersionMinor[1];
    test =  EcVersion.VersionMajor[0];
    project_num = EcVersion.VersionMajor[1];
   
   if ( test == 0 )
      sprintf( strVersion, "%02X.%02X.00, Project number: %X", major, minor, project_num);
   else
      sprintf( strVersion, "%02X.%02XT%02X, Project number: %X", major, minor, test, project_num);
   return 0;
}

static int proc_read_lsensor(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   NvU8 status = 0;
   int len = 0;
       
   if ( off > 0 )
      return 0;

   paz00_diag_get_ec_external_device( &status );

   len = sprintf(page, "%d\n", status&0x04?1:0 );
       
   return len;
}

static int proc_read_usb(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   NvU8 status = 0;
   int len = 0;
       
   if ( off > 0 )
      return 0;

   paz00_diag_get_ec_external_device( &status );
   len = sprintf(page, "%s\n", (status&0x02)?"device":"host" );
       
   return len;
}

static int proc_read_version(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len;
	if ( off > 0 )
		return 0;

        len = sprintf(page, "%s\n", strVersion );
       
        return len;
}

static int proc_read_uuid(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   NvU8 strUuid[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
       
   if ( off > 0 )
      return 0;

   paz00_diag_get_ec_uuid( strUuid );

   len = sprintf(page, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
           strUuid[0], strUuid[1], strUuid[2], strUuid[3], strUuid[4], strUuid[5], strUuid[6], strUuid[7],
           strUuid[8], strUuid[9], strUuid[10], strUuid[11], strUuid[12], strUuid[13], strUuid[14], strUuid[15] );
       
   return len;
}
//I add code in this read_gpio
static int proc_read_sd(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle hGpioPin;
    NvU32 PinStateValue = 0;
    int len;
    
    hGpio = NvOdmGpioOpen();
    if(!hGpio)
       return 0;
       
	hGpioPin = NvOdmGpioAcquirePinHandle (hGpio , 
							'h'-'a', 
							 1);
	if(!hGpioPin)
	   return 0;
	NvOdmGpioConfig(hGpio,
                    hGpioPin,
                    NvOdmGpioPinMode_InputInterruptAny);
	

	NvOdmGpioGetState(hGpio, hGpioPin, &PinStateValue);
	
	if(hGpio){
		if(hGpioPin)
		NvOdmGpioReleasePinHandle(hGpio, hGpioPin);
		NvOdmGpioClose(hGpio);
	}
	
	
	len = sprintf(page, "%d", PinStateValue);
	
	return len;
}

static int proc_write_button(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '1':
         paz00_diag_set_button(1);
         break;
      default:
         paz00_diag_set_button(0);
   }
   
   return procfs_buffer_size;
}

static int proc_write_coldboot(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE )
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

	printk(KERN_INFO "------ %s   status=%c ------\r\n", __FUNCTION__, status);

   switch( status )
   {
      case '1':
         global_coldboot = 1;
			// Robert 2010/04/08
		paz00_pm_power_off();
         break;
      case '3':
         global_coldboot = 3;
         break;
      default:
         global_coldboot = 0;
   }

   return procfs_buffer_size;
}

static int proc_write_lsensor(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '1':
         paz00_diag_set_light_sensor(1);
         break;
      default:
         paz00_diag_set_light_sensor(0);
   }
   
   return procfs_buffer_size;
}

static int proc_write_usb(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char status;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(&status, buffer, 1)) {
      return -EFAULT;
   }

   switch( status )
   {
      case '0':
         paz00_diag_set_usb(USB_DEVICE);
         break;
      default:
         paz00_diag_set_usb(USB_HOST);
   }
   
   return procfs_buffer_size;
}

static int proc_write_uuid(struct file *file, const char *buffer, unsigned long count, void *data)
{
   int len = 0, i = 0;
   char ReqNewUuid[PROCFS_MAX_SIZE];
   NvU8 strNewUuid[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
   char tmpHex[3] = { 0,0,0 };
   unsigned long procfs_buffer_size = 0;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if(copy_from_user(ReqNewUuid, buffer, procfs_buffer_size)) {
      return -EFAULT;
   }
   if ( count != 33 )
      return -EFAULT;
   
   ReqNewUuid[32] = 0;
   for( i = 0; i < 32; i++ )
   {
      tmpHex[0] = ReqNewUuid[i++];
      tmpHex[1] = ReqNewUuid[i];
      strNewUuid[len] = my_htoi( tmpHex );
      len++;
   }
    
   paz00_diag_write_ec_uuid( strNewUuid );

   
   return procfs_buffer_size;
}

static int proc_read_ecctl(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int i=0, len=0;
   char *response;
   char tmpbuf[4] = { 0,0,0,0 };

   if ( off > 0 )
      return 0;

   if ( NumEcResponse == 0 )
      return 0;

   response = kmalloc( NumEcResponse * 3, GFP_KERNEL );
   *response = 0;

   for ( i = 0; i < NumEcResponse; i++ )
   {
      sprintf( tmpbuf, "%02x ", EcCtlResponse[i] );
      strcat( response, tmpbuf );
      len += 3;
   }
   response[len-1] = 0;
   len = sprintf(page, "%s\n", response );

   kfree( response );
   for ( i = 0; i < NumEcResponse; i++ )
   {
      EcCtlResponse[i] = 0;
   }
   NumEcResponse = 0;
   return len;
}
static int paz00_diag_ec_cmd_req( char *ParsedCmd, int CmdLen )
{
   NvError NvStatus = NvError_Success;
   NvEcRequest Request = {0};
   NvEcResponse Response = {0};
   int i=0;
   int err=-1;

   Request.PacketType = NvEcPacketType_Request;
   Request.RequestType = NvEcRequestResponseType_OEM0;

   if ( CmdLen < 1 )
      return -1;
   Request.RequestSubtype = ParsedCmd[0];

   for ( i = 1; i < CmdLen; i++ )
   {
      Request.Payload[i-1] = ParsedCmd[i];
   }
   Request.NumPayloadBytes = CmdLen-1;

//Henry2010.7.30 Add retry when request fail
for(i=1;i<=RETRY_COUNT;i++){
   NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
   if (NvStatus != NvError_Success)
   {
      printk( KERN_INFO "NvEcSendRequest Req failed\n,try %ith\n",i );
      NvOsWaitUS(50000);
      continue;
   }
   if (Response.Status != NvEcStatus_Success )
   {
      printk( KERN_INFO "NvEcSendRequest Resp failed\n,try %ith\n",i );
      NvOsWaitUS(50000);
      continue;
   }
   err=0;
   break;
}
    if(err!=0)
	return err;

   for( i = 0; i < Response.NumPayloadBytes; i++ )
      EcCtlResponse[i] = Response.Payload[i];

   NumEcResponse = Response.NumPayloadBytes;

   printk( KERN_INFO "Get %d bytes responsed from EC\n", Response.NumPayloadBytes );
   return 0;

}

static int proc_write_ecctl(struct file *file, const char *buffer, unsigned long count, void *data)
{
   unsigned long procfs_buffer_size = 0;
   char *ReqCmd, *key, *ParsedCmd;
   int PosCmd = 0;

   procfs_buffer_size = count;
   if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
      procfs_buffer_size = PROCFS_MAX_SIZE;

   ReqCmd = kmalloc( procfs_buffer_size, GFP_KERNEL );
   ParsedCmd = kmalloc( procfs_buffer_size/2, GFP_KERNEL );
   if(copy_from_user(ReqCmd, buffer, procfs_buffer_size)) {
      return -EFAULT;
   }
   *(ReqCmd+count-1) = 0;
   while ((key = strsep(&ReqCmd, " :,-"))) {
      if (!*key)
         continue;
    
      ParsedCmd[PosCmd] = my_htoi( key );
      PosCmd++;
   }
   paz00_diag_ec_cmd_req( ParsedCmd, PosCmd );

   kfree( ReqCmd );  
   kfree( ParsedCmd );  
   return procfs_buffer_size;
}

// EDID function
#define LCD_I2C_SPEED_KHZ   40

#define LCD_EDID_LG_LP101WSA            0x30E4B901
#define LCD_EDID_LG_LP101WSB            0x30E46E02
#define LCD_EDID_SAMSUNG_LTN101NT02     0x4CA34E54
#define LCD_EDID_SAMSUNG_LTN101NT05     0x4CA34E35
#define LCD_EDID_LG_LP101WH2            0x30E4B402


NvBool
EDID_I2cRead8(NvOdmServicesI2cHandle hI2c, NvU8 Addr, NvU8 *Data)
{
    NvU8 ReadBuffer =0;
    NvOdmI2cStatus status = NvOdmI2cStatus_Success;
    NvOdmI2cTransactionInfo TransactionInfo[2];

    // Write the PMU offset
    ReadBuffer = Addr & 0xFF;

    //NvOdmOsDebugPrintf("PDBG::ReadBuffer[0]:%d\n", ReadBuffer);   //PDBG
    TransactionInfo[0].Address = 0xA0;
    TransactionInfo[0].Buf = &ReadBuffer;
    TransactionInfo[0].Flags = 
            NVODM_I2C_IS_WRITE | NVODM_I2C_USE_REPEATED_START;;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = (0xA0 | 0x1);
    TransactionInfo[1].Buf = &ReadBuffer;
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 1;

    // Read data from PMU at the specified offset
    status = NvOdmI2cTransaction(hI2c, &TransactionInfo[0], 2,
                                 LCD_I2C_SPEED_KHZ, NV_WAIT_INFINITE);
    //FastbootStatus("PDBG::Status %d\r\n", status);
    if(status == 0) 
    {
	 *Data = ReadBuffer;
     return NV_TRUE;
    }
    return NV_FALSE;
}

unsigned short int gCheckSum = 0;
unsigned char gPanelID[16] = {0};

static NvU32
GetDeviceEDID()
{
    NvU8 i;
    NvU8 data;
    NvU32 VendorID = 0;
    NvOdmServicesI2cHandle hEdidI2c;
     
	if (! (hEdidI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x0)))
	{
		return 0xFFFFFFFF;
	}

	// get EDID
	for (i=0; i<4; i++)
	{
		if (EDID_I2cRead8(hEdidI2c, i+8, &data))
		{
			VendorID += (((NvU32)data) << ((4-i-1)*8));
		}
		else
		{
		    NvOdmI2cClose(hEdidI2c);
			return 0xFFFFFFFF;
		}
    }

	// get Panel ID
	for (i=0; i<13; i++)
	{
		if (EDID_I2cRead8(hEdidI2c, i+0x71, &data))
		{
			gPanelID[i] = data;
		}
		else
		{
		    NvOdmI2cClose(hEdidI2c);
			return 0xFFFFFFFF;
		}
    }

	// get check sum
	for (i=0; i<2; i++)
	{
		if (EDID_I2cRead8(hEdidI2c, i+0x7E, &data))
		{
			gCheckSum = (gCheckSum << 8) + data;
		}
		else
		{
		    NvOdmI2cClose(hEdidI2c);
			return 0xFFFFFFFF;
		}
    }

    NvOdmI2cClose(hEdidI2c);
    return VendorID;
} 

static int proc_read_edid(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   NvU32 EdidValue = 0;
   
   EdidValue = GetDeviceEDID();
   if(EdidValue < 0)
     return 0;
   len = sprintf(page, "%08x\n%s\n%04x\n", EdidValue, gPanelID, gCheckSum);
       
   return len;
}

static int proc_write_edid(struct file *file, const char *buffer, unsigned long count, void *data)
{
   return 0;
}

static int enable_ec_timer(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    int i=0;

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0xF4;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
      printk( KERN_INFO "NvEcSendRequest failed\n" );
      return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
      printk( KERN_INFO "Response failed\n" );
      return -1;
    }       
}

static int disable_ec_timer(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    int i=0;

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0xF5;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
      printk( KERN_INFO "NvEcSendRequest failed\n" );
      return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
      printk( KERN_INFO "Response failed\n" );
      return -1;
    }       
}

static int proc_write_ec_timer(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned long procfs_buffer_size = 0;
    char *write_buf;

    procfs_buffer_size = count;
    if (procfs_buffer_size > PROCFS_MAX_SIZE ) 
        procfs_buffer_size = PROCFS_MAX_SIZE;

    write_buf = kmalloc( procfs_buffer_size, GFP_KERNEL );

    if(copy_from_user(write_buf, buffer, 1)) {
        return -EFAULT;
    }

    if (write_buf[0] == '1') {
        enable_ec_timer();
    } else if (write_buf[0] == '0') {
        disable_ec_timer();
    }

    kfree( write_buf );  

    return procfs_buffer_size;
}

static int proc_read_ec_timer(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    int time_low_byte = 0;
    int time_high_byte = 0;
    int battey_life_time = 0;

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    int i=0;

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x4c;
    Request.Payload[1] = 0xa0;
    Request.Payload[2] = 0xe0;
    Request.Payload[3] = 0x07;
    Request.NumPayloadBytes = 4;

    NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
      printk( KERN_INFO "NvEcSendRequest failed\n" );
      return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
      printk( KERN_INFO "Response failed\n" );
      return -1;
    }

    NumEcResponse = Response.NumPayloadBytes;

    printk( KERN_INFO "Get %d high bytes responsed from EC: %x\n", Response.NumPayloadBytes, Response.Payload[0]);

    time_high_byte = Response.Payload[0] << 8;

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x4c;
    Request.Payload[1] = 0xa0;
    Request.Payload[2] = 0xe1;
    Request.Payload[3] = 0x07;
    Request.NumPayloadBytes = 4;

    NvStatus = NvEcSendRequest(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
      printk( KERN_INFO "NvEcSendRequest failed\n" );
      return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
      printk( KERN_INFO "Response failed\n" );
      return -1;
    }

    NumEcResponse = Response.NumPayloadBytes;

    printk( KERN_INFO "Get %d low bytes responsed from EC: %x\n", Response.NumPayloadBytes, Response.Payload[0]);

    time_low_byte = Response.Payload[0];

    battey_life_time = time_high_byte + time_low_byte;
    
    len = sprintf(page, "%d\n", battey_life_time);

    return len;
}

static int __devinit paz00_diag_probe(struct platform_device *device)
{
   return 0;
}

static int paz00_diag_remove(struct platform_device *device)
{
   return 0;
}

struct platform_driver paz00_diag_drv = {
	.driver = {
		.name = "test_program",
		.owner = THIS_MODULE,
	},
	.probe = paz00_diag_probe,
	.remove = paz00_diag_remove,
};


static int paz00_diag_register_platform(void)
{
   int err = 0;

   err = platform_driver_register(&paz00_diag_drv);
   if (err)
   {
      printk( KERN_ERR "PAZ00 Diagnostic: platfrom_driver_register() fail\n" );
      return err;
   }

   paz00_diag_dev = platform_device_alloc("test_program", -1);
   platform_device_add(paz00_diag_dev);

   return 0;
}

static void paz00_diag_unregister_platform(void)
{
   if (!paz00_diag_dev)
      return;

   platform_device_del(paz00_diag_dev);
   platform_driver_unregister(&paz00_diag_drv);
}

static int paz00_diag_ec_init(void)
{
   NvError NvStatus = NvError_Success;

   NvStatus =  NvEcOpen(&s_NvEcHandle, 0 /* instance */);
   if (NvStatus != NvError_Success)
   {
      printk( KERN_ERR "PAZ00 Diagnostic: NvEcOpen() failed\n" );
       return -ENODEV;
   }

   return 0;

}

static int __init paz00_diag_init(void)
{
pr_info("Henry>>%s Begin===\n",__FUNCTION__);
   if ( paz00_diag_ec_init() )
	{
		printk("------ ec init error ------\n");
      goto err_ec;
	}

   ec_dir = proc_mkdir(DIR_NAME, NULL);
   if(ec_dir == NULL) {
      goto err_procdir;
   }
   ec_ver_file = create_proc_entry("ec_version", 0444, ec_dir);
   if(ec_ver_file == NULL) {
      goto err_procfile;
   }
   ec_ver_file->read_proc = proc_read_version;
   uuid_file = create_proc_entry("uuid", 0666, ec_dir);
   if(uuid_file == NULL) {
      goto err_uuidfile;
   }
   uuid_file->read_proc = proc_read_uuid;
   uuid_file->write_proc = proc_write_uuid;
   
   //I add code in it 
   gpio_file = create_proc_entry("SD-test", 0666, ec_dir);
   if(gpio_file == NULL)
   {
	  // goto err_gpio_file;
	}
	
    
	gpio_file->read_proc = proc_read_sd;
//////
   usb_file = create_proc_entry("usb", 0666, ec_dir);
   if(usb_file == NULL) {
      goto err_usbfile;
   }
   usb_file->read_proc = proc_read_usb;
   usb_file->write_proc = proc_write_usb;

   lsensor_file = create_proc_entry("lsensor", 0666, ec_dir);
   if(lsensor_file == NULL) {
      goto err_lsensorfile;
   }
   lsensor_file->read_proc = proc_read_lsensor;
   lsensor_file->write_proc = proc_write_lsensor;

   button_file = create_proc_entry("disable_button", 0666, ec_dir);
   if(lsensor_file == NULL) {
      goto err_buttonfile;
   }
//   button_file->read_proc = proc_read_button;
   button_file->write_proc = proc_write_button;

   led_file = create_proc_entry("led", 0222, ec_dir);
   if(led_file == NULL) {
      goto err_ledfile;
   }
   led_file->write_proc = proc_write_led;

   light_file = create_proc_entry("light", 0444, ec_dir);
   if(light_file == NULL) {
      goto err_lightfile;
   }
   light_file->read_proc = proc_read_light;
   
   loopback_file = create_proc_entry("loopback", 0222, ec_dir);
   if(loopback_file == NULL) {
      goto err_lbackfile;
   }
   loopback_file->write_proc = proc_write_lback;

   ecctl_file = create_proc_entry("ecctl", 0666, ec_dir);
   if(loopback_file == NULL) {
      goto err_ecctlfile;
   }
   ecctl_file->write_proc = proc_write_ecctl;
   ecctl_file->read_proc = proc_read_ecctl;

   coldboot_file = create_proc_entry("coldboot", 0222, ec_dir);
   if(coldboot_file == NULL) {
      goto err_coldboot;
   }
   coldboot_file->write_proc = proc_write_coldboot;

   wifi3g_file = create_proc_entry("wifi3g", 0666, ec_dir);
   if(wifi3g_file == NULL) {
      goto err_wifi3gfile;
   }
   wifi3g_file->write_proc = proc_write_wifi3g;

   hdmi_file = create_proc_entry("hdmi", 0666, ec_dir);
   if(hdmi_file == NULL) {
      goto err_hdmifile;
   }
   hdmi_file->read_proc = proc_read_hdmiswitch;
   hdmi_file->write_proc = proc_write_hdmiswitch;

   suspend_file = create_proc_entry("suspend", 0222, ec_dir);
   if(suspend_file == NULL) {
      goto err_suspend;
   }
   suspend_file->write_proc = proc_write_suspend;

   pm_power_off = paz00_pm_power_off;
   arm_pm_restart = paz00_pm_reset;

   paz00_diag_get_ec_version( strVersion );

#ifdef CONFIG_PAZ00_USB_CLIENT 
   paz00_diag_set_usb(USB_DEVICE);
#endif

   printk(KERN_INFO "%s %s initialised, current version = %s\n", MODULE_NAME, DRV_VER, strVersion );

   lcd_edid_file = create_proc_entry("edid", 0666, ec_dir);
   if(lcd_edid_file == NULL) {
      goto err_edidfile;
   }
   lcd_edid_file->read_proc  = proc_read_edid;
   lcd_edid_file->write_proc = proc_write_edid;


   tpwheel_file = create_proc_entry("tpwheel", 0666, ec_dir);
   if(tpwheel_file == NULL) {
      goto err_tpwheelfile;
   }
   tpwheel_file->read_proc = proc_read_tpwheel;
   tpwheel_file->write_proc = proc_write_tpwheel;


//henry+ support disable/enable battery command 
   battery_file = create_proc_entry("battery", 0666, ec_dir);
   if(battery_file == NULL) {
      goto err_batteryfile;
   }
   battery_file->read_proc = proc_read_battery;
   battery_file->write_proc = proc_write_battery;

//henry add  get battery manufacturer for test app
   batteryManufacturer_file = create_proc_entry("batteryManufacturer", 0666, ec_dir);
   if(batteryManufacturer_file==NULL){
	goto err_batteryManufacturerfile;	
   }
   batteryManufacturer_file->read_proc = proc_read_batteryManufacturer;

//henry add  get battery cell number for test app
   batteryCellNumber_file = create_proc_entry("batteryCellNumber", 0666, ec_dir);
   if(batteryCellNumber_file==NULL){
	goto err_batteryCellNumberfile;	
   }
   batteryCellNumber_file->read_proc = proc_read_batteryCellNumber;




    ec_timer_file = create_proc_entry("ec_timer", 0666, ec_dir);
	if(ec_timer_file == NULL) {
	goto err_ec_timerfile;
     }
    ec_timer_file->write_proc  = proc_write_ec_timer;
    ec_timer_file->read_proc = proc_read_ec_timer;
pr_info("Henry>>%s End===\n",__FUNCTION__);
   return 0;


//henry modify
err_ec_timerfile:
    remove_proc_entry("batteryCellNumber", ec_dir);
err_batteryCellNumberfile:
    remove_proc_entry("batteryManufacturer", ec_dir);
err_batteryManufacturerfile:
    remove_proc_entry("battery", ec_dir);
err_batteryfile:
	remove_proc_entry("tpwheel", ec_dir);
err_tpwheelfile:
	remove_proc_entry("edid", ec_dir);
err_edidfile:
    remove_proc_entry("suspend", ec_dir);
err_suspend:
   remove_proc_entry("hdmi", ec_dir);
err_hdmifile:
   remove_proc_entry("wifi3g", ec_dir);
err_wifi3gfile:
   remove_proc_entry("coldboot", ec_dir);
err_coldboot:
   remove_proc_entry("ecctl", ec_dir);
err_ecctlfile:
   remove_proc_entry("loopback", ec_dir);
err_lbackfile:
   remove_proc_entry("light", ec_dir);
err_lightfile:
   remove_proc_entry("led", ec_dir);
err_ledfile:
   remove_proc_entry("disable_button", ec_dir);
err_buttonfile:
   remove_proc_entry("lsensor", ec_dir);
err_lsensorfile:
   remove_proc_entry("usb", ec_dir);
err_usbfile:
   remove_proc_entry("uuid", ec_dir);
err_uuidfile:
   remove_proc_entry("ec_version", ec_dir);

err_procfile:
   remove_proc_entry(DIR_NAME, NULL);
err_procdir:
   NvEcClose(s_NvEcHandle);
err_ec:
   return -ENODEV;
}

static void __exit paz00_diag_exit(void)
{
   remove_proc_entry("ecctl", ec_dir);
   remove_proc_entry("loopback", ec_dir);
   remove_proc_entry("light", ec_dir);
   remove_proc_entry("led", ec_dir);
   remove_proc_entry("disable_button", ec_dir);
   remove_proc_entry("lsensor", ec_dir);
   remove_proc_entry("usb", ec_dir);
   remove_proc_entry("uuid", ec_dir);
   remove_proc_entry("ec_version", ec_dir);

   //henry+
   remove_proc_entry("tpwheel", ec_dir);
   remove_proc_entry("battery", ec_dir);

   remove_proc_entry("batteryManufacturer", ec_dir);
   remove_proc_entry("batteryCellNumber", ec_dir);

   remove_proc_entry("hdmi", ec_dir);
   remove_proc_entry("wifi3g", ec_dir);
   remove_proc_entry("suspend", ec_dir);

   remove_proc_entry(DIR_NAME, NULL);
   printk(KERN_INFO "%s %s removed\n", MODULE_NAME, DRV_VER);
   NvEcClose(s_NvEcHandle);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Genie Wang");
MODULE_DESCRIPTION("PAZ00 Diagnostic Driver");

module_init(paz00_diag_init);
module_exit(paz00_diag_exit);
