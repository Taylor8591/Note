#ifndef DRIVER_H
#define DRIVER_H
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

/* Commands */
#define A131M0408_CMD_NOP	0x00
#define A131M0408_CMD_WAKEUP	0x02
#define A131M0408_CMD_PWRDWN	0x04
#define A131M0408_CMD_RESET	0x06
#define A131M0408_CMD_START	0x08
#define A131M0408_CMD_STOP	0x0a
#define A131M0408_CMD_SYOCAL	0x16
#define A131M0408_CMD_SYGCAL	0x17
#define A131M0408_CMD_SFOCAL	0x19
#define A131M0408_CMD_RDATA	0x12
#define A131M0408_CMD_RREG	0x20
#define A131M0408_CMD_WREG	0x40

/* Registers */
#define A131M0408_ID_REG	0x00
#define A131M0408_STATUS	0x01
#define A131M0408_INPUT_MUX	0x02
#define A131M0408_PGA		0x03
#define A131M0408_DATA_RATE	0x04
#define A131M0408_REF		0x05
#define A131M0408_IDACMAG	0x06
#define A131M0408_IDACMUX	0x07
#define A131M0408_VBIAS		0x08
#define A131M0408_SYS		0x09
#define A131M0408_OFCAL0	0x0a
#define A131M0408_OFCAL1	0x0b
#define A131M0408_OFCAL2	0x0c
#define A131M0408_FSCAL0	0x0d
#define A131M0408_FSCAL1	0x0e
#define A131M0408_FSCAL2	0x0f
#define A131M0408_GPIODAT	0x10
#define A131M0408_GPIOCON	0x11

/* A131M040x common channels */
#define A131M0408_AIN0		0x00
#define A131M0408_AIN1		0x01
#define A131M0408_AIN2		0x02
#define A131M0408_AIN3		0x03
#define A131M0408_AIN4		0x04
#define A131M0408_AIN5		0x05
#define A131M0408_AINCOM	0x0c
/* A131M0408 only channels */
#define A131M0408_AIN6		0x06
#define A131M0408_AIN7		0x07
#define A131M0408_AIN8		0x08
#define A131M0408_AIN9		0x09
#define A131M0408_AIN10		0x0a
#define A131M0408_AIN11		0x0b
#define A131M0408_MAX_CHANNELS	12

#define A131M0408_POS_MUX_SHIFT	0x04
#define A131M0408_INT_REF		0x09

#define A131M0408_START_REG_MASK	0x1f
#define A131M0408_NUM_BYTES_MASK	0x1f

#define A131M0408_START_CONV	0x01
#define A131M0408_STOP_CONV	0x00

enum a131m04_id {
	A131M04_ID,
	A131M0406_ID,
};

struct a131m04_chip_info {
	const struct iio_chan_spec *channels;
	unsigned int num_channels;
};

#define A131M04_CHAN(index)								\
{														\
	.type = IIO_CURRENT,								\
	.indexed = 1,										\
	.channel = index,									\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.scan_index = index,								\
	.scan_type = {										\
		.sign = 'u',									\
		.realbits = 32,									\
		.storagebits = 32,								\
	},													\
}

static const struct iio_chan_spec a131m04_channels[] = {
	A131M04_CHAN(0),
	A131M04_CHAN(1),
	A131M04_CHAN(2),
	A131M04_CHAN(3),
};


struct a131m04_private {
	const struct a131m04_chip_info	*chip_info;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *cs_gpio;
	struct gpio_desc *interrupt_gpio;
	struct spi_device *spi;
	struct mutex lock;
	/*
	 * Used to correctly align data.
	 * Ensure timestamp is naturally aligned.
	 * Note that the full buffer length may not be needed if not
	 * all channels are enabled, as long as the alignment of the
	 * timestamp is maintained.
	 */
	u32 buffer[A131M0408_MAX_CHANNELS + sizeof(s64)/sizeof(u32)] __aligned(8);
	u8 data[50] __aligned(IIO_DMA_MINALIGN);
};

static const struct a131m04_chip_info a131m04_chip_info_tbl[] = {
	[A131M04_ID] = {
		.channels = a131m04_channels,
		.num_channels = ARRAY_SIZE(a131m04_channels),
	},

};

void spiSendReceiveArrays(struct iio_dev *indio_dev, const uint8_t *dataTx, uint8_t *dataRx, const uint8_t byteLength);
void toggleRESET(struct iio_dev *indio_dev);
void toggleSYNC(struct iio_dev *indio_dev);
void setSYNC_RESET(struct iio_dev *indio_dev, const bool state);
// void setCS(struct iio_dev *indio_dev, const bool state);


#endif