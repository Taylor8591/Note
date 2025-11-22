// SPDX-License-Identifier: GPL-2.0
/*
 * max96724 driver
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 * v0.1.0x00 : 1. create file.
 * V0.0X01.0X02 fix mclk issue when probe multiple camera.
 * V0.0X01.0X03 add enum_frame_interval function.
 * V0.0X01.0X04 add quick stream on/off
 * V0.0X01.0X05 add function g_mbus_config
 * V0.0X01.0X06
 * 1. fix g_mbus_config lane config issues.
 * 2. and add debug info
 * 3. add r1a version support
  * V0.0X01.0X07  modify otp mode : sensor or rockchip
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/rk-camera-module.h>
#include <linux/regmap.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-fwnode.h>
#include <linux/pinctrl/consumer.h>
#include <linux/rk-preisp.h>
#include "../platform/rockchip/isp/rkisp_tb_helper.h"
#include <linux/of_graph.h>
#include "otp_eeprom.h"

#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x07)

#ifndef V4L2_CID_DIGITAL_GAIN
#define V4L2_CID_DIGITAL_GAIN		V4L2_CID_GAIN
#endif
#define MAX96724_PIXEL_RATE		(360000000LL * 2LL * 2LL / 10LL)

#define MIPI_FREQ			360000000U
#define MAX96724_XVCLK_FREQ		24000000

#define CHIP_ID				0xa2
#define MAX96724_REG_CHIP_ID		0xd

#define MAX96724_REG_CTRL_MODE		0x0100
#define MAX96724_MODE_SW_STANDBY		0x0
#define MAX96724_MODE_STREAMING		0x1

#define MAX96724_REG_EXPOSURE		0x3500
#define	MAX96724_EXPOSURE_MIN		4
#define	MAX96724_EXPOSURE_STEP		1
#define MAX96724_VTS_MAX			0x7fff

#define MAX96724_REG_GAIN_H		0x3508
#define MAX96724_REG_GAIN_L		0x3509
#define MAX96724_GAIN_H_MASK		0x07
#define MAX96724_GAIN_H_SHIFT		8
#define MAX96724_GAIN_L_MASK		0xff
#define MAX96724_GAIN_MIN			0x80
#define MAX96724_GAIN_MAX			0x7ff
#define MAX96724_GAIN_STEP		1
#define MAX96724_GAIN_DEFAULT		0x80

#define MAX96724_REG_TEST_PATTERN		0x5e00
#define	MAX96724_TEST_PATTERN_ENABLE	0x80
#define	MAX96724_TEST_PATTERN_DISABLE	0x0

#define MAX96724_REG_VTS			0x380e

#define REG_NULL			0xFFFF

#define MAX96724_REG_VALUE_08BIT		1
#define MAX96724_REG_VALUE_16BIT		2
#define MAX96724_REG_VALUE_24BIT		3

#define MAX96724_LANES			2
#define MAX96724_BITS_PER_SAMPLE		10

#define MAX96724_CHIP_REVISION_REG	0x302A

#define OF_CAMERA_PINCTRL_STATE_DEFAULT	"rockchip,camera_default"
#define OF_CAMERA_PINCTRL_STATE_SLEEP	"rockchip,camera_sleep"
#define OF_CAMERA_CAPTURE_MODE   "rockchip,max96724-capture-mode"

#define MAX96724_NAME			"max96724"
#define MAX96724_MEDIA_BUS_FMT		MEDIA_BUS_FMT_SBGGR10_1X10


#define max96724_write_1byte(client, reg, val)	\
	max96724_write_reg((client), (reg), MAX96724_REG_VALUE_08BIT, (val))

#define max96724_read_1byte(client, reg, val)	\
	max96724_read_reg((client), (reg), MAX96724_REG_VALUE_08BIT, (val))

static const struct regval *max96724_global_regs;


static const char * const max96724_supply_names[] = {
	"avdd",		/* Analog power */
	"dovdd",	/* Digital I/O power */
	"dvdd",		/* Digital core power */
};

#define MAX96724_NUM_SUPPLIES ARRAY_SIZE(max96724_supply_names)

struct regval {
	u16 addr;
	u8 val;
};

struct max96724_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	const struct regval *reg_list;
};

struct max96724 {
	struct i2c_client	*client;
	struct clk		*xvclk;
	struct gpio_desc	*power_gpio;
	struct gpio_desc	*reset_gpio;
	struct gpio_desc	*pwdn_gpio;
	struct regulator_bulk_data supplies[MAX96724_NUM_SUPPLIES];

	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_sleep;

