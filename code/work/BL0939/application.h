
#ifndef ADC_H_
#define ADC_H_

// Standard libraries
// #include <assert.h>
// #include <stdint.h>
// #include <stdbool.h>

// Custom libraries
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

#include "driver.h"

uint32_t bl0939_read_reg(struct iio_dev *indio_dev, uint8_t reg);
int bl0939_write_reg(struct iio_dev *indio_dev, uint8_t reg, uint32_t val, int check);
void bl0939_spi_reset(struct iio_dev *indio_dev);
void bl0939_reset(struct iio_dev *indio_dev);
uint32_t bl0939_get_current_A(struct iio_dev *indio_dev);
uint32_t bl0939_get_current_B(struct iio_dev *indio_dev);
uint32_t bl0939_get_voltage(struct iio_dev *indio_dev);

#endif
