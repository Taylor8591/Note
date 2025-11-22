// SPDX-License-Identifier: GPL-2.0
/*
 * MAX96724 GMSL2 Deserializer Driver with Colorbar Pattern Generator
 * Simplified version for testing
 *
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/rk-camera-module.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/media-entity.h>
#include <linux/regmap.h>

#define MAX96724_DEVICE_ID      0x0D  // Device ID register
#define MAX96724_CHIP_ID        0xA4  // Expected chip ID (may vary)

/* MAX96724 Register Addresses */
#define REG_DEV_ID              0x0D
#define REG_CTRL0               0x13
#define REG_VIDEO_TX0           0x100
#define REG_MIPI_TX_CTRL        0x330
#define REG_MIPI_PHY_0          0x8A0
#define REG_PATTERN_GEN         0x1050  // Pattern generator control
#define REG_PATTERN_TYPE        0x1051  // Pattern type selection

/* MIPI CSI-2 Configuration */
#define MAX96724_LANES          4
#define MAX96724_BITS_PER_SAMPLE 8
/* 
 * MIPI data rate calculation for 1920x1080@30fps YUV422 8-bit:
 * - Pixel clock: 1920 * 1080 * 30 = 62.208 MHz
 * - Data per pixel: 16 bits (YUV422)
 * - Total bandwidth: 62.208 MHz * 16 bits = 995.328 Mbps
 * - Per-lane (4 lanes): 995.328 / 4 = 248.832 Mbps
 * 
 * Use 250 MHz link frequency → 500 Mbps DDR per lane
 */
#define MAX96724_LINK_FREQ_250MHZ 250000000ULL  // 250 MHz/lane = 500 Mbps DDR
#define MAX96724_PIXEL_RATE     (MAX96724_LINK_FREQ_250MHZ * 2 * MAX96724_LANES / MAX96724_BITS_PER_SAMPLE)
// pixel_rate = 250M * 2 * 4 / 8 = 250M pixels/sec

static const s64 link_freq_items[] = {
    MAX96724_LINK_FREQ_250MHZ,
};

struct max96724_priv {
    struct i2c_client *client;
    struct v4l2_subdev sd;
    struct media_pad pad;
    struct v4l2_ctrl_handler ctrl_handler;
    struct v4l2_ctrl *link_freq;
    struct v4l2_ctrl *pixel_rate;
    struct gpio_desc *pwdn_gpio;
    struct regulator *supply;
    
    struct mutex mutex;
    struct regmap *regmap;
    bool power_on;
    bool streaming;
    bool pattern_enabled;
    
    u32 width;
    u32 height;
    u32 fps;
};

static const struct regmap_config max96724_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0x1272,
};

static inline struct max96724_priv *to_max96724(struct v4l2_subdev *sd)
{
    return container_of(sd, struct max96724_priv, sd);
}

/* I2C read/write functions */
static int max96724_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
    struct i2c_msg msgs[2];
    u8 buf[2];
    int ret;

    buf[0] = (reg >> 8) & 0xff;
    buf[1] = reg & 0xff;

    msgs[0].addr = client->addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = buf;

    msgs[1].addr = client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = val;

    ret = i2c_transfer(client->adapter, msgs, 2);
    return ret == 2 ? 0 : -EIO;
}

static int max96724_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
    u8 buf[3];
    int ret;

    buf[0] = (reg >> 8) & 0xff;
    buf[1] = reg & 0xff;
    buf[2] = val;

    ret = i2c_master_send(client, buf, 3);
    return ret == 3 ? 0 : -EIO;
}

/* Power on sequence */
static int __max96724_power_on(struct max96724_priv *priv)
{
    struct i2c_client *client = priv->client;
    struct device *dev = &client->dev;

    dev_info(dev, "MAX96724 power on\n");

    /* Power down = 0 (active) to enable chip */
    if (!IS_ERR(priv->pwdn_gpio)) {
        gpiod_set_value_cansleep(priv->pwdn_gpio, 0);
        usleep_range(10000, 20000);
    }

    return 0;
}

static void __max96724_power_off(struct max96724_priv *priv)
{
    struct i2c_client *client = priv->client;

    dev_info(&client->dev, "MAX96724 power off\n");

    /* Power down = 1 to disable chip */
    if (!IS_ERR(priv->pwdn_gpio))
        gpiod_set_value_cansleep(priv->pwdn_gpio, 1);
}