	struct v4l2_subdev	subdev;
	struct media_pad	pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl	*exposure;
	struct v4l2_ctrl	*anal_gain;
	struct v4l2_ctrl	*digi_gain;
	struct v4l2_ctrl	*hblank;
	struct v4l2_ctrl	*vblank;
	struct v4l2_ctrl	*test_pattern;
	struct mutex		mutex;
	bool			streaming;
	const struct max96724_mode *cur_mode;
	bool			is_r2a;
	unsigned int		lane_num;
	unsigned int		cfg_num;
	unsigned int		pixel_rate;
	bool			power_on;	
	u32			module_index;
	const char		*module_name;
	const char		*len_name;
	struct rkmodule_inf	module_inf;
	struct rkmodule_awb_cfg	awb_cfg;
	struct rkmodule_lsc_cfg	lsc_cfg;
	u32 capture_mode;
	struct regmap *regmap;
};

#define to_max96724(sd) container_of(sd, struct max96724, subdev)

static const struct regmap_config max96724_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0x1F17,
};

static const struct regval max96724_global_regs_r2a_4lane[] = { // 上电初始化寄存器
	
	{0x0103, 0x01},
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x0302, 0x1e},
	{0x0303, 0x00},
	{0x0304, 0x03},
	
	{REG_NULL, 0x00},
};

static const struct regval max96724_3264x2448_regs_r2a_4lane[] = { // start_stream写的寄存器
	{0x0100, 0x00},
	{0x3501, 0x9a},
	{0x3502, 0x20},
	{0x3508, 0x02},
	
	{REG_NULL, 0x00},
};

static const struct max96724_mode supported_modes_r2a_4lane[] = {
	{
		.width = 3264,
		.height = 2448,
		.max_fps = {
			.numerator = 10000,
			.denominator = 300000,
		},
		.exp_def = 0x09a0,
		.hts_def = 0x0794 * 2,
		.vts_def = 0x0a00,
		.reg_list = max96724_3264x2448_regs_r2a_4lane,
	},
};

static const struct max96724_mode *supported_modes;

static const s64 link_freq_menu_items[] = {
	MIPI_FREQ
};

/* Write registers up to 4 at a time */
static int max96724_write_reg(struct i2c_client *client, u16 reg,
			    u32 len, u32 val)
{
	u32 buf_i, val_i;
	u8 buf[6];
	u8 *val_p;
	__be32 val_be;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	val_be = cpu_to_be32(val);
	val_p = (u8 *)&val_be;
	buf_i = 2;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

static int max96724_write_array(struct i2c_client *client,
			      const struct regval *regs)
{
	u32 i;
	int ret = 0;

	for (i = 0; ret == 0 && regs[i].addr != REG_NULL; i++)
		ret = max96724_write_reg(client, regs[i].addr,
					MAX96724_REG_VALUE_08BIT,
					regs[i].val);

	return ret;
}

/* Read registers up to 4 at a time */
static int max96724_read_reg(struct i2c_client *client, u16 reg,
			   unsigned int len, u32 *val)
{
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	__be32 data_be = 0;
	__be16 reg_addr_be = cpu_to_be16(reg);
	int ret;

	if (len > 4 || !len)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);

	return 0;
}

static int max96724_get_reso_dist(const struct max96724_mode *mode,
				struct v4l2_mbus_framefmt *framefmt)
{
	return abs(mode->width - framefmt->width) +
	       abs(mode->height - framefmt->height);
}

static const struct max96724_mode *
max96724_find_best_fit(struct max96724 *max96724,
		     struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt = &fmt->format;
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	unsigned int i;

	for (i = 0; i < max96724->cfg_num; i++) {
		dist = max96724_get_reso_dist(&supported_modes[i], framefmt);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}

	return &supported_modes[cur_best_fit];
}

static int max96724_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *fmt)
{
	struct max96724 *max96724 = to_max96724(sd);
	const struct max96724_mode *mode;
	s64 h_blank, vblank_def;

	mutex_lock(&max96724->mutex);

	mode = max96724_find_best_fit(max96724, fmt);
	fmt->format.code = MAX96724_MEDIA_BUS_FMT;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		*v4l2_subdev_get_try_format(sd, sd_state, fmt->pad) = fmt->format;
#else
		mutex_unlock(&max96724->mutex);
		return -ENOTTY;
#endif
	} else {
		max96724->cur_mode = mode;
		h_blank = mode->hts_def - mode->width;
		__v4l2_ctrl_modify_range(max96724->hblank, h_blank,
					 h_blank, 1, h_blank);
		vblank_def = mode->vts_def - mode->height;
		__v4l2_ctrl_modify_range(max96724->vblank, vblank_def,
					 MAX96724_VTS_MAX - mode->height,
					 1, vblank_def);
	}

	mutex_unlock(&max96724->mutex);

	return 0;
}

