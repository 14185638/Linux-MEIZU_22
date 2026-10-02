// SPDX-License-Identifier: GPL-2.0-only
/* Meizu M2582 VTDR6130A command-mode panel. Commands extracted from OEM DT. */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <video/mipi_display.h>

struct m2582_cmd {
	u8 type, delay_ms, len;
	u8 data[95];
};

static const struct m2582_cmd m2582_on_cmds[] = {
	{ .type = 0x39, .delay_ms = 21, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x01 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd0, 0x05 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xcf, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb0, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb2, 0x13 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x15 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 10,
	  .data = { 0xb4, 0x45, 0x21, 0x00, 0x55, 0x11, 0x00, 0x50, 0x10, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x03, 0x01 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x35, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x53, 0x20 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x5e, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x59, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x71, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6f, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x75, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x72, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x03, 0x01 } },
	{ .type = 0x39, .delay_ms = 0, .len = 95,
	  .data = { 0x70, 0x12, 0x00, 0x00, 0xab, 0x30, 0x80, 0x0a, 0x6e, 0x04, 0xb0, 0x00, 0x1e, 0x02, 0x58, 0x02, 0x58, 0x02, 0x00, 0x01, 0x19, 0x00, 0x20, 0x05, 0xd0, 0x00, 0x08, 0x00, 0x01, 0x00, 0x47, 0x03, 0x0d, 0x18, 0x00, 0x10, 0xf0, 0x07, 0x10, 0x20, 0x00, 0x06, 0x0f, 0x0f, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e, 0x02, 0x02, 0x22, 0x00, 0x2a, 0x40, 0x2a, 0xbe, 0x3a, 0xfc, 0x3a, 0xfa, 0x3a, 0xf8, 0x3b, 0x38, 0x3b, 0x78, 0x3b, 0xb6, 0x4b, 0xb6, 0x4b, 0xf4, 0x4b, 0xf4, 0x6c, 0x34, 0x84, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x80 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x0a } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf9, 0x9e, 0x8f } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x0f } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf9, 0x14 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x81 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf3, 0x24 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x05 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf3, 0xa0 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x81 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf8, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x1a } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf8, 0x0f } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x0a } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf8, 0xfb } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x81 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xfb, 0xd3, 0xd3, 0xd3, 0xd3 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x82 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf8, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd1, 0x00, 0x77 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd1, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd2, 0x00, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd2, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 9,
	  .data = { 0xd5, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x08 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd5, 0x77, 0x77, 0x77, 0x77 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x0c } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd5, 0x03, 0x03, 0x03, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd5, 0x00, 0x55, 0xaa, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 9,
	  .data = { 0xd7, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x08 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd7, 0x77, 0x77, 0x77, 0x77 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x0c } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd7, 0x03, 0x03, 0x03, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd7, 0x00, 0x55, 0xaa, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd3, 0x00, 0x59 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd3, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd4, 0x00, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd4, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 9,
	  .data = { 0xd6, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd6, 0x59, 0x59, 0x59, 0x59 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x14 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd6, 0x02, 0x02, 0x02, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd6, 0x00, 0x55, 0xaa, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 9,
	  .data = { 0xd8, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd8, 0x59, 0x59, 0x59, 0x59 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x14 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd8, 0x02, 0x02, 0x02, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd8, 0x00, 0x55, 0xaa, 0xff } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x17 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb2, 0x16 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb3, 0x2a } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb5, 0x0d } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb3, 0xa0 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x12 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd6, 0x00, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd0, 0x00, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x12 } },
	{ .type = 0x39, .delay_ms = 0, .len = 4,
	  .data = { 0xd1, 0x06, 0x00, 0x00 } },
	{ .type = 0x5, .delay_ms = 121, .len = 1,
	  .data = { 0x11 } },
	{ .type = 0x5, .delay_ms = 0, .len = 1,
	  .data = { 0x29 } },
};

static const struct m2582_cmd m2582_off_cmds[] = {
	{ .type = 0x5, .delay_ms = 15, .len = 2,
	  .data = { 0x28, 0x00 } },
	{ .type = 0x5, .delay_ms = 106, .len = 2,
	  .data = { 0x10, 0x00 } },
};

struct m2582_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset;
	struct regulator_bulk_data *supplies;
	struct drm_dsc_config dsc;
};

static const struct regulator_bulk_data m2582_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vci" },
	{ .supply = "vdd" },
};

static int m2582_send(struct m2582_panel *ctx, const struct m2582_cmd *cmds, size_t count)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;

	for (i = 0; i < count; i++) {
		const struct m2582_cmd *cmd = &cmds[i];
		struct mipi_dsi_msg msg = {
			.channel = dsi->channel,
			.type = cmd->type,
			.flags = MIPI_DSI_MSG_USE_LPM,
			.tx_buf = cmd->data,
			.tx_len = cmd->type == MIPI_DSI_DCS_SHORT_WRITE ? 1 : cmd->len,
		};
		struct drm_dsc_picture_parameter_set pps;
		u8 payload[95];
		ssize_t ret;

		/* Keep the DDIC's vendor PPS packet consistent with the host encoder. */
		if (cmd->data[0] == 0x70 && cmd->len == sizeof(payload)) {
			drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
			payload[0] = 0x70;
			memcpy(payload + 1, &pps, sizeof(payload) - 1);
			msg.tx_buf = payload;
		}
		if (!dsi->host->ops->transfer)
			return -EOPNOTSUPP;
		ret = dsi->host->ops->transfer(dsi->host, &msg);
		if (ret < 0)
			return ret;
		if (ret != msg.tx_len)
			return -EIO;
		if (cmd->delay_ms)
			msleep(cmd->delay_ms);
	}
	return 0;
}