/* Hardware initialization - Based on MAX96724 Register Map */
/* Pre-configure everything EXCEPT final enables (CSI_OUT_EN, Pattern Gen, MIPI clock) */
static int max96724_hw_init(struct max96724_priv *priv)
{
    struct i2c_client *client = priv->client;
    u8 val;
    int ret;

    dev_info(&client->dev, "MAX96724 hardware initialization\n");

    /* Read chip ID from DEV_ID (0x000D) */
    ret = max96724_read_reg(client, 0x000D, &val);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read chip ID: %d\n", ret);
        return ret;
    }
    dev_info(&client->dev, "Chip ID: 0x%02x\n", val);

    // /* === Configure ALL registers except final enables === */
    
    // /* Disable GMSL2 */
    // max96724_write_reg(client, 0x0006, 0xF0); // Link A/B/C/D: select GMSL2, Disabled
    // max96724_write_reg(client, 0x040B, 0x00); // CSI_OUT_EN=0, CSI output disabled

    // // Pipe0上的Pattern Generator 时钟配置
    // max96724_write_reg(client, 0x01DC, 0x80);  // 375MHz
    
    // // Pattern Generator 的总开关
    // max96724_write_reg(client, 0x1050, 0x00);  // Disabled
    
    // // 配置 Pattern 的图案
    // max96724_write_reg(client, 0x1051, 0x10);

    // // 配置 VSYNC (帧同步) 时序
    // // VS Delay
    // max96724_write_reg(client, 0x1052, 0x00); max96724_write_reg(client, 0x1053, 0x00); max96724_write_reg(client, 0x1054, 0x00);
    // // VS High Period
    // max96724_write_reg(client, 0x1055, 0x00); max96724_write_reg(client, 0x1056, 0x00); max96724_write_reg(client, 0x1057, 0x05);
    // // VS Low Period
    // max96724_write_reg(client, 0x1058, 0x00); max96724_write_reg(client, 0x1059, 0x04); max96724_write_reg(client, 0x105A, 0x65);
    
    // // 配置 HSYNC (行同步) 时序
    // // V2H
    // max96724_write_reg(client, 0x105B, 0x00); max96724_write_reg(client, 0x105C, 0x00); max96724_write_reg(client, 0x105D, 0x00);
    // // HS High Period
    // max96724_write_reg(client, 0x105E, 0x00); max96724_write_reg(client, 0x105F, 0x2C);
    // // HS Low Period
    // max96724_write_reg(client, 0x1060, 0x08); max96724_write_reg(client, 0x1061, 0x98);
    // // HS Count
    // max96724_write_reg(client, 0x1062, 0x00); max96724_write_reg(client, 0x1063, 0x01);

    // // 配置 DE (有效数据区) 时序
    // // V2D
    // max96724_write_reg(client, 0x1064, 0x00); max96724_write_reg(client, 0x1065, 0x00); max96724_write_reg(client, 0x1066, 0x29);
    // // DE High Period
    // max96724_write_reg(client, 0x1067, 0x04); max96724_write_reg(client, 0x1068, 0x38);
    // // DE Low Period
    // max96724_write_reg(client, 0x1069, 0x00); max96724_write_reg(client, 0x106A, 0x2D);
    // // DE Count
    // max96724_write_reg(client, 0x106B, 0x07); max96724_write_reg(client, 0x106C, 0x80);

    // // 配置彩条图案内容
    // max96724_write_reg(client, 0x106D, 0x20);

    // // ???
    // max96724_write_reg(client, 0x106E, 0xFF); max96724_write_reg(client, 0x106F, 0xFF); max96724_write_reg(client, 0x1070, 0xFF);
    // max96724_write_reg(client, 0x1071, 0x00); max96724_write_reg(client, 0x1072, 0x00); max96724_write_reg(client, 0x1073, 0x00);
    

    // /* Video Pipeline */
    // max96724_write_reg(client, 0x00F0, 0x03); // Video Pipe Selection: Phy A -> Pipe Z -> Pipe 0
    // max96724_write_reg(client, 0x00F4, 0x0F);  // Enable all pipes
    
    // /* MIPI Data Type (CSI_OUT_EN=0 for now) */
    // max96724_write_reg(client, 0x040B, 0x80);  // 控制管道0的输出数据格式和使能状态(YUV422 8-bit+disable)
    // // max96724_write_reg(client, 0x040C, 0x00);  // 控制管道0的虚拟通道
    // // max96724_write_reg(client, 0x040D, 0x00);
    // max96724_write_reg(client, 0x040E, 0x1E);  // 强制管道 0 输出 YUV422 8-bit 格式
    // max96724_write_reg(client, 0x040F, 0x00);
    
    // /* MIPI PHY (force_clk_out=0 for now) */
    // max96724_write_reg(client, 0x08A3, 0x00);  // Lane mapping
    // max96724_write_reg(client, 0x08A1, 0x54);  // HS timing
    // max96724_write_reg(client, 0x08A2, 0xF5);  // PHY enable + LP timing
    // max96724_write_reg(client, 0x08A0, 0x04);  // phy_1x4=1, force_clk_out=0
    
    priv->pattern_enabled = false;
    
    dev_info(&client->dev, "Pattern generator configured: colorbar mode\n");
    dev_info(&client->dev, "Output: %dx%d@%dfps via 4-lane MIPI CSI-2\n",
             priv->width, priv->height, priv->fps);

    return 0;
}

