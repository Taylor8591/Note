#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/sysfs.h>

#include <asm/unaligned.h>



// static struct file_operations fops = {
//     .open = NULL,
//     .release = NULL,
//     .read = NULL,
//     .write = NULL,
// };

static int probe(struct platform_device *pdev)
{
	printk(KERN_EMERG "test_probe\n");
    
    struct pwm_device *pwm;
    int ret;

    pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(pwm)) {
        printk(KERN_EMERG "Failed to get PWM device\n");
        return 0;
    }
    else {
        printk(KERN_EMERG "get pwm success\n");
    }

    ret = pwm_config(pwm, 50000, 100000);
    if (ret < 0) {
        printk(KERN_EMERG "Failed to configure PWM\n");
        return ret;
    }

    ret = pwm_enable(pwm);
    if (ret < 0) {
        printk(KERN_EMERG "Failed to enable PWM\n");
        return ret;
    }
    printk(KERN_EMERG "pwm enable success\n");

	return 0;
}


static int pwm_driver_remove(struct platform_device *pdev)
{
    struct pwm_device *pwm = dev_get_drvdata(&pdev->dev);
    pwm_disable(pwm);
    return 0;
}


static const struct of_device_id pwm_of_table[] = {
	{ .compatible = "pwm_test" },
	{ },
};
MODULE_DEVICE_TABLE(of, pwm_of_table);

static struct platform_driver pwm_driver = {
	.driver = {
		.name	= "pwm_test",
		.of_match_table = pwm_of_table,
	},
	.probe		= probe,
    .remove = pwm_driver_remove,
};
module_platform_driver(pwm_driver);

MODULE_LICENSE("GPL");
