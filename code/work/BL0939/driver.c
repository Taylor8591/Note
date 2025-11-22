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

#include "driver.h"
#include "application.h"

// static struct work_struct my_work;


struct wait_queue_head wait_queue;


static int bl0939_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long m)
{
	struct bl0939_private *priv = iio_priv(indio_dev);//返回指向该 IIO 设备私有数据的指针
	int ret = 0;
	mutex_lock(&priv->lock);

	switch (m) {
	case IIO_CHAN_INFO_RAW:
		
		if (ret) {
			dev_err(&priv->spi->dev, "Set ADC CH failed\n");
			goto out;
		}

		ret = IIO_VAL_INT;
		break;
	default:
		ret = -EINVAL;
		break;
	}
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static const struct iio_info bl0939_info = {
	.read_raw = &bl0939_read_raw,
};

static int bl0939_probe(struct spi_device *spi)
{
	printk(KERN_EMERG "BL0939_probe\n");

	u32 mode = SPI_MODE_1; // 默认 SPI 模式 1 (CPOL=0, CPHA=1)
    u32 speed = 800000;   // 设置 SPI 最大速度为 8 MHz
    u8 bits = 8;           // 8 位数据位宽

    // 设置 SPI 模式，SPI 模式是通过 CPOL 和 CPHA 来控制的
    mode |= SPI_CPHA;     // 设置 CPHA = 1 (数据在时钟下降沿采样)
    mode &= ~SPI_CPOL;    // 设置 CPOL = 0 (空闲时 SCLK 低电平)

    // 配置 SPI 设备
    spi->mode = mode;     // 设置 SPI 工作模式（CPOL 和 CPHA）
    spi->max_speed_hz = speed; // 设置最大传输频率
    spi->bits_per_word = bits; // 设置每个字的位宽

    // 执行 SPI 设置
    spi_setup(spi); // 进行配置更新

	struct bl0939_private *bl0939_priv;
	struct iio_dev *indio_dev;
	const struct spi_device_id *spi_id = spi_get_device_id(spi);//获取 spi 设备 ID
	int ret;
	

	//分配 iio 设备
	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*bl0939_priv));
	if (indio_dev == NULL) {
		return -ENOMEM;
	}
	else {
		printk(KERN_EMERG "devm_iio_device_alloc success\n");
	}
	bl0939_priv = iio_priv(indio_dev);//获取指向该 IIO 设备私有数据的指针

	//注册iio
	bl0939_priv->chip_info = &bl0939_chip_info_tbl[spi_id->driver_data];
	bl0939_priv->spi = spi;
	indio_dev->name = spi_id->name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = bl0939_priv->chip_info->channels;
	indio_dev->num_channels = bl0939_priv->chip_info->num_channels;
	indio_dev->info = &bl0939_info;

	ret = devm_iio_device_register(&spi->dev, indio_dev);
	if (ret) {
		dev_err(&spi->dev, "iio device register failed\n");
		return ret;
	} else {
		printk(KERN_EMERG "iio device register success\n");
	}


	init_waitqueue_head(&wait_queue);


	int m = 0;
	while(1) {
		m++;
		if (m > 1000) break;

		uint32_t current_A = bl0939_get_current_A(indio_dev);
		uint32_t current_B = bl0939_get_current_B(indio_dev);
		uint32_t voltage = bl0939_get_voltage(indio_dev);
		printk(KERN_EMERG "current_A: %u, current_B: %u, voltage: %u\n", current_A, current_B, voltage);
		mdelay(100);
	}	

	return 0;
}





















static const struct spi_device_id bl0939_id[] = {
	{ "bl0939",  BL0939_ID},
	{ }
};
MODULE_DEVICE_TABLE(spi, bl0939_id);

static const struct of_device_id bl0939_of_table[] = {
	{ .compatible = "bl0939" },
	{ .compatible = "bl0939" },
	{ },
};
MODULE_DEVICE_TABLE(of, bl0939_of_table);

static struct spi_driver bl0939_driver = {
	.driver = {
		.name	= "bl0939",
		.of_match_table = bl0939_of_table,
	},
	.probe		= bl0939_probe,
	.id_table	= bl0939_id,
};
module_spi_driver(bl0939_driver);


MODULE_LICENSE("GPL");