/* V4L2 subdev ops */
static int max96724_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct max96724_priv *priv = to_max96724(sd);
    struct i2c_client *client = priv->client;
    int ret = 0;

    dev_info(&client->dev, "Stream %s: %dx%d@%dfps (streaming=%d)\n", 
             enable ? "ON" : "OFF", priv->width, priv->height, priv->fps, priv->streaming);

    mutex_lock(&priv->mutex);
    
    /* Allow re-configuration even if already in the target state (for reset) */
    if (enable) {
        /* Only call pm_runtime_resume_and_get if not already streaming */
        if (!priv->streaming) {
            ret = pm_runtime_resume_and_get(&client->dev);
            if (ret < 0) {
                dev_err(&client->dev, "Failed to resume device: %d\n", ret);
                mutex_unlock(&priv->mutex);
                return ret;
            }
        }
        
        /* All registers pre-configured in hw_init - just enable 3 things */
        dev_info(&client->dev, "MAX96724 quick stream start\n");
        
		// reset
		max96724_write_reg(client, 0x0013, 0x40);
		msleep(20);
		
		// mipi_disable
        max96724_write_reg(client, 0x08a0, 0x00);
		max96724_write_reg(client, 0x040b, 0x00);

		// Select 2x4 mode
		max96724_write_reg(client, 0x08a0, 0x04);
		// 4-lane D-PHY
        max96724_write_reg(client, 0x094a, 0xc0);
		// Add support for lane swapping
        max96724_write_reg(client, 0x08a3, 0xe4);
		// Configure lane polarity for PHY0 and PHY1
        max96724_write_reg(client, 0x08a5, 0x00);
		// Set link frequency for PHY0 and PHY1
        max96724_write_reg(client, 0x0415, 0x3d);
        max96724_write_reg(client, 0x0418, 0x3d);
        max96724_write_reg(client, 0x08a2, 0x30);
		

		// pattern_enable
        max96724_write_reg(client, 0x1052, 0x00);
        max96724_write_reg(client, 0x1053, 0x00);
        max96724_write_reg(client, 0x1054, 0x00);
        max96724_write_reg(client, 0x1055, 0x01);
        max96724_write_reg(client, 0x1056, 0x7a);
        max96724_write_reg(client, 0x1057, 0x20);
        max96724_write_reg(client, 0x1058, 0x25);
        max96724_write_reg(client, 0x1059, 0x9a);
        max96724_write_reg(client, 0x105a, 0x80);
        max96724_write_reg(client, 0x105b, 0x00);
        max96724_write_reg(client, 0x105c, 0x00);
        max96724_write_reg(client, 0x105d, 0x00);
        max96724_write_reg(client, 0x105e, 0x00);
        max96724_write_reg(client, 0x105f, 0x2c);
        max96724_write_reg(client, 0x1060, 0x08);
        max96724_write_reg(client, 0x1061, 0x6c);
        max96724_write_reg(client, 0x1062, 0x04);
        max96724_write_reg(client, 0x1063, 0x65);
        max96724_write_reg(client, 0x1064, 0x01);
        max96724_write_reg(client, 0x1065, 0x61);
        max96724_write_reg(client, 0x1066, 0x18);
        max96724_write_reg(client, 0x1067, 0x07);
        max96724_write_reg(client, 0x1068, 0x80);
        max96724_write_reg(client, 0x1069, 0x01);
        max96724_write_reg(client, 0x106a, 0x18);
        max96724_write_reg(client, 0x106b, 0x04);
        max96724_write_reg(client, 0x106c, 0x38);
		// Generate VS, HS and DE in free-running mode
        max96724_write_reg(client, 0x1050, 0xfb);
		// Set checkerboard pattern colors.
		max96724_write_reg(client, 0x1074, 0x3c);
        max96724_write_reg(client, 0x1075, 0x3c);
        max96724_write_reg(client, 0x1076, 0x3c);
		max96724_write_reg(client, 0x106e, 0xfe);
        max96724_write_reg(client, 0x106f, 0xcc);
        max96724_write_reg(client, 0x1070, 0x00);
        max96724_write_reg(client, 0x1071, 0x00);
        max96724_write_reg(client, 0x1072, 0x6a);
        max96724_write_reg(client, 0x1073, 0xa7);
		max96724_write_reg(client, 0x1051, 0x10);

		// mipi enable
		max96724_write_reg(client, 0x040b, 0x02);
        max96724_write_reg(client, 0x08a0, 0x84);

        msleep(5);  // Minimal delay
        
        priv->streaming = true;
        dev_info(&client->dev, "MAX96724 streaming\n");
    } else {
        if (priv->streaming) {
            /* Stop pattern generator */
            // max96724_write_reg(client, 0x1050, 0xE0);  // Keep VTG_MODE but disable sync generators
            
            priv->streaming = false;
            pm_runtime_put(&client->dev);
            dev_info(&client->dev, "Streaming stopped\n");
        }
    }

    mutex_unlock(&priv->mutex);
    return ret;
}

