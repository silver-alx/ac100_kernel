#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/slab.h> 
#include <linux/errno.h> 
#include <linux/types.h> 
#include <linux/proc_fs.h>
#include <linux/fcntl.h> 
#include <asm/system.h> 
#include <asm/uaccess.h> 
#include <linux/cdev.h>     
#include <linux/fs.h>        
#include <linux/device.h>    
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
//#include "linux/nvos_ioctl.h"
#include "nvec.h"
//#include "linux/nvec_ioctls.h"
#include "nvreftrack.h"
#include "nvassert.h"
#include "nvec_device.h"
#include "nvodm_ec.h"

MODULE_AUTHOR("John Ye");
MODULE_DESCRIPTION("Compal EC ODM Driver");
MODULE_LICENSE("GPL");

#define COMPAL_EC_COMMAND			    0x10		// Execute EC command
#define COMPAL_EC_ADDRESS_READ_UUID		0x4E		// Read SPI ROM command
#define COMPAL_EC_ADDRESS_WRITE_UUID	0x4D		// Write SPI ROM command

#define DEVICE_NAME   "ec_odm"
#define MAMORY_MAJOR  60

static char *ec_odm_buffer;

static dev_t  devid;
static struct cdev     *ec_odm_cdev; 
static struct class    *ec_odm_class;

NvEcHandle s_NvEcHandle = NULL;

static char ec_version[20];

static int uuid_read_flag = 0;
static int manufacture_read_flag = 0;
static int product_name_read_flag = 0;
static int version_read_flag = 0;
static int serial_number_read_flag = 0;
static int oem_strings_read_flag = 0;

extern int NvEcGetBatteryReqeustFlag(void);
NvError
NvEcSendRequest_early(
    NvEcHandle hEc,
    NvEcRequest *pRequest,
    NvEcResponse *pResponse,
    NvU32 RequestSize,
    NvU32 ResponseSize)
{
	 unsigned status;
	//printk(KERN_ALERT ">%s\n", __FUNCTION__);
	status=NvEcGetBatteryReqeustFlag();
	//printk(KERN_ALERT ">status=%i\n", status);
	if (status==1){
	   printk(KERN_ALERT ">bypass odm req\n");
	   return NvSuccess-1;
	}
	return NvEcSendRequest(hEc, pRequest, pResponse, RequestSize, ResponseSize);
}

//Henry add get system event status
NvU32 get_systemevent_status(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = 0x01;
    Request.RequestSubtype = 0x00;   
    Request.NumPayloadBytes = 0;
    NvU32 ret=0;
    if(s_NvEcHandle==NULL){
	printk( KERN_ERR "Get system event status ERROR\n" );
	return 0;
    }

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Read system event status NvEcSendRequest_early() Req Failed\n" );
        return 0;
    }   
    if (Response.Status != NvEcStatus_Success ){
	printk( KERN_INFO "Read systemevent status NvEcSendRequest_early() Resp Failed\n" );
        return 0;
    }

    ret=(Response.Payload[3] << 24) | (Response.Payload[2] <<16) | (Response.Payload[1] <<8) | Response.Payload[0];
            
    return ret;
}

int ec_odm_open(struct inode *inode, struct file *filp) 
{   
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);    

    return 0;
}

int ec_odm_release(struct inode *inode, struct file *filp) 
{
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);
    
    return 0;
}

ssize_t ec_odm_read(struct file *filp, char __user *read_buf,
                     size_t count, loff_t *f_pos) 
{
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);    

    int ret = 0;
    int i;
    char *memory_buffer;
    
    #if 0
    if (copy_from_user(read_buf, memory_buffer, count)) {
        ret =  - EFAULT;    
    } else {       
        *f_pos += count;
        ret = count;
    }

    //printk(KERN_ALERT "IN KERNEL, write %d bytes!\n", ret);

    //for (i = 0; i < count; i++){
    //    printk(KERN_ALERT "IN KERNEL, write buf: %c\n", memory_buffer[i]);
    //}
    #endif    

    return ret;   
}

ssize_t ec_odm_write( struct file *filp, char __user *write_buf,
                      size_t count, loff_t *f_pos) 
{
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);

    int ret = 0;
    int i;
    char *memory_buffer;

    #if 0    
    if (copy_from_user(memory_buffer, write_buf, count)) {
        ret =  - EFAULT;    
    } else {       
        *f_pos += count;
        ret = count;
    }

    printk(KERN_ALERT "IN KERNEL, write %d bytes!\n", ret);

    for (i = 0; i < count; i++){
        printk(KERN_ALERT "IN KERNEL, write buf: %c\n", memory_buffer[i]);
    }
    #endif

    return ret;
}

static int get_ec_version(ec_odm_dmi_params *dmi_params)
{
    NvU8 unused = 0, major = 0, minor = 0, test = 0;
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    NvEcControlGetFirmwareVersionResponsePayload EcVersion;

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_Control;
    Request.RequestSubtype = NvEcControlSubtype_GetFirmwareVersion;
    Request.Payload[0] = 0;
    Request.NumPayloadBytes = 0;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;

    if (Response.Status != NvEcStatus_Success )
        return -1;

    NvOsMemcpy(&EcVersion, Response.Payload, Response.NumPayloadBytes);

    test = EcVersion.VersionMinor[0];
    minor = EcVersion.VersionMinor[1];
    major = EcVersion.VersionMajor[0];
    unused= EcVersion.VersionMajor[1];
    if ( test == 0 )
        sprintf(ec_version, "%X.%02X", major, minor);
    else
        sprintf(ec_version, "%X.%02XT%02X", major, minor, test);
 
    strcpy(dmi_params->str_read_buf, ec_version);

    return 0;
}

