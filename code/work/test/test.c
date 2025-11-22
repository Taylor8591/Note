#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

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



static struct file_operations fops = {
    .open = NULL,
    .release = NULL,
    .read = NULL,
    .write = NULL,
};

static int probe(struct spi_device *spi)
{
	printk(KERN_EMERG "test_probe\n");


	//spi
	u32 mode = SPI_MODE_0;    // 设置为 SPI 模式 0 (CPOL=0, CPHA=0)
	u32 speed = 800000;       // 设置 SPI 最大速度为 800 kHz
	u8 bits = 8;              // 8 位数据位宽
	struct cdev spi_cdev;
	dev_t dev_num;
	struct class *spi_class;

	//配置 SPI 设备
	spi->mode = mode;         // 设置 SPI 工作模式
	spi->max_speed_hz = speed; // 设置最大传输频率
	spi->bits_per_word = bits; // 设置每个字的位宽
    spi_setup(spi); // 进行配置更新

	alloc_chrdev_region(&dev_num, 0, 1, "test");
	cdev_init(&spi_cdev, &fops);
	spi_cdev.owner = THIS_MODULE;
	cdev_add(&spi_cdev, dev_num, 1);
	spi_class = class_create(THIS_MODULE, "test");
	device_create(spi_class, &spi->dev, dev_num, NULL, "test_spi");

/* *************************************************************** */

	uint8_t dataTx[] = {0x01,0x02,0x03,0x04,0x05};
	uint8_t dataRx[5] = {0};
	struct spi_transfer t[] = {
			{
				.tx_buf = dataTx,
				.rx_buf = dataRx,
				.len    = 5,
			},
		};
	spi_sync_transfer(spi, t, ARRAY_SIZE(t));

/* *************************************************************** */
	return 0;
}


static const struct spi_device_id a131m04_id[] = {
	{ "my-adc",  A131M04_ID},
	{ }
};
MODULE_DEVICE_TABLE(spi, a131m04_id);

static const struct of_device_id a131m04_of_table[] = {
	{ .compatible = "my-adc" },
	{ },
};
MODULE_DEVICE_TABLE(of, a131m04_of_table);

static struct spi_driver a131m04_driver = {
	.driver = {
		.name	= "my-adc",
		.of_match_table = a131m04_of_table,
	},
	.probe		= probe,
	.id_table	= a131m04_id,
};
module_spi_driver(a131m04_driver);

MODULE_LICENSE("GPL");
