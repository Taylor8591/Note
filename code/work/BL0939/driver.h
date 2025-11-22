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
#define BL093908_CMD_NOP	0x00
#define BL093908_CMD_WAKEUP	0x02
#define BL093908_CMD_PWRDWN	0x04
#define BL093908_CMD_RESET	0x06
#define BL093908_CMD_START	0x08
#define BL093908_CMD_STOP	0x0a
#define BL093908_CMD_SYOCAL	0x16
#define BL093908_CMD_SYGCAL	0x17
#define BL093908_CMD_SFOCAL	0x19
#define BL093908_CMD_RDATA	0x12
#define BL093908_CMD_RREG	0x20
#define BL093908_CMD_WREG	0x40

/* Registers */
#define BL093908_ID_REG	0x00
#define BL093908_STATUS	0x01
#define BL093908_INPUT_MUX	0x02
#define BL093908_PGA		0x03
#define BL093908_DATA_RATE	0x04
#define BL093908_REF		0x05
#define BL093908_IDACMAG	0x06
#define BL093908_IDACMUX	0x07
#define BL093908_VBIAS		0x08
#define BL093908_SYS		0x09
#define BL093908_OFCAL0	0x0a
#define BL093908_OFCAL1	0x0b
#define BL093908_OFCAL2	0x0c
#define BL093908_FSCAL0	0x0d
#define BL093908_FSCAL1	0x0e
#define BL093908_FSCAL2	0x0f
#define BL093908_GPIODAT	0x10
#define BL093908_GPIOCON	0x11

/* BL09390x common channels */
#define BL093908_AIN0		0x00
#define BL093908_AIN1		0x01
#define BL093908_AIN2		0x02
#define BL093908_AIN3		0x03
#define BL093908_AIN4		0x04
#define BL093908_AIN5		0x05
#define BL093908_AINCOM	0x0c
/* BL093908 only channels */
#define BL093908_AIN6		0x06
#define BL093908_AIN7		0x07
#define BL093908_AIN8		0x08
#define BL093908_AIN9		0x09
#define BL093908_AIN10		0x0a
#define BL093908_AIN11		0x0b
#define BL093908_MAX_CHANNELS	12

#define BL093908_POS_MUX_SHIFT	0x04
#define BL093908_INT_REF		0x09

#define BL093908_START_REG_MASK	0x1f
#define BL093908_NUM_BYTES_MASK	0x1f

#define BL093908_START_CONV	0x01
#define BL093908_STOP_CONV	0x00

enum bl0939_id {
	BL0939_ID,
};

struct bl0939_chip_info {
	const struct iio_chan_spec *channels;
	unsigned int num_channels;
};

#define BL0939_CHAN(index)								\
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

static const struct iio_chan_spec bl0939_channels[] = {
	BL0939_CHAN(0),
	BL0939_CHAN(1),
	BL0939_CHAN(2),
};


struct bl0939_private {
	const struct bl0939_chip_info	*chip_info;
	struct spi_device *spi;
	struct mutex lock;

	u32 buffer[BL093908_MAX_CHANNELS + sizeof(s64)/sizeof(u32)] __aligned(8);
	u8 data[50] __aligned(IIO_DMA_MINALIGN);
};

static const struct bl0939_chip_info bl0939_chip_info_tbl[] = {
	[BL0939_ID] = {
		.channels = bl0939_channels,
		.num_channels = ARRAY_SIZE(bl0939_channels),
	},

};


#endif