static int write_uuid_to_ec(NvU8 uuid_offset, NvU8 uuid_value)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = COMPAL_EC_ADDRESS_WRITE_UUID;
    Request.Payload[1] = uuid_offset;
    Request.Payload[2] = uuid_value;
    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "UUID NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    
    return 0;
}

/* Convert Hex to Int */
static NvU8 htoi( char *tem_hex )
{
    NvU8 k = 0;
   
    if ( *tem_hex >= '0' && *tem_hex <= '9' )
        k = ( *tem_hex - '0' ) * 16;
    else if ( *tem_hex >= 'a' && *tem_hex <= 'f' )
        k = ( *tem_hex - 'a' + 10 ) * 16;
    else if ( *tem_hex >= 'A' && *tem_hex <= 'F' )
        k = ( *tem_hex - 'A' + 10 ) * 16;
    tem_hex++;
    if ( *tem_hex >= '0' && *tem_hex <= '9' )
        k += ( *tem_hex - '0' );
    else if ( *tem_hex >= 'a' && *tem_hex <= 'f' )
        k += ( *tem_hex - 'a' + 10 );
    else if ( *tem_hex >= 'A' && *tem_hex <= 'F' )
        k += ( *tem_hex - 'A' + 10 ) ;

    return k;
}

static int write_uuid(ec_odm_dmi_params *dmi_params)
{
    NvU8 i = 0;
    NvU8 uuid_to_write[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    char tem_hex[3] = { 0,0,0 };
    int len = 0;

    for (i = 0; i < 32; i++) {
        tem_hex[0] = dmi_params->str_write_buf[i++];
        tem_hex[1] = dmi_params->str_write_buf[i];

        uuid_to_write[len] = htoi(tem_hex);
        len++;
    }
   
    for (i = 0; i < 16; i++) {
        write_uuid_to_ec(i, uuid_to_write[i]);
    }

    uuid_read_flag = 0;
    
    return 0;
}

static int read_uuid_from_ec(NvU8 uuid_offset, NvU8 *uuid_value )
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = COMPAL_EC_ADDRESS_READ_UUID;
    Request.Payload[1] = uuid_offset;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "UUID NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
        printk( KERN_INFO "Response Failed\n" );
        return -1;
    }

    *uuid_value = Response.Payload[0];

    return( Response.Payload[0] );
}

static int read_uuid(ec_odm_dmi_params *dmi_params)
{ 
    NvU8 *uuid_value;
 
    NvU8 i = 0;
    static NvU8 strUuid[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    if (uuid_read_flag == 0) {

        for (i = 0; i < 16; i++) {
            read_uuid_from_ec(i, strUuid + i);
        }

        uuid_read_flag = 1;
    }

    for (i = 0; i < 16; i++) {
        dmi_params->read_buf[i] = strUuid[i];
    }

    return 0;
}

static int write_spi_rom(NvU8 offset, NvU8 value)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x40;
    Request.Payload[1] = 0x00;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Select SPI bank NvEcSendRequest_early() Failed\n" );
        return -1;
    }

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = COMPAL_EC_ADDRESS_WRITE_UUID;
    Request.Payload[1] = offset;
    Request.Payload[2] = value;
    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Write SPI ROM NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    
    return 0;
}

static int write_manufacture(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;

    //printk("strlen(dmi_params->str_write_buf) = %d\n", strlen(dmi_params->str_write_buf));
   
    for (i = 0x10; i < 0x20; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0x10]);
    }

    manufacture_read_flag = 0;

    return 0;
}

static int read_spi_rom(NvU8 offset, NvU8 *value)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x40;
    Request.Payload[1] = 0x00;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Select SPI bank NvEcSendRequest_early() Failed\n" );
        return -1;
    }

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = COMPAL_EC_ADDRESS_READ_UUID;
    Request.Payload[1] = offset;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Read SPI ROM NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    if (Response.Status != NvEcStatus_Success ) {
        printk( KERN_INFO "Response Failed\n" );
        return -1;
    }

    *value = Response.Payload[0];

    return( Response.Payload[0] );
}

static int read_manufacture(ec_odm_dmi_params *dmi_params)
{ 
    NvU8 i = 0;
    static char manufacture_buf[17] = "";
    static int manufacture_len = 0;

    if (manufacture_read_flag == 0) {
        for (i = 0x10; i < 0x20; i++) {
            if (read_spi_rom(i, (manufacture_buf + i - 0x10)) == 0) {
                break;
            }
        }

        manufacture_buf[i - 0x10] = '\0';
        manufacture_len = i - 0x10;

        manufacture_read_flag = 1;
    }

    for (i = 0; i < (manufacture_len + 1); i++) {
        dmi_params->str_read_buf[i] = manufacture_buf[i];
    }

    return 0;
}