static int max96724_s_power(struct v4l2_subdev *sd, int on)
{
    struct max96724_priv *priv = to_max96724(sd);
    struct i2c_client *client = priv->client;
    int ret = 0;

    dev_info(&client->dev, "Power %s\n", on ? "ON" : "OFF");

    mutex_lock(&priv->mutex);

    /* If the power state is not modified - no work to do. */
    if (priv->power_on == !!on)
        goto unlock_and_return;

    if (on) {
        ret = pm_runtime_get_sync(&client->dev);
        if (ret < 0) {
            pm_runtime_put_noidle(&client->dev);
            goto unlock_and_return;
        }
        priv->power_on = true;
    } else {
        pm_runtime_put(&client->dev);
        priv->power_on = false;
    }

unlock_and_return:
    mutex_unlock(&priv->mutex);
    return ret;
}

/* Runtime PM callbacks */
static int max96724_runtime_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct max96724_priv *priv = v4l2_get_subdevdata(sd);
    int ret;

    ret = __max96724_power_on(priv);
    if (ret)
        return ret;

    /* Initialize hardware after power on */
    return max96724_hw_init(priv);
}

static int max96724_runtime_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct max96724_priv *priv = v4l2_get_subdevdata(sd);

    __max96724_power_off(priv);
    return 0;
}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int max96724_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
    struct max96724_priv *priv = v4l2_get_subdevdata(sd);
    struct v4l2_mbus_framefmt *try_fmt =
        v4l2_subdev_get_try_format(sd, fh->state, 0);

    mutex_lock(&priv->mutex);
    
    /* Initialize try_fmt */
    try_fmt->width = priv->width;
    try_fmt->height = priv->height;
    try_fmt->code = MEDIA_BUS_FMT_UYVY8_2X8;
    try_fmt->field = V4L2_FIELD_NONE;
    try_fmt->colorspace = V4L2_COLORSPACE_SRGB;
    try_fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    try_fmt->quantization = V4L2_QUANTIZATION_DEFAULT;
    try_fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
    
    mutex_unlock(&priv->mutex);
    return 0;
}
#endif

static const struct v4l2_subdev_internal_ops max96724_internal_ops = {
    .open = max96724_open,
};

/* Initialize default format */
static int max96724_init_cfg(struct v4l2_subdev *sd,
                             struct v4l2_subdev_state *state)
{
    struct v4l2_mbus_framefmt *format;
    