static int max96724_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *fmt)
{
	struct max96724 *max96724 = to_max96724(sd);
	const struct max96724_mode *mode = max96724->cur_mode;

	mutex_lock(&max96724->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		fmt->format = *v4l2_subdev_get_try_format(sd, sd_state, fmt->pad);
#else
		mutex_unlock(&max96724->mutex);
		return -ENOTTY;
#endif
	} else {
		fmt->format.width = mode->width;
		fmt->format.height = mode->height;
		fmt->format.code = MAX96724_MEDIA_BUS_FMT;
		fmt->format.field = V4L2_FIELD_NONE;
	}
	mutex_unlock(&max96724->mutex);

	return 0;
}

static int max96724_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index != 0)
		return -EINVAL;
	code->code = MAX96724_MEDIA_BUS_FMT;

	return 0;
}

static int max96724_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *sd_state,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	struct max96724 *max96724 = to_max96724(sd);

	if (fse->index >= max96724->cfg_num)
		return -EINVAL;

	if (fse->code != MAX96724_MEDIA_BUS_FMT)
		return -EINVAL;

	fse->min_width  = supported_modes[fse->index].width;
	fse->max_width  = supported_modes[fse->index].width;
	fse->max_height = supported_modes[fse->index].height;
	fse->min_height = supported_modes[fse->index].height;

	return 0;
}

static int max96724_enable_test_pattern(struct max96724 *max96724, u32 pattern)
{
	u32 val;

	if (pattern)
		val = (pattern - 1) | MAX96724_TEST_PATTERN_ENABLE;
	else
		val = MAX96724_TEST_PATTERN_DISABLE;

	return max96724_write_reg(max96724->client,
				 MAX96724_REG_TEST_PATTERN,
				 MAX96724_REG_VALUE_08BIT,
				 val);
}

static int max96724_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct max96724 *max96724 = to_max96724(sd);
	const struct max96724_mode *mode = max96724->cur_mode;

	fi->interval = mode->max_fps;

	return 0;
}


static void max96724_get_module_inf(struct max96724 *max96724,
				  struct rkmodule_inf *inf)
{


	strlcpy(inf->base.sensor, MAX96724_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, max96724->module_name, sizeof(inf->base.module));
	strlcpy(inf->base.lens, max96724->len_name, sizeof(inf->base.lens));		
}

static void max96724_set_awb_cfg(struct max96724 *max96724,
			       struct rkmodule_awb_cfg *cfg)
{
	mutex_lock(&max96724->mutex);
	memcpy(&max96724->awb_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&max96724->mutex);
}

static void max96724_set_lsc_cfg(struct max96724 *max96724,
			       struct rkmodule_lsc_cfg *cfg)
{
	mutex_lock(&max96724->mutex);
	memcpy(&max96724->lsc_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&max96724->mutex);
}