static int write_product_name(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;
   
    for (i = 0x20; i < 0x40; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0x20]);
    }

    product_name_read_flag = 0;

    return 0;
}

static int read_product_name(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    static char product_name_buf[33] = "";
    static int product_name_len = 0;

    if (product_name_read_flag == 0) {
        for (i = 0x20; i < 0x40; i++) {
            if (read_spi_rom(i, (product_name_buf + i - 0x20)) == 0 ) {
                break;
            }
        }

        product_name_buf[i - 0x20] = '\0';
        product_name_len = i - 0x20; 

        product_name_read_flag = 1;
    }

    for (i = 0; i < (product_name_len + 1); i++) {
        dmi_params->str_read_buf[i] = product_name_buf[i];
    }


    return 0;
}

static int write_version(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;
   
    for (i = 0x40; i < 0x60; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0x40]);
    }

    version_read_flag = 0;

    return 0;
}

static int read_version(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    static char version_buf[33] = "";
    static int version_len = 0;

    if (version_read_flag == 0) {
        for (i = 0x40; i < 0x60; i++) {
            read_spi_rom(i, (version_buf + i - 0x40));
        }
    
        version_buf[i - 0x40] = '\0';
        version_len = i - 0x40;
       
        version_read_flag = 1;
    }

    for (i = 0; i < (version_len + 1); i++) {
        dmi_params->str_read_buf[i] = version_buf[i];
    }

    return 0;
}

static int write_serial_number(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;
   
    for (i = 0x60; i < 0x80; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0x60]);
    }

    serial_number_read_flag = 0;

    return 0;
}

static int read_serial_number(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    static char serial_number_buf[33] = "";
    static int serial_number_len = 0;

    if (serial_number_read_flag == 0) {
        for (i = 0x60; i < 0x80; i++) {
            if (read_spi_rom(i, (serial_number_buf + i - 0x60)) == 0) {
                break;
            }
        }

        serial_number_buf[i - 0x60] = '\0';
        serial_number_len = i - 0x60;

        serial_number_read_flag = 1;
    }

    for (i = 0; i < (serial_number_len + 1); i++) {
        dmi_params->str_read_buf[i] = serial_number_buf[i];
    }

    return 0;
}

static int write_oem_strings(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;
   
    for (i = 0x80; i < 0xC0; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0x80]);
    }

    oem_strings_read_flag = 0;

    return 0;
}

static int read_oem_strings(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    static char oem_strings_buf[65] = "";
    static int oem_strings_len = 0;

    if (oem_strings_read_flag == 0) {
        for (i = 0x80; i < 0xC0; i++) {
            if (read_spi_rom(i, (oem_strings_buf + i - 0x80)) == 0) {
                break;
            }
        }

        oem_strings_buf[i - 0x80] = '\0';
        oem_strings_len = i - 0x80;

        oem_strings_read_flag = 1;
    }

    for (i = 0; i < (oem_strings_len + 1); i++) {
        dmi_params->str_read_buf[i] = oem_strings_buf[i];
    }

    return 0;
}

static int write_keyboard_info(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    int len = 0;
   
    for (i = 0xD0; i < 0xD4; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0xD0]);
    }

    return 0;
}

static int read_keyboard_info(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);   

    NvU8 i = 0;
    char keyboard_info_buf[5] = "";

    for (i = 0xD0; i < 0xD4; i++) {
        read_spi_rom(i, (keyboard_info_buf + i - 0xD0));
    }

    keyboard_info_buf[4] = '\0';

    for (i = 0; i < 4; i++) {
        dmi_params->str_read_buf[i] = keyboard_info_buf[i];
    }

    return 0;
}

static int write_country_code(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);

    NvU8 i = 0;
    int len = 0;

    for (i = 0xD4; i < 0xD6; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0xD4]);
    }

    return 0;
}

static int read_country_code(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);

    NvU8 i = 0;
    char country_code_buf[3] = "";

    for (i = 0xD4; i < 0xD6; i++) {
        read_spi_rom(i, (country_code_buf + i - 0xD4));
    }

    country_code_buf[2] = '\0';

    for (i = 0; i < 2; i++) {
        dmi_params->str_read_buf[i] = country_code_buf[i];
    }

    return 0;
}

static int write_config_for_test(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);

    NvU8 i = 0;
    int len = 0;

    for (i = 0xD6; i < 0xD7; i++) {
        write_spi_rom(i, dmi_params->str_write_buf[i - 0xD6]);
    }

    return 0;
}

static int read_config_for_test(ec_odm_dmi_params *dmi_params)
{
    //printk("%s\n", __FUNCTION__);

    NvU8 i = 0;
    char config_for_test_buf[2] = "";

    for (i = 0xD6; i < 0xD7; i++) {
        read_spi_rom(i, (config_for_test_buf + i - 0xD6));
    }

    config_for_test_buf[1] = '\0';

    for (i = 0; i < 1; i++) {
        dmi_params->str_read_buf[i] = config_for_test_buf[i];
    }

    return 0;
}