    format = v4l2_subdev_get_try_format(sd, state, 0);
    format->width = 1920;
    format->height = 1080;
    format->code = MEDIA_BUS_FMT_UYVY8_2X8;
    format->field = V4L2_FIELD_NONE;
    format->colorspace = V4L2_COLORSPACE_SRGB;
    format->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    format->quantization = V4L2_QUANTIZATION_DEFAULT;
    format->xfer_func = V4L2_XFER_FUNC_DEFAULT;
    
    return 0;
}

/* Get media bus configuration */
static int max96724_get_mbus_config(struct v4l2_subdev *sd,
                                    unsigned int pad,
                                    struct v4l2_mbus_config *cfg)
{
    cfg->type = V4L2_MBUS_CSI2_DPHY;
    cfg->bus.mipi_csi2.num_data_lanes = 4;
    cfg->bus.mipi_csi2.flags = 0;
    
    return 0;
}

/* Get format */
static int max96724_get_fmt(struct v4l2_subdev *sd,
                            struct v4l2_subdev_state *state,
                            struct v4l2_subdev_format *fmt)
{
    struct max96724_priv *priv = to_max96724(sd);
    
    /* rkcif passes NULL state, so check both which and state */
    if (fmt->which == V4L2_SUBDEV_FORMAT_TRY && state) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
        fmt->format = *v4l2_subdev_get_try_format(sd, state, fmt->pad);
#else
        return -ENOTTY;
#endif
    } else {
        /* ACTIVE format - return current mode */
        fmt->format.width = priv->width;
        fmt->format.height = priv->height;
        fmt->format.code = MEDIA_BUS_FMT_UYVY8_2X8;
        fmt->format.field = V4L2_FIELD_NONE;
        fmt->format.colorspace = V4L2_COLORSPACE_SRGB;
        fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
        fmt->format.quantization = V4L2_QUANTIZATION_DEFAULT;
        fmt->format.xfer_func = V4L2_XFER_FUNC_DEFAULT;
    }
    
    dev_info(&priv->client->dev, "get_fmt called: which=%d, pad=%d, returning %dx%d, code=0x%x\n",
             fmt->which, fmt->pad, fmt->format.width, fmt->format.height, fmt->format.code);
    
    return 0;
}

/* Set format */
static int max96724_set_fmt(struct v4l2_subdev *sd,
                            struct v4l2_subdev_state *state,
                            struct v4l2_subdev_format *fmt)
{
    struct max96724_priv *priv = to_max96724(sd);
    struct v4l2_mbus_framefmt *mbus_fmt;
    
    /* rkcif passes NULL state, so check both which and state */
    if (fmt->which == V4L2_SUBDEV_FORMAT_TRY && state) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
        mbus_fmt = v4l2_subdev_get_try_format(sd, state, fmt->pad);
#else
        return -ENOTTY;
#endif
    } else {
        mbus_fmt = &fmt->format;
    }
    
    /* Only support 1920x1080 colorbar pattern */
    mbus_fmt->width = 1920;
    mbus_fmt->height = 1080;
    mbus_fmt->code = MEDIA_BUS_FMT_UYVY8_2X8;
    mbus_fmt->field = V4L2_FIELD_NONE;
    mbus_fmt->colorspace = V4L2_COLORSPACE_SRGB;
    mbus_fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    mbus_fmt->quantization = V4L2_QUANTIZATION_DEFAULT;
    mbus_fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
    
    if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE || !state) {
        priv->width = mbus_fmt->width;
        priv->height = mbus_fmt->height;
    }
    
    /* Copy back to format */
    fmt->format = *mbus_fmt;
    
    return 0;
}

/* Enumerate media bus codes */
static int max96724_enum_mbus_code(struct v4l2_subdev *sd,
                                   struct v4l2_subdev_state *state,
                                   struct v4l2_subdev_mbus_code_enum *code)
{
    if (code->index > 0)
        return -EINVAL;
    
    code->code = MEDIA_BUS_FMT_UYVY8_2X8;
    return 0;
}

/* Enumerate frame sizes */
static int max96724_enum_frame_size(struct v4l2_subdev *sd,
                                    struct v4l2_subdev_state *state,
                                    struct v4l2_subdev_frame_size_enum *fse)
{
    if (fse->index > 0)
        return -EINVAL;
    
    if (fse->code != MEDIA_BUS_FMT_UYVY8_2X8)
        return -EINVAL;
    
    fse->min_width = 1920;
    fse->max_width = 1920;
    fse->min_height = 1080;
    fse->max_height = 1080;
    
    return 0;
}

