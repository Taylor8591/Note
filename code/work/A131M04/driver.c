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
#include "adc.h"
#include "driver.h"


static struct work_struct my_work;
struct iio_dev *indio_dev;
adc_channel_data DataStruct;
struct wait_queue_head wait_queue;




void spiSendReceiveArrays(struct iio_dev *indio_dev, const uint8_t *dataTx, uint8_t *dataRx, const uint8_t byteLength) {
	
	struct a131m04_private *priv = iio_priv(indio_dev);
	struct spi_transfer t[] = {
		{
			.tx_buf = dataTx,
			.rx_buf = dataRx,
			.len    = byteLength,
			.cs_change = 1,
		},
	};
    spi_sync_transfer(priv->spi, t, 1);
}
void toggleRESET(struct iio_dev *indio_dev) {
	
	struct a131m04_private *priv = iio_priv(indio_dev);

	gpiod_set_value(priv->reset_gpio, 0);
	mdelay(2);
	gpiod_set_value(priv->reset_gpio, 1);
	udelay(5);
	restoreRegisterDefaults();
	writeSingleRegister(indio_dev, MODE_ADDRESS, MODE_DEFAULT);
}
void toggleSYNC(struct iio_dev *indio_dev) {
	struct a131m04_private *priv = iio_priv(indio_dev);

    gpiod_set_value(priv->reset_gpio, 0);
    udelay(2);
    gpiod_set_value(priv->reset_gpio, 1);
}
void setSYNC_RESET(struct iio_dev *indio_dev, const bool state) {
	struct a131m04_private *priv = iio_priv(indio_dev);

    gpiod_set_value(priv->reset_gpio, state ? HIGH : LOW);
}
void setCS(struct iio_dev *indio_dev, const bool state) {
	struct a131m04_private *priv = iio_priv(indio_dev);

	gpiod_set_value(priv->cs_gpio, state ? 1 : 0);
}
void long_task_work(struct work_struct *work) {
	readData(indio_dev, &DataStruct);
	printk(KERN_EMERG "%d,%d,%d,%d\n", DataStruct.channel0, DataStruct.channel1, DataStruct.channel2, DataStruct.channel3);
}
irqreturn_t test_interrupt(int irq, void *dev_id) {

	// printk(KERN_EMERG "interrupt\n");
	schedule_work(&my_work); 
	// iio_trigger_poll(my_trigger);
    return IRQ_HANDLED;
}
int a131m04_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long m)
{
	struct a131m04_private *priv = iio_priv(indio_dev);//返回指向该 IIO 设备私有数据的指针
	int ret = 0;
	int channel = chan->channel;
	mutex_lock(&priv->lock);

	switch (m) {
	case IIO_CHAN_INFO_RAW:
		switch(channel) {
			case 0:*val = DataStruct.channel0;break;
			case 1:*val = DataStruct.channel1;break;
			case 2:*val = DataStruct.channel2;break;
			case 3:*val = DataStruct.channel3;break;
		}
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
static const struct iio_info a131m04_info = {
	.read_raw = &a131m04_read_raw,
};

//********************************************************************************************************************************************************************* */
static int a131m04_probe(struct spi_device *spi)
{
	printk(KERN_EMERG "a131m04_probe\n");

	struct a131m04_private *a131m04_priv;
	const struct spi_device_id *spi_id = spi_get_device_id(spi);//获取 spi 设备 ID
	int ret;

	u32 mode = SPI_MODE_1;    // 设置为 SPI 模式 0 (CPOL=0, CPHA=0)
	u32 speed = 800000;       // 设置 SPI 最大速度为 800 kHz
	u8 bits = 8;              // 8 位数据位宽


	//配置 SPI 设备
	spi->mode = mode;         // 设置 SPI 工作模式
	spi->max_speed_hz = speed; // 设置最大传输频率
	spi->bits_per_word = bits; // 设置每个字的位宽
    spi_setup(spi); // 进行配置更新

	
	//分配 iio 设备
	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*a131m04_priv));
	if (indio_dev == NULL)
		return -ENOMEM;
	printk(KERN_EMERG "devm_iio_device_alloc success\n");
	a131m04_priv = iio_priv(indio_dev);//获取指向该 IIO 设备私有数据的指针



	//获取复位 GPIO
	a131m04_priv->reset_gpio = devm_gpiod_get_optional(&spi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(a131m04_priv->reset_gpio))
		dev_info(&spi->dev, "Reset GPIO not defined\n");
	else
		printk(KERN_EMERG "Reset GPIO defined\n");


	//获取中断 GPIO
	a131m04_priv->interrupt_gpio = devm_gpiod_get_optional(&spi->dev, "interrupt", GPIOD_IN);
	if (IS_ERR(a131m04_priv->interrupt_gpio))
		dev_info(&spi->dev, "Interrupt GPIO not defined\n");
	else
		printk(KERN_EMERG "Interrupt GPIO defined\n");
	int irq = gpiod_to_irq(a131m04_priv->interrupt_gpio);//将 GPIO 描述符转换为中断号
	printk(KERN_EMERG "adc irq = %d\n", irq); 


	//注册中断
	ret = devm_request_irq(&spi->dev, irq, test_interrupt, IRQF_TRIGGER_FALLING, "my_gpio_handler", spi);
    if(ret) {
        printk(KERN_EMERG "request irq failed\n");
        return -1;
    }
	INIT_WORK(&my_work, long_task_work);
    printk(KERN_EMERG "request irq success\n");


	//注册iio
	a131m04_priv->chip_info = &a131m04_chip_info_tbl[spi_id->driver_data];
	a131m04_priv->spi = spi;
	indio_dev->name = spi_id->name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = a131m04_priv->chip_info->channels;
	indio_dev->num_channels = a131m04_priv->chip_info->num_channels;
	indio_dev->info = &a131m04_info;

	ret = devm_iio_device_register(&spi->dev, indio_dev);
	if (ret) {
		dev_err(&spi->dev, "iio device register failed\n");
		return ret;
	} else {
		printk(KERN_EMERG "iio device register success\n");
	}


	init_waitqueue_head(&wait_queue);
	adcStartup(indio_dev);


	// int m = 0;
	// mdelay(1000);
	// while(1) {
	// 	m++;
	// 	if (m > 100) break;
	// 	// spiSendReceiveByte(indio_dev, 0xF0);
	// 	readData(indio_dev, &DataStruct);
	// 	mdelay(100);

		
	// }	

	return 0;
}





















static const struct spi_device_id a131m04_id[] = {
	{ "my-adc",  A131M04_ID},
	{ }
};
MODULE_DEVICE_TABLE(spi, a131m04_id);

static const struct of_device_id a131m04_of_table[] = {
	{ .compatible = "my-adc" },
	{ .compatible = "my-adc" },
	{ },
};
MODULE_DEVICE_TABLE(of, a131m04_of_table);

static struct spi_driver a131m04_driver = {
	.driver = {
		.name	= "my-adc",
		.of_match_table = a131m04_of_table,
	},
	.probe		= a131m04_probe,
	.id_table	= a131m04_id,
};
module_spi_driver(a131m04_driver);

MODULE_LICENSE("GPL");