static int dmi_opt(ec_odm_dmi_params *dmi_params)
{
    switch (dmi_params->operation) {
        case READ_EC_VERSION:
            get_ec_version(dmi_params);
            break;
        case WRITE_UUID:
            write_uuid(dmi_params);
            break;
        case READ_UUID:
            read_uuid(dmi_params);
            break;
        case WRITE_MANUFACTURE:
            write_manufacture(dmi_params);
            break;
        case READ_MANUFACTURE:
            read_manufacture(dmi_params);
            break;
        case WRITE_PRODUCT_NAME:
            write_product_name(dmi_params);
            break;
        case READ_PRODUCT_NAME:
            read_product_name(dmi_params);
            break;
        case WRITE_VERSION:
            write_version(dmi_params);
            break;
        case READ_VERSION:
            read_version(dmi_params);
            break;
        case WRITE_SERIAL_NUMBER:
            write_serial_number(dmi_params);
            break;
        case READ_SERIAL_NUMBER:
            read_serial_number(dmi_params);
            break;
        case WRITE_OEM_STRINGS:
            write_oem_strings(dmi_params);
            break;
        case READ_OEM_STRINGS:
            read_oem_strings(dmi_params);
            break;
        case WRITE_KEYBOARD_INFO:
            write_keyboard_info(dmi_params);
            break;
        case READ_KEYBOARD_INFO:
            read_keyboard_info(dmi_params);
            break;
        case WRITE_COUNTRY_CODE:
            write_country_code(dmi_params);
            break;
        case READ_COUNTRY_CODE:
            read_country_code(dmi_params);
            break;
        case WRITE_CONFIG_FOR_TEST:
            write_config_for_test(dmi_params);
            break;
        case READ_CONFIG_FOR_TEST:
            read_config_for_test(dmi_params);
            break;
        default:
            break;
    }     

    return 0;  
}

static int write_keyboard_type(ec_odm_keyboard_params *keyboard_params)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x40;
    Request.Payload[1] = 0x00;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Write keyboard type NvEcSendRequest_early() Failed\n" );
        return -1;
    }

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x4D;
    Request.Payload[1] = 0xF0;
    Request.Payload[2] = keyboard_params->write_buf;
    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Write keyboard type NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    
    return 0;
}

static int write_keyboard_to_ram(ec_odm_keyboard_params *keyboard_params)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x59;
    if (keyboard_params->write_buf == 'U') {
        Request.Payload[1] = 0xE5;
    } else if (keyboard_params->write_buf == 'J') {
        Request.Payload[1] = 0xE6;
    } else if (keyboard_params->write_buf == 'K') {
        Request.Payload[1] = 0xE7;
    }
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Write keyboard type NvEcSendRequest_early() Failed\n" );
        return -1;
    }
    
    return 0;
}

static int read_keyboard_type(ec_odm_keyboard_params *keyboard_params)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x40;
    Request.Payload[1] = 0x00;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Write keyboard type NvEcSendRequest_early() Failed\n" );
        return -1;
    }

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0x4E;
    Request.Payload[1] = 0xF0;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Read keyboard type NvEcSendRequest_early() Failed\n" );
        return -1;
    }

    //printk("keyboard type value: %d\n", Response.Payload[0]);
    keyboard_params->read_buf = Response.Payload[0];
    
    return 0;
}

static int keyboard_cmd(int sub_cmd, int data)
{
    int i = 0;
    
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = 0x05; //keyboard command byte
    Request.RequestSubtype = sub_cmd;//sub command
    Request.Payload[0] = data;

    Request.NumPayloadBytes = 1;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Wakeup event config NvEcSendRequest_early() Failed\n" );
        return -1;
    }    
    return 0;
}

static int keyboard_opt(ec_odm_keyboard_params *keyboard_params)
{
    switch (keyboard_params->operation) {
        case WRITE_KEYBOARD_TYPE:
            //printk( "WRITE_KEYBOARD_TYPE\n" );
            write_keyboard_type(keyboard_params);
            write_keyboard_to_ram(keyboard_params);
            break;
        case READ_KEYBOARD_TYPE:
            //printk( "READ_KEYBOARD_TYPE\n" );
            read_keyboard_type(keyboard_params);
            //read_keyboard_from_ram(keyboard_params);
            break;
        case ENABLE_CAPSLOCK:   
            //printk( "ENABLE_CAPSLOCK\n" );
            keyboard_cmd(0xED, 0x01);
            break;
        case DISABLE_CAPSLOCK:
            //printk( "DISABLE_CAPSLOCK\n" );
            keyboard_cmd(0xED, 0x00);
            break;
        default:
            break;
    }     

    return 0;  
}

static int read_touchpad_status(ec_odm_touchpad_params *touchpad_params)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = COMPAL_EC_COMMAND;
    Request.Payload[0] = 0xB0;
    Request.Payload[1] = 0x9E;
    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Read touchpad status NvEcSendRequest_early() Failed\n" );
        return -1;
    }   
    
    if ((Response.Payload[0] & 0x08) == 0x00) {
        touchpad_params->read_buf = 1;//tochpad status is enable
    } else if ((Response.Payload[0] & 0x08) == 0x01) {
        touchpad_params->read_buf = 0;//tochpad status is disable    
    }

    //printk("Touchpad status: %d\n", touchpad_params->read_buf);
    return 0;
}