/* Enumerate frame intervals (for rkcif buffer size calculation) */
static int max96724_enum_frame_interval(struct v4l2_subdev *sd,
                                        struct v4l2_subdev_state *state,
                                        struct v4l2_subdev_frame_interval_enum *fie)
{
    struct max96724_priv *priv = to_max96724(sd);
    
    /* We only have one mode - index 0 */
    if (fie->index > 0)
        return -EINVAL;
    
    /* Always set these fields - rkcif needs them! */
    fie->code = MEDIA_BUS_FMT_UYVY8_2X8;
    fie->width = priv->width;
    fie->height = priv->height;
    fie->interval.numerator = 1;
    fie->interval.denominator = priv->fps;
    
    return 0;
}

/* Get frame interval (FPS) */
static int max96724_g_frame_interval(struct v4l2_subdev *sd,
                                     struct v4l2_subdev_frame_interval *fi)
{
    struct max96724_priv *priv = to_max96724(sd);
    
    fi->interval.numerator = 1;
    fi->interval.denominator = priv->fps;
    
    return 0;
}

/* Handle Rockchip-specific ioctl commands */
static long max96724_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct max96724_priv *priv = to_max96724(sd);
    struct rkmodule_channel_info *ch_info;
    
    switch (cmd) {
    case RKMODULE_GET_CHANNEL_INFO:
        ch_info = (struct rkmodule_channel_info *)arg;
        ch_info->vc = 0;
        ch_info->width = priv->width;
        ch_info->height = priv->height;
        ch_info->bus_fmt = MEDIA_BUS_FMT_UYVY8_2X8;
        ch_info->data_type = 0x1E; /* YUV422 8-bit CSI-2 data type */
        ch_info->data_bit = 8;
        ch_info->field = V4L2_FIELD_NONE;
        dev_info(sd->dev, "RKMODULE_GET_CHANNEL_INFO: %dx%d, bus_fmt=0x%x\n",
                 ch_info->width, ch_info->height, ch_info->bus_fmt);
        return 0;
    default:
        return -ENOIOCTLCMD;
    }
}

static const struct v4l2_subdev_core_ops max96724_core_ops = {
    .s_power = max96724_s_power,
    .ioctl = max96724_ioctl,
};

static const struct v4l2_subdev_video_ops max96724_video_ops = {
    .s_stream = max96724_s_stream,
    .g_frame_interval = max96724_g_frame_interval,
};

static const struct v4l2_subdev_pad_ops max96724_pad_ops = {
    .init_cfg = max96724_init_cfg,
    .enum_mbus_code = max96724_enum_mbus_code,
    .enum_frame_size = max96724_enum_frame_size,
    .enum_frame_interval = max96724_enum_frame_interval,
    .get_fmt = max96724_get_fmt,
    .set_fmt = max96724_set_fmt,
    .get_mbus_config = max96724_get_mbus_config,
};

static const struct v4l2_subdev_ops max96724_subdev_ops = {
    .core = &max96724_core_ops,
    .video = &max96724_video_ops,
    .pad = &max96724_pad_ops,
};

/* Media entity operations */
static const struct media_entity_operations max96724_entity_ops = {
    .link_validate = v4l2_subdev_link_validate,
};