static long max96724_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct max96724 *max96724 = to_max96724(sd);
	long ret = 0;
	u32 stream = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		max96724_get_module_inf(max96724, (struct rkmodule_inf *)arg);
		break;
	case RKMODULE_AWB_CFG:
		max96724_set_awb_cfg(max96724, (struct rkmodule_awb_cfg *)arg);
		break;
	case RKMODULE_LSC_CFG:
		max96724_set_lsc_cfg(max96724, (struct rkmodule_lsc_cfg *)arg);
		break;
	case RKMODULE_SET_QUICK_STREAM:

		stream = *((u32 *)arg);

		if (stream)
			ret = max96724_write_reg(max96724->client,
				MAX96724_REG_CTRL_MODE,
				MAX96724_REG_VALUE_08BIT,
				MAX96724_MODE_STREAMING);
		else
			ret = max96724_write_reg(max96724->client,
				MAX96724_REG_CTRL_MODE,
				MAX96724_REG_VALUE_08BIT,
				MAX96724_MODE_SW_STANDBY);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long max96724_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct rkmodule_inf *inf;
	struct rkmodule_awb_cfg *awb_cfg;
	struct rkmodule_lsc_cfg *lsc_cfg;
	long ret = 0;
	u32 stream = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = max96724_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case RKMODULE_AWB_CFG:
		awb_cfg = kzalloc(sizeof(*awb_cfg), GFP_KERNEL);
		if (!awb_cfg) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(awb_cfg, up, sizeof(*awb_cfg));
		if (!ret)
			ret = max96724_ioctl(sd, cmd, awb_cfg);
		kfree(awb_cfg);
		break;
	case RKMODULE_LSC_CFG:
		lsc_cfg = kzalloc(sizeof(*lsc_cfg), GFP_KERNEL);
		if (!lsc_cfg) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(lsc_cfg, up, sizeof(*lsc_cfg));
		if (!ret)
			ret = max96724_ioctl(sd, cmd, lsc_cfg);
		kfree(lsc_cfg);
		break;
	case RKMODULE_SET_QUICK_STREAM:
		ret = copy_from_user(&stream, up, sizeof(u32));
		if (!ret)
			ret = max96724_ioctl(sd, cmd, &stream);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}
#endif

static int __max96724_start_stream(struct max96724 *max96724)
{
	int ret;

	ret = max96724_write_array(max96724->client, max96724->cur_mode->reg_list);
	if (ret)
		return ret;

	/* In case these controls are set before streaming */
	// mutex_unlock(&max96724->mutex);
	// ret = v4l2_ctrl_handler_setup(&max96724->ctrl_handler);
	// mutex_lock(&max96724->mutex);
	// if (ret)
	// 	return ret;
	// return max96724_write_reg(max96724->client,
	// 			MAX96724_REG_CTRL_MODE,
	// 			MAX96724_REG_VALUE_08BIT,
	// 			MAX96724_MODE_STREAMING);
		return 0;
}

static int __max96724_stop_stream(struct max96724 *max96724)
{
	// return max96724_write_reg(max96724->client,
	// 			MAX96724_REG_CTRL_MODE,
	// 			MAX96724_REG_VALUE_08BIT,
	// 			MAX96724_MODE_SW_STANDBY);
		return 0;
}

static int max96724_s_stream(struct v4l2_subdev *sd, int on)
{
	struct max96724 *max96724 = to_max96724(sd);
	struct i2c_client *client = max96724->client;
	int ret = 0;

	dev_info(&client->dev, "%s: on: %d, %dx%d@%d\n", __func__, on,
				max96724->cur_mode->width,
				max96724->cur_mode->height,
		DIV_ROUND_CLOSEST(max96724->cur_mode->max_fps.denominator,
				  max96724->cur_mode->max_fps.numerator));

	mutex_lock(&max96724->mutex);
	on = !!on;
	if (on == max96724->streaming)
		goto unlock_and_return;

	if (on) {
		// ret = pm_runtime_get_sync(&client->dev);
		// if (ret < 0) {
		// 	pm_runtime_put_noidle(&client->dev);
		// 	goto unlock_and_return;
		// }

		ret = __max96724_start_stream(max96724);
		if (ret) {
			v4l2_err(sd, "start stream failed while write regs\n");
			// pm_runtime_put(&client->dev);
			goto unlock_and_return;
		}
	} else {
		__max96724_stop_stream(max96724);
		// pm_runtime_put(&client->dev);
	}

	max96724->streaming = on;

unlock_and_return:
	mutex_unlock(&max96724->mutex);

	return ret;
}

static int max96724_s_power(struct v4l2_subdev *sd, int on)
{
	struct max96724 *max96724 = to_max96724(sd);
	struct i2c_client *client = max96724->client;
	int ret = 0;

	mutex_lock(&max96724->mutex);

	dev_info(&client->dev, "s_power\n");

	/* If the power state is not modified - no work to do. */
	if (max96724->power_on == !!on)
		goto unlock_and_return;

	if (on) {
		// ret = pm_runtime_get_sync(&client->dev);
		// if (ret < 0) {
		// 	pm_runtime_put_noidle(&client->dev);
		// 	goto unlock_and_return;
		// }

		ret = max96724_write_array(max96724->client, max96724_global_regs);
		// if (ret) {
		// 	v4l2_err(sd, "could not set init registers\n");
		// 	pm_runtime_put_noidle(&client->dev);
		// 	goto unlock_and_return;
		// }

		max96724->power_on = true;
	} else {
		// pm_runtime_put(&client->dev);
		max96724->power_on = false;
	}

unlock_and_return:
	mutex_unlock(&max96724->mutex);

	return ret;
}

