
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include <linux/gpio/consumer.h>
#include <linux/spi/spi.h>

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/sysfs.h>

#include <asm/unaligned.h>

#include "application.h"
#include "driver.h"

uint32_t bl0939_read_reg(struct iio_dev *indio_dev, uint8_t reg) {

    struct bl0939_private *priv = iio_priv(indio_dev);

    uint8_t tx[6] = {0x55, reg, 0, 0, 0, 0};
    uint8_t rx[6] = {0, 0, 0, 0, 0, 0};

    struct spi_transfer t[] = {
		{
			.tx_buf = tx,
            .rx_buf = rx,
			.len = 6,
		},
	};
	spi_sync_transfer(priv->spi, t, 1);
    // printk(KERN_EMERG "tx data: %02x %02x %02x %02x %02x %02x\n", tx[0], tx[1], tx[2], tx[3], tx[4], tx[5]);
    // printk(KERN_EMERG "rx data: %02x %02x %02x %02x %02x %02x\n", rx[0], rx[1], rx[2], rx[3], rx[4], rx[5]);

    uint32_t val = 0;
    val = (uint32_t)rx[2] << 16 | (uint32_t)rx[3] << 8 | (uint32_t)rx[4] << 0;
    return val;
}
int bl0939_write_reg(struct iio_dev *indio_dev, uint8_t reg, uint32_t val, int check) {
    
    struct bl0939_private *priv = iio_priv(indio_dev);
    
    static uint32_t r_temp = 0;
    uint8_t h = val >> 16;
    uint8_t m = val >> 8;
    uint8_t l = val >> 0;
    uint8_t tx[6] = {0xA5, reg, h, m, l, ~(0XA5 + reg + h + m + l)};

    struct spi_transfer t[] = {
		{
			.tx_buf = tx,
			.len = ARRAY_SIZE(tx),
		},
	};
	spi_sync_transfer(priv->spi, t, 1);

    if (0 == check)
        return 0;
    r_temp = bl0939_read_reg(indio_dev, reg);
    // printk(KERN_EMERG "read back reg 0x%02x: %x\n", reg, r_temp);
    if (r_temp == val) {
        printk(KERN_EMERG"success\n");
        return 0;
    }
    printk(KERN_EMERG"error\n");
    return 1;
}
void bl0939_spi_reset(struct iio_dev *indio_dev) {
    struct bl0939_private *priv = iio_priv(indio_dev);

    uint8_t tx[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct spi_transfer t[] = {
        {
            .tx_buf = tx,
            .len = ARRAY_SIZE(tx),
        },
    };
    spi_sync_transfer(priv->spi, t, 1);
}
void bl0939_reset(struct iio_dev *indio_dev) {
    bl0939_spi_reset(indio_dev);
    bl0939_write_reg(indio_dev, 0x19, 0x005a5a5a, 0);  //复位用户寄存器
    bl0939_write_reg(indio_dev, 0x1a, 0x00000055, 1);  //解除写保护
    bl0939_write_reg(indio_dev, 0x10, 0xffff, 0);  // Threshold A
    bl0939_write_reg(indio_dev, 0x1E, 0xffff, 1);  // Threshold B
    // B 通道漏电/过流报警输出指示管脚为 I_leak，无需配置即可直接输出。
    // A 通道漏电/过流报警输出指示引脚为 CF，需先设置 MODE[12]=1，再设置 TPS_CTRL[14]=1
    //高有效
    bl0939_write_reg(indio_dev, 0x18, 0x00002000, 1);  // cf
    bl0939_write_reg(indio_dev, 0x1B, 0x000047ff, 0);  // cf
    bl0939_write_reg(indio_dev, 0x1a, 0x00000000, 1);  //写保护
}
uint32_t bl0939_get_current_A(struct iio_dev *indio_dev) {
    uint32_t Ia = bl0939_read_reg(indio_dev, 0x00);
    return Ia;
}
uint32_t bl0939_get_current_B(struct iio_dev *indio_dev) {
    uint32_t Ib = bl0939_read_reg(indio_dev, 0x07);
    return Ib;
}
uint32_t bl0939_get_voltage(struct iio_dev *indio_dev) {
    uint32_t v = bl0939_read_reg(indio_dev, 0x06);
    return v;
}