static int m2582_prepare(struct drm_panel *panel)
{
	struct m2582_panel *ctx = container_of(panel, struct m2582_panel, panel);
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(m2582_supplies); i++) {
		ret = regulator_enable(ctx->supplies[i].consumer);
		if (ret)
			goto disable;
		msleep(i == 2 ? 20 : 2);
	}
	gpiod_set_value_cansleep(ctx->reset, 0);
	msleep(5);
	gpiod_set_value_cansleep(ctx->reset, 1);
	msleep(5);
	gpiod_set_value_cansleep(ctx->reset, 0);
	msleep(26);
	ret = m2582_send(ctx, m2582_on_cmds, ARRAY_SIZE(m2582_on_cmds));
	if (!ret)
		return 0;
	gpiod_set_value_cansleep(ctx->reset, 1);
disable:
	while (i--) {
		msleep(3);
		regulator_disable(ctx->supplies[i].consumer);
	}
	return ret;
}

static int m2582_unprepare(struct drm_panel *panel)
{
	struct m2582_panel *ctx = container_of(panel, struct m2582_panel, panel);
	int i;

	m2582_send(ctx, m2582_off_cmds, ARRAY_SIZE(m2582_off_cmds));
	gpiod_set_value_cansleep(ctx->reset, 1);
	for (i = ARRAY_SIZE(m2582_supplies) - 1; i >= 0; i--) {
		msleep(3);
		regulator_disable(ctx->supplies[i].consumer);
	}
	return 0;
}

static const struct drm_display_mode m2582_mode = {
	.clock = (1200 + 20 + 4 + 20) * (2670 + 33 + 2 + 31) * 120 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1220,
	.hsync_end = 1224,
	.htotal = 1244,
	.vdisplay = 2670,
	.vsync_start = 2703,
	.vsync_end = 2705,
	.vtotal = 2736,
	.width_mm = 65,
	.height_mm = 148,
};

static int m2582_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode = drm_mode_duplicate(connector->dev, &m2582_mode);

	if (!mode)
		return -ENOMEM;
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);
	return 1;
}

static const struct drm_panel_funcs m2582_panel_funcs = {
	.prepare = m2582_prepare,
	.unprepare = m2582_unprepare,
	.get_modes = m2582_get_modes,
};

static int m2582_backlight_update(struct backlight_device *bl)
{
	return mipi_dsi_dcs_set_display_brightness_large(bl_get_data(bl), backlight_get_brightness(bl));
}
static const struct backlight_ops m2582_bl_ops = { .update_status = m2582_backlight_update };

static int m2582_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct m2582_panel *ctx;
	struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.max_brightness = 8191,
		.brightness = 320,
	};
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct m2582_panel, panel, &m2582_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);
	ret = devm_regulator_bulk_get_const(dev, ARRAY_SIZE(m2582_supplies), m2582_supplies, &ctx->supplies);
	if (ret)
		return ret;
	ctx->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset))
		return dev_err_probe(dev, PTR_ERR(ctx->reset), "Get reset GPIO\n");
	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET | MIPI_DSI_CLOCK_NON_CONTINUOUS;
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 2;
	ctx->dsc.slice_height = 30;
	ctx->dsc.slice_width = 600;
	ctx->dsc.slice_count = 2;
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4;
	ctx->dsc.block_pred_enable = true;
	dsi->dsc = &ctx->dsc;
	ctx->panel.prepare_prev_first = true;
	ctx->panel.backlight = devm_backlight_device_register(dev, dev_name(dev), dev, dsi, &m2582_bl_ops, &props);
	if (IS_ERR(ctx->panel.backlight))
		return PTR_ERR(ctx->panel.backlight);
	drm_panel_add(&ctx->panel);
	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&ctx->panel);
	return ret;
}

static void m2582_remove(struct mipi_dsi_device *dsi)
{
	struct m2582_panel *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id m2582_of_match[] = {
	{ .compatible = "meizu,m2582-vtdr6130a" },
	{ }
};
MODULE_DEVICE_TABLE(of, m2582_of_match);
static struct mipi_dsi_driver m2582_driver = {
	.probe = m2582_probe,
	.remove = m2582_remove,
	.driver = { .name = "panel-meizu-m2582", .of_match_table = m2582_of_match },
};
module_mipi_dsi_driver(m2582_driver);
MODULE_DESCRIPTION("Meizu M2582 VTDR6130A command-mode AMOLED panel");
MODULE_LICENSE("GPL");