/* Calculate the delay in us by clock rate and clock cycles */
static inline u32 max96724_cal_delay(u32 cycles)
{
	return DIV_ROUND_UP(cycles, MAX96724_XVCLK_FREQ / 1000 / 1000);
}

static int __max96724_power_on(struct max96724 *max96724)
{
	// TODO: Implement power on for MAX96724
	return 0;
}

static void __max96724_power_off(struct max96724 *max96724)
{
	// int ret;
	// struct device *dev = &max96724->client->dev;

	// if (!IS_ERR(max96724->pwdn_gpio))
	// 	gpiod_set_value_cansleep(max96724->pwdn_gpio, 0);
	// clk_disable_unprepare(max96724->xvclk);
	// if (!IS_ERR(max96724->reset_gpio))
	// 	gpiod_set_value_cansleep(max96724->reset_gpio, 0);
	// if (!IS_ERR_OR_NULL(max96724->pins_sleep)) {
	// 	ret = pinctrl_select_state(max96724->pinctrl,
	// 				   max96724->pins_sleep);
	// 	if (ret < 0)
	// 		dev_dbg(dev, "could not set pins\n");
	// }

	// //if (!IS_ERR(max96724->power_gpio))
	// 	//gpiod_set_value_cansleep(max96724->power_gpio, 0);

	// regulator_bulk_disable(MAX96724_NUM_SUPPLIES, max96724->supplies);
}

static int __maybe_unused max96724_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct max96724 *max96724 = to_max96724(sd);

	return __max96724_power_on(max96724);
}

static int __maybe_unused max96724_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct max96724 *max96724 = to_max96724(sd);

	__max96724_power_off(max96724);

	return 0;
}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int max96724_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct max96724 *max96724 = to_max96724(sd);
	struct v4l2_mbus_framefmt *try_fmt =
				v4l2_subdev_get_try_format(sd, fh->state, 0);
	const struct max96724_mode *def_mode = &supported_modes[0];

	mutex_lock(&max96724->mutex);
	/* Initialize try_fmt */
	try_fmt->width = def_mode->width;
	try_fmt->height = def_mode->height;
	try_fmt->code = MAX96724_MEDIA_BUS_FMT;
	try_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&max96724->mutex);
	/* No crop or compose */

	return 0;
}
#endif

static int max96724_enum_frame_interval(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *sd_state,
				      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct max96724 *max96724 = to_max96724(sd);

	if (fie->index >= max96724->cfg_num)
		return -EINVAL;

	fie->code = MAX96724_MEDIA_BUS_FMT;
	fie->width = supported_modes[fie->index].width;
	fie->height = supported_modes[fie->index].height;
	fie->interval = supported_modes[fie->index].max_fps;
	return 0;
}

static int max96724_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad_id,
				struct v4l2_mbus_config *config)
{
	struct max96724  *sensor = to_max96724(sd);
	struct device *dev = &sensor->client->dev;

	dev_info(dev, "%s(%d) enter!\n", __func__, __LINE__);

	if (2 == sensor->lane_num || 4 == sensor->lane_num) {
		config->type = V4L2_MBUS_CSI2_DPHY;
		config->bus.mipi_csi2.num_data_lanes = sensor->lane_num;
	} else {
		dev_err(&sensor->client->dev,
			"unsupported lane_num(%d)\n", sensor->lane_num);
	}
	return 0;
}