/* Probe and remove */
static int max96724_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct max96724_priv *priv;
    struct device *dev = &client->dev;
    int ret;

    dev_info(dev, "MAX96724 driver probe\n");

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->client = client;

    priv->regmap = devm_regmap_init_i2c(client, &max96724_regmap_config);
    if (IS_ERR(priv->regmap)) {
        dev_err(dev, "Failed to initialize regmap\n");
        return PTR_ERR(priv->regmap);
    }

    /* Set default video format (1920x1080@30fps) */
    priv->width = 1920;
    priv->height = 1080;
    priv->fps = 30;
    
    /* Initialize mutex */
    mutex_init(&priv->mutex);

    /* Get power-down GPIO (active high) */
    priv->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn", GPIOD_OUT_HIGH);
    if (IS_ERR(priv->pwdn_gpio))
        dev_warn(dev, "Failed to get pwdn-gpio\n");

    /* Initialize V4L2 subdev */
    v4l2_i2c_subdev_init(&priv->sd, client, &max96724_subdev_ops);
    
    /* Initialize V4L2 controls */
    ret = v4l2_ctrl_handler_init(&priv->ctrl_handler, 2);
    if (ret < 0) {
        dev_err(dev, "Failed to init ctrl handler: %d\n", ret);
        media_entity_cleanup(&priv->sd.entity);
        return ret;
    }
    
    priv->ctrl_handler.lock = &priv->mutex;
    
    /* Link frequency control (read-only) */
    priv->link_freq = v4l2_ctrl_new_int_menu(&priv->ctrl_handler, NULL,
                        V4L2_CID_LINK_FREQ, 
                        ARRAY_SIZE(link_freq_items) - 1, 0,
                        link_freq_items);
    if (priv->link_freq)
        priv->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;
    
    /* Pixel rate control (read-only) */
    priv->pixel_rate = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL,
                        V4L2_CID_PIXEL_RATE, 0, MAX96724_PIXEL_RATE,
                        1, MAX96724_PIXEL_RATE);
    if (priv->pixel_rate)
        priv->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
    
    if (priv->ctrl_handler.error) {
        ret = priv->ctrl_handler.error;
        dev_err(dev, "Control handler init failed: %d\n", ret);
        goto err_free_handler;
    }
    
    priv->sd.ctrl_handler = &priv->ctrl_handler;
    
    /* Initialize media pad */
    priv->pad.flags = MEDIA_PAD_FL_SOURCE;
    ret = media_entity_pads_init(&priv->sd.entity, 1, &priv->pad);
    if (ret < 0) {
        dev_err(dev, "Failed to init media entity: %d\n", ret);
        goto err_free_handler;
    }

    /* Power on and check chip */
    ret = __max96724_power_on(priv);
    if (ret)
        goto err_clean_entity;
    
    ret = max96724_hw_init(priv);
    if (ret) {
        dev_warn(dev, "Failed to read chip ID, but keeping power on for debugging\n");
        /* Continue probe even if chip ID read fails */
    }

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
    priv->sd.internal_ops = &max96724_internal_ops;
    priv->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
    priv->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    priv->sd.entity.ops = &max96724_entity_ops;
#endif
    
    v4l2_set_subdevdata(&priv->sd, priv);
    
    /* Register V4L2 async subdev */
    ret = v4l2_async_register_subdev(&priv->sd);
    if (ret < 0) {
        dev_err(dev, "Failed to register async subdev: %d\n", ret);
        goto err_clean_entity;
    }
    
    /* Enable runtime PM */
    pm_runtime_set_active(dev);
    pm_runtime_enable(dev);
    pm_runtime_idle(dev);

    dev_info(dev, "MAX96724 registered as V4L2 subdev (colorbar mode)\n");
    return 0;

err_clean_entity:
    __max96724_power_off(priv);
#if defined(CONFIG_MEDIA_CONTROLLER)
    media_entity_cleanup(&priv->sd.entity);
#endif
err_free_handler:
    v4l2_ctrl_handler_free(&priv->ctrl_handler);
    mutex_destroy(&priv->mutex);
    return ret;
}

static void max96724_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct max96724_priv *priv = to_max96724(sd);
    struct device *dev = &client->dev;

    v4l2_async_unregister_subdev(&priv->sd);
    media_entity_cleanup(&priv->sd.entity);
    v4l2_ctrl_handler_free(&priv->ctrl_handler);
    
    pm_runtime_disable(dev);
    if (!pm_runtime_status_suspended(dev))
        __max96724_power_off(priv);
    pm_runtime_set_suspended(dev);
    
    mutex_destroy(&priv->mutex);
}

static const struct of_device_id max96724_of_match[] = {
    { .compatible = "maxim,max96724" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, max96724_of_match);

static const struct i2c_device_id max96724_id[] = {
    { "max96724", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, max96724_id);

static const struct dev_pm_ops max96724_pm_ops = {
    SET_RUNTIME_PM_OPS(max96724_runtime_suspend,
                       max96724_runtime_resume, NULL)
};

static struct i2c_driver max96724_i2c_driver = {
    .driver = {
        .name = "max96724",
        .pm = &max96724_pm_ops,
        .of_match_table = max96724_of_match,
    },
    .probe = max96724_probe,
    .remove = max96724_remove,
    .id_table = max96724_id,
};

module_i2c_driver(max96724_i2c_driver);

MODULE_DESCRIPTION("MAX96724 GMSL2 Deserializer Driver with Colorbar");
MODULE_AUTHOR("Rockchip");
MODULE_LICENSE("GPL");
