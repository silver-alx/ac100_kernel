#include <linux/module.h>
#include <linux/platform_device.h>
#include "mach/nvrm_linux.h"
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

#include "nvodm_tmon.h"


struct thermometer_dev
{
	int state;
	NvOdmTmonDeviceHandle hOdmTcore;
	struct device *dev;
	int temp;
};

struct class *thermometer_class;
static atomic_t device_count;
struct device *dev;
static unsigned int devno;


static ssize_t temp_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
    int temp = 0; 
    struct thermometer_dev *thdev = (struct thermometer_dev *)
		dev_get_drvdata(dev);
    
	if(!thdev->hOdmTcore)
        printk(KERN_INFO "NULL NAME\n");
    else
        NvOdmTmonTemperatureGet (thdev->hOdmTcore, &temp);
    
    printk(KERN_INFO "Driver temperature is %d\n", temp);
	
    if(!temp)
        printk(KERN_INFO "Error value\n");
	
	return sprintf(buf, "%d\n", temp*1000);
}

static DEVICE_ATTR(temp, S_IRUGO | S_IWUSR, temp_show, NULL);

static int __init tegra_thermometer_probe(struct platform_device *pdev)
{
	int index;
	struct thermometer_dev *thdev = kzalloc(sizeof(struct thermometer_dev), GFP_KERNEL);
	
	//create the node
	
	alloc_chrdev_region (&devno, 0, 1, "thermometer_dev");
	
	if (!thermometer_class)
	{
		thermometer_class = class_create (THIS_MODULE, "thermal1");
		if (IS_ERR(thermometer_class))
		    return PTR_ERR(thermometer_class);
	    atomic_set(&device_count, 0);
	}
	
	index = atomic_inc_return (&device_count);
	
	
	thdev->dev = device_create (thermometer_class, NULL, devno, NULL, "thermal_zone0");
	
	if (IS_ERR(thdev->dev))
		return PTR_ERR (thdev->dev);
	
	device_create_file (thdev->dev, &dev_attr_temp); 
	
	 
	thdev->temp = 0;
	
	
	//open the device
	thdev->hOdmTcore = NvOdmTmonDeviceOpen (NvOdmTmonZoneID_Core);
	
	dev_set_drvdata(thdev->dev, thdev);
	
	//NvOdmTmonTemperatureGet (thdev->hOdmTcore, &temp);
	
	dev = thdev->dev;

	return 0;
}

static __devexit 
tegra_thermometer_remove(struct platform_device *pdev)
{
    struct thermometer_dev *thdev = (struct thermometer_dev *)
        dev_get_drvdata(dev);
    NvOdmTmonDeviceClose(thdev->hOdmTcore);

    device_remove_file(thdev->dev, &dev_attr_temp);
    device_destroy(thdev->dev, devno);
    class_destroy(thermometer_class);
    device_unregister(thdev->dev);

	dev = NULL;
    
	return 0;	
}

#ifdef CONFIG_PM
static int tegra_thermometer_suspend(struct platform_device *pdev, 
               pm_message_t state)
{
    struct thermometer_dev *thdev = (struct thermometer_dev *)
        dev_get_drvdata(dev);
    NvOdmTmonSuspend(thdev->hOdmTcore);
    return 0;
}

static int tegra_thermometer_resume(struct platform_device *pdev)
{
    struct thermometer_dev *thdev = (struct thermometer_dev *)
        dev_get_drvdata(dev);
	NvOdmTmonResume(thdev->hOdmTcore);
    return 0;
}
#else
#define tegra_thermometer_suspend NULL
#define tegra_thermometer_resume  NULL
#endif

static struct platform_driver tegra_thermometer_driver = 
{
	.probe = tegra_thermometer_probe,
	.remove = __devexit_p(tegra_thermometer_remove),
    .suspend = tegra_thermometer_suspend,
    .resume = tegra_thermometer_resume,
	.driver = {
		.name = "tegra_thermometer",
	},
};


static int __devinit tegra_thermometer_init(void)
{
	return platform_driver_register(&tegra_thermometer_driver);
}

static void __exit tegra_thermometer_exit(void)
{
	platform_driver_unregister(&tegra_thermometer_driver);
}


module_init(tegra_thermometer_init);
module_exit(tegra_thermometer_exit);

MODULE_DESCRIPTION("thermometer driver test");