static const struct dev_pm_ops max96724_pm_ops = {
	SET_RUNTIME_PM_OPS(max96724_runtime_suspend,
			   max96724_runtime_resume, NULL)
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_internal_ops max96724_internal_ops = {
	.open = max96724_open,
};
#endif

static const struct v4l2_subdev_core_ops max96724_core_ops = {
	.s_power = max96724_s_power,
	.ioctl = max96724_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = max96724_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops max96724_video_ops = {
	.s_stream = max96724_s_stream,
	.g_frame_interval = max96724_g_frame_interval,
};

static const struct v4l2_subdev_pad_ops max96724_pad_ops = {
	.enum_mbus_code = max96724_enum_mbus_code,
	.enum_frame_size = max96724_enum_frame_sizes,
	.enum_frame_interval = max96724_enum_frame_interval,
	.get_fmt = max96724_get_fmt,
	.set_fmt = max96724_set_fmt,
	.get_mbus_config = max96724_g_mbus_config,
};

static const struct v4l2_subdev_ops max96724_subdev_ops = {
	.core	= &max96724_core_ops,
	.video	= &max96724_video_ops,
	.pad	= &max96724_pad_ops,
};

/* *********************************************************************** */
static int max96724_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct max96724 *max96724 = container_of(ctrl->handler,
					     struct max96724, ctrl_handler);
	struct i2c_client *client = max96724->client;
	s64 max;
	int ret = 0;

	/* Propagate change of current control to all related controls */
	switch (ctrl->id) {
	case V4L2_CID_VBLANK:
		/* Update max exposure while meeting expected vblanking */
		max = max96724->cur_mode->height + ctrl->val - 4;
		__v4l2_ctrl_modify_range(max96724->exposure,
					 max96724->exposure->minimum, max,
					 max96724->exposure->step,
					 max96724->exposure->default_value);
		break;
	}

	if (!pm_runtime_get_if_in_use(&client->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		/* 4 least significant bits of expsoure are fractional part */
		dev_dbg(&client->dev, "set exposure value 0x%x\n", ctrl->val);
		ret = max96724_write_reg(max96724->client,
					MAX96724_REG_EXPOSURE,
					MAX96724_REG_VALUE_24BIT,
					ctrl->val << 4);
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		dev_dbg(&client->dev, "set analog gain value 0x%x\n", ctrl->val);
		ret = max96724_write_reg(max96724->client,
					MAX96724_REG_GAIN_H,
					MAX96724_REG_VALUE_08BIT,
					(ctrl->val >> MAX96724_GAIN_H_SHIFT) &
					MAX96724_GAIN_H_MASK);
		ret |= max96724_write_reg(max96724->client,
					MAX96724_REG_GAIN_L,
					MAX96724_REG_VALUE_08BIT,
					ctrl->val & MAX96724_GAIN_L_MASK);
		break;
	case V4L2_CID_VBLANK:
		dev_dbg(&client->dev, "set vb value 0x%x\n", ctrl->val);
		ret = max96724_write_reg(max96724->client,
					MAX96724_REG_VTS,
					MAX96724_REG_VALUE_16BIT,
					ctrl->val + max96724->cur_mode->height);
		break;
	case V4L2_CID_TEST_PATTERN:
		ret = max96724_enable_test_pattern(max96724, ctrl->val);
		break;
	default:
		dev_warn(&client->dev, "%s Unhandled id:0x%x, val:0x%x\n",
			 __func__, ctrl->id, ctrl->val);
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static const struct v4l2_ctrl_ops max96724_ctrl_ops = {
	.s_ctrl = max96724_set_ctrl,
};

static int max96724_initialize_controls(struct max96724 *max96724)
{
	const struct max96724_mode *mode;
	struct v4l2_ctrl_handler *handler;
	struct v4l2_ctrl *ctrl;
	s64 exposure_max, vblank_def;
	u32 h_blank;
	int ret;

	handler = &max96724->ctrl_handler;
	mode = max96724->cur_mode;
	ret = v4l2_ctrl_handler_init(handler, 8);
	if (ret)
		return ret;
	handler->lock = &max96724->mutex;

	ctrl = v4l2_ctrl_new_int_menu(handler, NULL, V4L2_CID_LINK_FREQ,
				      0, 0, link_freq_menu_items);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(handler, NULL, V4L2_CID_PIXEL_RATE,
			  0, max96724->pixel_rate, 1, max96724->pixel_rate);

	h_blank = mode->hts_def - mode->width;
	max96724->hblank = v4l2_ctrl_new_std(handler, NULL, V4L2_CID_HBLANK,
				h_blank, h_blank, 1, h_blank);
	if (max96724->hblank)
		max96724->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank_def = mode->vts_def - mode->height;
	max96724->vblank = v4l2_ctrl_new_std(handler, &max96724_ctrl_ops,
				V4L2_CID_VBLANK, vblank_def,
				MAX96724_VTS_MAX - mode->height,
				1, vblank_def);

	exposure_max = mode->vts_def - 4;
	max96724->exposure = v4l2_ctrl_new_std(handler, &max96724_ctrl_ops,
				V4L2_CID_EXPOSURE, MAX96724_EXPOSURE_MIN,
				exposure_max, MAX96724_EXPOSURE_STEP,
				mode->exp_def);

	max96724->anal_gain = v4l2_ctrl_new_std(handler, &max96724_ctrl_ops,
				V4L2_CID_ANALOGUE_GAIN, MAX96724_GAIN_MIN,
				MAX96724_GAIN_MAX, MAX96724_GAIN_STEP,
				MAX96724_GAIN_DEFAULT);

	if (handler->error) {
		ret = handler->error;
		dev_err(&max96724->client->dev,
			"Failed to init controls(%d)\n", ret);
		goto err_free_handler;
	}

	max96724->subdev.ctrl_handler = handler;

	return 0;

err_free_handler:
	v4l2_ctrl_handler_free(handler);

	return ret;
}

/* *********************************************************************** */


static int max96724_check_sensor_id(struct max96724 *max96724,
				   struct i2c_client *client)
{
	struct device *dev = &max96724->client->dev;
	u32 id = 0;
	int ret;

	ret = max96724_read_reg(client, MAX96724_REG_CHIP_ID,
			       MAX96724_REG_VALUE_08BIT, &id);
	dev_info(dev, "Detected %06x sensor\n", id);
	if (id != CHIP_ID) {
		dev_err(dev, "Unexpected sensor id(%06x), ret(%d)\n", id, ret);
		return -ENODEV;
	}

	if (id == CHIP_ID) {
		if (4 == max96724->lane_num) {
			max96724_global_regs = max96724_global_regs_r2a_4lane;

			max96724->cur_mode = &supported_modes_r2a_4lane[0];
			supported_modes = supported_modes_r2a_4lane;
			max96724->cfg_num = ARRAY_SIZE(supported_modes_r2a_4lane);
		}

		max96724->is_r2a = true;
	} else {
		
	}

	return 0;
}

static int max96724_configure_regulators(struct max96724 *max96724)
{
	unsigned int i;

	for (i = 0; i < MAX96724_NUM_SUPPLIES; i++)
		max96724->supplies[i].supply = max96724_supply_names[i];

	return devm_regulator_bulk_get(&max96724->client->dev,
				       MAX96724_NUM_SUPPLIES,
				       max96724->supplies);
}

static int max96724_parse_of(struct max96724 *max96724)
{
	struct device *dev = &max96724->client->dev;
	struct device_node *endpoint;
	struct fwnode_handle *fwnode;
	int rval;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint) {
		dev_err(dev, "Failed to get endpoint\n");
		return -EINVAL;
	}
	fwnode = of_fwnode_handle(endpoint);
	/*
		NULL, 0:
			这是这个函数调用的一个特殊用法。
			当第三个参数（用于存放读取结果的数组指针）为 NULL，
			第四个参数（数组大小）为 0 时，
			这个函数不会去读取属性的具体值（比如 1 和 2），而是只返回该属性数组中元素的个数。

	*/
	rval = fwnode_property_read_u32_array(fwnode, "data-lanes", NULL, 0); // 此时是2
	if (rval <= 0) {
		dev_warn(dev, " Get mipi lane num failed!\n");
		return -1;
	}

	max96724->lane_num = rval;
	if (4 == max96724->lane_num) {
		max96724->cur_mode = &supported_modes_r2a_4lane[0];
		supported_modes = supported_modes_r2a_4lane;
		max96724->cfg_num = ARRAY_SIZE(supported_modes_r2a_4lane);

		max96724->pixel_rate = MIPI_FREQ * 2U * max96724->lane_num / 10U;
		dev_info(dev, "lane_num(%d)  pixel_rate(%u)\n",
				 max96724->lane_num, max96724->pixel_rate);
	}
	return 0;
}

static int max96724_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	struct max96724 *max96724;
	struct v4l2_subdev *sd;
	// char facing[2];  // unused for now
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	max96724 = devm_kzalloc(dev, sizeof(*max96724), GFP_KERNEL);
	if (!max96724)
		return -ENOMEM;

	max96724->client = client;
	
	ret = of_property_read_u32(node, RKMODULE_CAMERA_MODULE_INDEX,
				   &max96724->module_index); // 用于对多摄像头支持
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_NAME,
				       &max96724->module_name);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_LENS_NAME,
				       &max96724->len_name);
	if (ret) {
		dev_err(dev,
			"could not get module information!\n");
		return -EINVAL;
	}

	// TODO: Implement regmap initialization for MAX96724
	max96724->regmap = devm_regmap_init_i2c(client, &max96724_regmap_config);
	if (IS_ERR(max96724->regmap)) {
		dev_err(dev, "Failed to regmap initialize I2C\n");
		return PTR_ERR(max96724->regmap);
	}

	// 低分高帧时，有这个属性；高分低帧时，没有这个属性
	ret = of_property_read_u32(node, OF_CAMERA_CAPTURE_MODE, &max96724->capture_mode); // 读到了就是1
	if (ret) {
		max96724->capture_mode = 0; // 没读到就是0
		dev_warn(dev, " Get capture_mode failed! captrue revolution setting to 3264x2448\n");
	}

	max96724->xvclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(max96724->xvclk)) {
		dev_err(dev, "Failed to get xvclk\n");
		return -EINVAL;
	}

	max96724->power_gpio = devm_gpiod_get(dev, "power", GPIOD_OUT_LOW); // 都没有
	if (IS_ERR(max96724->power_gpio))
		dev_warn(dev, "Failed to get power-gpios, maybe no use\n");

	max96724->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(max96724->reset_gpio))
		dev_warn(dev, "Failed to get reset-gpios, maybe no use\n");

	max96724->pwdn_gpio = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_LOW);
	if (IS_ERR(max96724->pwdn_gpio))
		dev_warn(dev, "Failed to get pwdn-gpios, maybe no use\n");

	/*
		dovdd-supply= <&cam0_dovdd>; 1.8v 
		avdd-supply = <&cam0_avdd>;  2.8v
		dvdd-supply = <&cam0_dvdd>;  1.2v
	*/
	ret = max96724_configure_regulators(max96724);
	if (ret) {
		dev_err(dev, "Failed to get power regulators\n");
		return ret;
	}

	ret = max96724_parse_of(max96724);
	if (ret != 0)
		return -EINVAL;

	max96724->pinctrl = devm_pinctrl_get(dev);
	if (!IS_ERR(max96724->pinctrl)) {
		max96724->pins_default =
			pinctrl_lookup_state(max96724->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_DEFAULT);
		if (IS_ERR(max96724->pins_default))
			dev_err(dev, "could not get default pinstate\n");

		max96724->pins_sleep =
			pinctrl_lookup_state(max96724->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_SLEEP);
		if (IS_ERR(max96724->pins_sleep))
			dev_err(dev, "could not get sleep pinstate\n");
	}

	mutex_init(&max96724->mutex);

	sd = &max96724->subdev;
	v4l2_i2c_subdev_init(sd, client, &max96724_subdev_ops);
	ret = max96724_initialize_controls(max96724);
	if (ret)
		goto err_destroy_mutex;

	ret = __max96724_power_on(max96724);
	if (ret)
		goto err_free_handler;

	ret = max96724_check_sensor_id(max96724, client);
	if (ret)
		goto err_power_off;
		