static int check_touchpad_status(ec_odm_touchpad_params *touchpad_params)
{
    return read_touchpad_status(touchpad_params);
}

extern void set_elan_wheel_mode(unsigned mode);
extern unsigned get_elan_wheel_mode(void);

static int check_touchpad_srcoll_status(ec_odm_touchpad_params *touchpad_params)
{
    touchpad_params->read_buf = get_elan_wheel_mode();   

    return 0;
}

static int enable_touchpad_scroll(ec_odm_touchpad_params *touchpad_params)
{
    set_elan_wheel_mode(1);  

    return 0;
}

static int disable_touchpad_scroll(ec_odm_touchpad_params *touchpad_params)
{
    set_elan_wheel_mode(0);   

    return 0;
}

//henry++ add support setting mouse speed
extern void set_mouse_speed(int speed);
static int set_touchpad_move_speed(ec_odm_touchpad_params *touchpad_params)
{
 // pr_info("%s,write_buf=%i\n",__FUNCTION__,touchpad_params->write_buf);
  if(touchpad_params->write_buf>5){
     touchpad_params->write_buf=5;
   }
  else if (touchpad_params->write_buf<1){
     touchpad_params->write_buf=1;
   }
//pr_info("%s,speed=%i\n",__FUNCTION__,touchpad_params->write_buf);
    set_mouse_speed(touchpad_params->write_buf);   
    return 0;
}

//extern int check_touchpad_status(ec_odm_touchpad_params *touchpad_params);
extern int enable_touchpad(ec_odm_touchpad_params *touchpad_params);
extern int disable_touchpad(ec_odm_touchpad_params *touchpad_params);

static int touchpad_opt(ec_odm_touchpad_params *touchpad_params)
{
    switch (touchpad_params->operation) {
        case CHECK_TOUCHPAD_STATUS:
            //printk("CHECK_TOUCHPAD_STATUS\n");
            check_touchpad_status(touchpad_params);
            break;
        case ENABLE_TOUCHPAD:
            //printk("ENABLE_TOUCHPAD\n");
            //enable_touchpad(touchpad_params);
            break;
        case DISABLE_TOUCHPAD:
            //printk("DISABLE_TOUCHPAD\n");
            //disable_touchpad(touchpad_params);
            break;
        case CHECK_TOUCHPAD_SCROLL_STATUS:
            //printk("CHECK_TOUCHPAD_SCROLL_STATUS\n");
            check_touchpad_srcoll_status(touchpad_params);
            break;
        case ENABLE_TOUCHPAD_SCROLL:
            //printk("ENABLE_TOUCHPAD_SCROLL\n");
            enable_touchpad_scroll(touchpad_params);
            break;
        case DISABLE_TOUCHPAD_SCROLL:
            //printk("DISABLE_TOUCHPAD_SCROLL\n");
            disable_touchpad_scroll(touchpad_params);
            break;
	//henry++ setting mouse move speed
        case SETTING_TOUCHPAD_SPEED:
            //printk("DISABLE_TOUCHPAD_SCROLL\n");
            set_touchpad_move_speed(touchpad_params);
            break;
        default:
            break;
    }     

    return 0;  
}

static int set_led_pattern(int pattern)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    /* Disable led */
    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0x10;
    Request.Payload[2] = 0x00;

    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;

    /* Set led with according to the pattern */
    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0x10;
    Request.Payload[2] = pattern;

    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;

    return 0;
}

static int led_opt(ec_odm_led_params *led_params)
{
    int pattern = 0;    
    
    switch (led_params->operation) {        
        case SET_LED_PATTERN_FIRST:
            //printk( "SET_LED_PATTERN_FIRST\n" );
            pattern = 0x01;
            break;
        case SET_LED_PATTERN_SECOND:
            //printk( "SET_LED_PATTERN_SECOND\n" );
            pattern = 0x02;
            break;
        case SET_LED_PATTERN_THIRD:
            //printk( "SET_LED_PATTERN_THIRD\n" );
            pattern = 0x03;
            break;
        case SET_LED_PATTERN_FOURTH:
            //printk( "SET_LED_PATTERN_FOURTH\n" );
            pattern = 0x04;
            break;
        case SET_LED_PATTERN_FIFTH:
            //printk( "SET_LED_PATTERN_FIFTH\n" );
            pattern = 0x05;
            break;
        case SET_LED_PATTERN_SIXTH:
            //printk( "SET_LED_PATTERN_SIXTH\n" );
            pattern = 0x06;
            break;
        case SET_LED_PATTERN_SEVENTH:
            //printk( "SET_LED_PATTERN_SEVENTH\n" );
            pattern = 0x07;
            break;
        case SET_LED_PATTERN_EIGHTH:
            //printk( "SET_LED_PATTERN_EIGHTH\n" );
            pattern = 0x08;
            break;
        case DISABLE_LED:
            //printk( "DISABLE_LED\n" );
            pattern = 0x00;
            break;
        default:
            break;
    }     

    set_led_pattern(pattern);

    return 0;  
}

static int get_recovery_pin_status(ec_odm_ec_params *ec_params)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0xF8;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;        

    //printk("%s, Response.Payload[0] = %d\n", __FUNCTION__, Response.Payload[0]);
    ec_params->read_buf = Response.Payload[0];

    return 0;
}