#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &max96724_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		     V4L2_SUBDEV_FL_HAS_EVENTS;
#endif
#if defined(CONFIG_MEDIA_CONTROLLER)
	max96724->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &max96724->pad);
	if (ret < 0)
		goto err_power_off;
#endif

	ret = v4l2_async_register_subdev_sensor(sd);
	if (ret) {
		dev_err(dev, "v4l2 async register subdev failed\n");
		goto err_clean_entity;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
err_power_off:
	__max96724_power_off(max96724);
err_free_handler:
	v4l2_ctrl_handler_free(&max96724->ctrl_handler);
err_destroy_mutex:
	mutex_destroy(&max96724->mutex);

	return ret;
}

static void max96724_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct max96724 *max96724 = to_max96724(sd);

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	v4l2_ctrl_handler_free(&max96724->ctrl_handler);
		
	mutex_destroy(&max96724->mutex);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		__max96724_power_off(max96724);
	pm_runtime_set_suspended(&client->dev);
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id max96724_of_match[] = {
	{ .compatible = "max96724" },
	{},
};
MODULE_DEVICE_TABLE(of, max96724_of_match);
#endif

static const struct i2c_device_id max96724_match_id[] = {
	{ "max96724", 0 },
	{ },
};

static struct i2c_driver max96724_i2c_driver = {
	.driver = {
		.name = MAX96724_NAME,
		.pm = &max96724_pm_ops,
		.of_match_table = of_match_ptr(max96724_of_match),
	},
	.probe		= &max96724_probe,
	.remove		= &max96724_remove,
	.id_table	= max96724_match_id,
};

static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&max96724_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&max96724_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("OmniVision max96724 sensor driver");
MODULE_LICENSE("GPL v2");