static int recovery_pin_up(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0xF6;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;        

    return 0;
}

static int recovery_pin_down(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x45;
    Request.Payload[1] = 0xF7;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int suspend(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x59;
    Request.Payload[1] = 0xE9;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int resume(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x59;
    Request.Payload[1] = 0xE8;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int set_power_led(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x47;
    Request.Payload[1] = 0x81;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int set_battery_low_led(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x47;
    Request.Payload[1] = 0x83;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int set_battery_full_led(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x47;
    Request.Payload[1] = 0x84;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int disable_power_battery_led(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x47;
    Request.Payload[1] = 0x88;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

static int ec_control_led(void)
{
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x47;
    Request.Payload[1] = 0x80;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;       

    return 0; 
}

extern int power_button_control_lcd(void);
extern int power_button_control_suspend(void);
extern int lid_switch_control_lcd(void);
extern int lid_switch_control_suspend(void);

static int ec_opt(ec_odm_ec_params *ec_params)
{
    switch (ec_params->operation) {        
        case GET_RECOVERY_PIN_STATUS:
            //printk( "GET_RECOVERY_PIN_STATUS\n" );
            get_recovery_pin_status(ec_params);
            break;
        case RECOVERY_PIN_UP:
            //printk( "RECOVERY_PIN_UP\n" );
            recovery_pin_up();
            break;
        case RECOVERY_PIN_DOWN:
            //printk( "RECOVERY_PIN_DOWN\n" );
            recovery_pin_down();
            break;
        case SUSPEND:
            //printk( "SUSPEND\n" );
            suspend();
            break;
        case RESUME:
            //printk( "RESUME\n" );
            resume();
            break;
        case SET_POWER_LED:
            //printk( "SET_POWER_LED\n" );
            set_power_led();
            break;
        case SET_BATTERY_LOW_LED:
            //printk( "SET_BATTERY_LOW_LED\n" );
            set_battery_low_led();
            break;
        case SET_BATTERY_FULL_LED:
            //printk( "SET_BATTERY_FULL_LED\n" );
            set_battery_full_led();
            break;
        case DISABLE_POWER_BATTERY_LED:
            //printk( "DISABLE_POWER_BATTERY_LED\n" );
            disable_power_battery_led();
            break;
        case EC_CONTROL_LED:
            //printk( "EC_CONTROL_LED\n" );
            ec_control_led();
            break;
        case POWER_BUTTON_CONTROL_LCD:
            //printk( "POWER_BUTTON_CONTROL_LCD\n" );
            power_button_control_lcd();
            break;
        case POWER_BUTTON_CONTROL_SUSPEND:
            //printk( "POWER_BUTTON_CONTROL_SUSPEND\n" );
            power_button_control_suspend();
            break;
        case LID_SWITCH_CONTROL_LCD:
            //printk( "LID_SWITCH_CONTROL_LCD\n" );
            lid_switch_control_lcd();
            break;
        case LID_SWITCH_CONTROL_SUSPEND:
            //printk( "LID_SWITCH_CONTROL_SUSPEND\n" );
            lid_switch_control_suspend();
            break;
        default:
            break;
    }  

    return 0;  
}

static int keyboard_wakeup_event_config(int mask_bit, int config_action)
{
    int i = 0;
    
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = 0x05; //keyboard wakeup event command byte
    Request.RequestSubtype = 0x03;//sub command
    Request.Payload[0] = config_action;//enable or disable event reporting
    Request.Payload[1] = mask_bit;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Wakeup event config NvEcSendRequest_early() Failed\n" );
        return -1;
    }    
    return 0;
}

static int wakeup_event_config_fun(int config_field, int mask_bit, int config_action)
{
    int i = 0;
    
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = 0x01; //system event command byte
    Request.RequestSubtype = 0xFD;//sub command
    Request.Payload[0] = config_action;//enable or disable event reporting
    for (i = 1; i < 5; i++) {
        if (i != config_field) {
            Request.Payload[i] = 0x00;
        } else {
            Request.Payload[i] = mask_bit;
        }
    }

    Request.NumPayloadBytes = 5;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "Wakeup event config NvEcSendRequest_early() Failed\n" );
        return -1;
    }    
    return 0;
}

static int wakeup_event_config(ec_odm_event_params *event_params)
{
    //printk("%s\n", __FUNCTION__);

    switch (event_params->sub_operation) {
        case ENABLE_POWER_BUTTON_WAKEUP_EVENT:
            //printk("ENABLE_POWER_BUTTON_WAKEUP_EVENT\n");
            wakeup_event_config_fun(0x03, 0x01, 0x01);//Power button wakeup event mask bit: OEM system wakeup event bit 0
            break;
        case DISABLE_POWER_BUTTON_WAKEUP_EVENT:
            //printk("DISABLE_POWER_BUTTON_WAKEUP_EVENT\n");
            wakeup_event_config_fun(0x03, 0x01, 0x00);
            break;
        case ENABLE_LID_SWITCH_WAKEUP_EVENT:
            //printk("ENABLE_LID_SWITCH_WAKEUP_EVENT\n");
            wakeup_event_config_fun(0x03, 0x02, 0x01);//Lid switch wakeup event mask bit: OEM system wakeup event bit 1
            break;
        case DISABLE_LID_SWITCH_WAKEUP_EVENT:
            //printk("DISABLE_LID_SWITCH_WAKEUP_EVENT\n");
            wakeup_event_config_fun(0x03, 0x02, 0x00);
            break;
        case ENABLE_HOMEKEY_WAKEUP_EVENT:
            //printk("ENABLE_HOMEKEY_WAKEUP_EVENT\n");
            keyboard_wakeup_event_config(0x02, 0x01);//Homekey wakeup event mask bit: keyboard wakeup event bit 1
            keyboard_wakeup_event_config(0x01, 0x00);//Disable homekey wakeup event mask bit: keyboard any key wakeup event bit 0
            break;
        case DISABLE_HOMEKEY_WAKEUP_EVENT:
            //printk("DISABLE_HOMEKEY_WAKEUP_EVENT\n");
            keyboard_wakeup_event_config(0x02, 0x00);
            break;
        default:
            break;
    }  

    return 0;
}

static int system_event_config_fun(int config_field, int mask_bit, int config_action)
{
    int i = 0;
    
    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = 0x01; //system event command byte
    Request.RequestSubtype = 0x01;//sub command
    Request.Payload[0] = config_action;//enable or disable event reporting
    for (i = 1; i < 5; i++) {
        if (i != config_field) {
            Request.Payload[i] = 0x00;
        } else {
            Request.Payload[i] = mask_bit;
        }
    }

    Request.NumPayloadBytes = 5;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success) {
        printk( KERN_INFO "System event config NvEcSendRequest_early() Failed\n" );
        return -1;
    }    

   return 0;
}

static int system_event_config(ec_odm_event_params *event_params)
{
    //printk("%s\n", __FUNCTION__);

    switch (event_params->sub_operation) {
        case ENABLE_AC_PRESENT_EVENT:
            //printk("ENABLE_AC_PRESENT_EVENT\n");
            system_event_config_fun(0x01, 0x01, 0x01);//AC present event mask bit: system event bit 0
            break;
        case DISABLE_AC_PRESENT_EVENT:
            //printk("ENABLE_AC_PRESENT_EVENT\n");
            system_event_config_fun(0x01, 0x01, 0x00);
            break;
        case ENABLE_LID_SWITCH_EVENT:
            //printk("ENABLE_LID_SWITCH_EVENT\n");
            system_event_config_fun(0x03, 0x02, 0x01);//Lid switch event mask bit: OEM system event bit 1
            break;
        case DISABLE_LID_SWITCH_EVENT:
            //printk("DISABLE_LID_SWITCH_EVENT\n");
            system_event_config_fun(0x03, 0x02, 0x00);
            break;
        case ENABLE_POWER_BUTTON_EVENT:
            //printk("ENABLE_POWER_BUTTON_EVENT\n");
            system_event_config_fun(0x03, 0x80, 0x01);//Power button event mask bit: OEM system event bit 7
            break;
        case DISABLE_POWER_BUTTON_EVENT:
            //printk("DISABLE_POWER_BUTTON_EVENT\n");
            system_event_config_fun(0x03, 0x80, 0x00);
            break;
        case ENABLE_USB_OVER_CURRENT:
            //printk("ENABLE_USB_OVER_CURRENT\n");
            system_event_config_fun(0x03, 0x40, 0x01);//USB event mask bit: OEM system event bit 6
            break;
        case DISABLE_USB_OVER_CURRENT:
            //printk("DISABLE_USB_OVER_CURRENT\n");
            system_event_config_fun(0x03, 0x40, 0x00);
            break;
        default:
            break;
    }  

    return 0;
}

static int event_opt(ec_odm_event_params *event_params)
{
    switch (event_params->operation) {
        case SYSTEM_EVENT_CONFIG:
            //printk("SYSTEM_EVENT_CONFIG\n");
            system_event_config(event_params);
            break;
        case WAKEUP_EVENT_CONFIG:
            //printk("SYSTEM_EVENT_CONFIG\n");
            wakeup_event_config(event_params);
            break;

        //case BATTERY_EVENT_CONFIG:
        //    printk("BATTERY_EVENT_CONFIG\n");
            //battery_event_config(event_params);
        //    break;
        default:
            break;
    }     

    return 0;  
}

extern int paz00_set_lcd_output(int mode);

static int mute(void)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x59;
    Request.Payload[1] = 0x94;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;        

    return 0;    
}

static int unmute(void)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x59;
    Request.Payload[1] = 0x95;

    Request.NumPayloadBytes = 2;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;        

    return 0;    
}

static int auto_resume(ec_odm_append_params *append_params)
{
    //printk("%s\n", __FUNCTION__);

    NvError NvStatus = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_OEM0;
    Request.RequestSubtype = 0x10;
    Request.Payload[0] = 0x40;
    Request.Payload[1] = 0x11;
    Request.Payload[2] = append_params->write_buf;

    Request.NumPayloadBytes = 3;

    NvStatus = NvEcSendRequest_early(s_NvEcHandle, &Request, &Response, sizeof(Request), sizeof(Response));
    if (NvStatus != NvError_Success)
        return -1;        

    return 0;    
}

static int append_opt(ec_odm_append_params *append_params)
{  
    switch (append_params->operation) {        
        case SHUT_DOWN:
            //printk( "SHUT_DOWN\n" );
            break;
        case ENABLE_LCD:
            //printk( "ENABLE_LCD\n" );
            paz00_set_lcd_output(1);
            break;
        case DISABLE_LCD:
            //printk( "DISABLE_LCD\n" );
            paz00_set_lcd_output(0);
            break;
        case MUTE:
            //printk( "MUTE\n" );
            mute();
            break;
        case UNMUTE:
            //printk( "UNMUTE\n" );
            unmute();
            break;
        case AUTO_RESUME:
            //printk( "AUTO_RESUME\n" );
            auto_resume(append_params);
            break;
        default:
            break;
    }     

    return 0;  
}

long ec_odm_unlocked_ioctl(struct file *file,
    unsigned int cmd, unsigned long arg)
{
    NvError err;
    //NvOsIoctlParams p;
    NvU32 size;
    NvU32 small_buf[8];
    void *ptr = 0;
    ec_odm_dmi_params dmi_params;
    ec_odm_keyboard_params keyboard_params;
    ec_odm_touchpad_params touchpad_params;
    ec_odm_led_params led_params;
    ec_odm_ec_params ec_params;
    ec_odm_event_params event_params;
    ec_odm_append_params append_params;

    switch (cmd) {
        case DMI_CMD:
            NvOsCopyIn(&dmi_params, (void *)arg, sizeof(dmi_params));
            dmi_opt(&dmi_params);
            NvOsCopyOut((void *)arg, &dmi_params, sizeof(dmi_params));
            break;
        case KEYBOARD_CMD:
            NvOsCopyIn(&keyboard_params, (void *)arg, sizeof(keyboard_params));
            keyboard_opt(&keyboard_params);
            NvOsCopyOut((void *)arg, &keyboard_params, sizeof(keyboard_params));
            break;
        case TOUCHPAD_CMD:
            NvOsCopyIn(&touchpad_params, (void *)arg, sizeof(touchpad_params));
            touchpad_opt(&touchpad_params);
            NvOsCopyOut((int *)arg, &touchpad_params, sizeof(touchpad_params));
            break;
        case LED_CMD:
            NvOsCopyIn(&led_params, (void *)arg, sizeof(led_params));
            led_opt(&led_params);
            NvOsCopyOut((void *)arg, &led_params, sizeof(led_params));
            break;       
        case EC_CMD:
            NvOsCopyIn(&ec_params, (void *)arg, sizeof(ec_params));
            ec_opt(&ec_params);
            NvOsCopyOut((int *)arg, &ec_params, sizeof(ec_params));
            break;
        case EVENT_CONFIG:
            NvOsCopyIn(&event_params, (void *)arg, sizeof(event_params));
            event_opt(&event_params);
            NvOsCopyOut((int *)arg, &event_params, sizeof(event_params));
            break;
        case APPEND_CMD:
            NvOsCopyIn(&append_params, (void *)arg, sizeof(append_params));
            append_opt(&append_params);
            NvOsCopyOut((int *)arg, &append_params, sizeof(append_params));
            break;
        
        default:
            printk( "unknown ioctl code\n" );
    }

    return 0;
}

struct file_operations ec_odm_fops = {
    .open           = ec_odm_open,
    .read           = ec_odm_read,
    .write          = ec_odm_write,
    .unlocked_ioctl = ec_odm_unlocked_ioctl,
    .release        = ec_odm_release,
};

int ec_odm_init(void) 
{
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);
//pr_info("Henry>>%s Begin===\n",__FUNCTION__);
    NvError NvStatus = NvError_Success;
    int err;

    NvStatus =  NvEcOpen(&s_NvEcHandle, 0 /* instance */);
    if (NvStatus != NvError_Success) {
        printk( KERN_ERR "PAZ00 Diagnostic: NvEcOpen() failed\n" );
        return -ENODEV;
    }

    ec_odm_cdev        = cdev_alloc(); 
    cdev_init(ec_odm_cdev, &ec_odm_fops); 
    ec_odm_cdev->owner = THIS_MODULE;

    alloc_chrdev_region(&devid, 0, 1, "ec_odm"); 

    err = cdev_add(ec_odm_cdev, devid, 1); 
    if (err) { 
        printk(KERN_NOTICE "Error %d adding device\n", err); 
        return -1; 
    }

    ec_odm_class = class_create(THIS_MODULE, "ec_odm_class"); 
    if (IS_ERR(ec_odm_class)) {
        printk(KERN_INFO "create class error\n"); 
        return -1; 
    } 
    device_create(ec_odm_class, NULL, devid, NULL, "ec_odm");
//pr_info("Henry>>%s End===\n",__FUNCTION__);
    return 0;
}

void ec_odm_exit(void) 
{
    //printk(KERN_ALERT "IN KERNEL, %s()\n", __FUNCTION__);

    unregister_chrdev(MAMORY_MAJOR, "ec_odm");

    if (ec_odm_buffer) {
        kfree(ec_odm_buffer);
    }

    //printk("<1>Removing ec_odm module\n");
}

module_init(ec_odm_init);
module_exit(ec_odm_exit);

