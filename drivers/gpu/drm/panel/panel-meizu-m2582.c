// SPDX-License-Identifier: GPL-2.0-only
/* Meizu M2582 VTDR6130A command-mode panel. Commands extracted from OEM DT. */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <video/mipi_display.h>

struct m2582_cmd {
	u8 type, delay_ms, len;
	/*
	 * Factory burst grouping: set on the last command of a burst.  This
	 * driver sends one command per transfer, so it never sets
	 * MIPI_DSI_MSG_BATCH_COMMAND; the flag records the grouping the
	 * factory's sequences use.
	 */
	u8 end;
	u8 data[95];
};

/* A command and its payload, sized for the largest entry in the tables. */
#define M2582_CMD_BUF	(sizeof(((struct m2582_cmd *)0)->data))

/*
 * Power-on sequence, transcribed entry for entry from
 * qcom,mdss-dsi-on-command of the factory panel node; it ends with Sleep Out
 * (0x11, 121 ms) and Display On (0x29).
 */
static const struct m2582_cmd m2582_on_cmds[] = {
	{ .type = 0x39, .delay_ms = 21, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x01 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0xd0, 0x05 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0xcf, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xb0, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0xb2, 0x13 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x15 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 10, .end = 1,
	  .data = { 0xb4, 0x45, 0x21, 0x00, 0x55, 0x11, 0x00, 0x50, 0x10, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x03, 0x01 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x35, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x53, 0x20 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x5e, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x59, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x71, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x6f, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x75, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x72, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
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
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0x65, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xf3, 0x24 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x05 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
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
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0xf8, 0xfb } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x81 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5, .end = 1,
	  .data = { 0xfb, 0xd3, 0xd3, 0xd3, 0xd3 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xff, 0x5a, 0x82 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
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
	{ .type = 0x39, .delay_ms = 0, .len = 5, .end = 1,
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
	{ .type = 0x39, .delay_ms = 0, .len = 5, .end = 1,
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
	{ .type = 0x39, .delay_ms = 0, .len = 2, .end = 1,
	  .data = { 0xb3, 0xa0 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x12 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3, .end = 1,
	  .data = { 0xd6, 0x00, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd0, 0x00, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x12 } },
	{ .type = 0x39, .delay_ms = 0, .len = 4, .end = 1,
	  .data = { 0xd1, 0x06, 0x00, 0x00 } },
	/*
	 * The three cold-wake writes: the entire difference between this set and
	 * the factory kernel's qcom,mdss-dsi-on-command.  The factory kernel only
	 * ever runs on a bootloader-initialised panel; the cold-init sequence it
	 * ships (DisplayDxe) carries these three here, right before sleep-out.
	 * This driver resets the panel itself, so it must send them -- measured,
	 * without them about half of all cold boots ended with the panel TE dead.
	 */
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x04 } },
	{ .type = 0x39, .delay_ms = 20, .len = 2, .end = 1,
	  .data = { 0xcf, 0x4b } },
	{ .type = 0x05, .delay_ms = 121, .len = 1, .end = 1,
	  .data = { 0x11 } },
	{ .type = 0x05, .delay_ms = 0, .len = 1, .end = 1,
	  .data = { 0x29 } },
};

/*
 * Adaptive refresh, transcribed byte for byte from the factory panel node.  It
 * exposes six mode sets, three per frame-rate family, byte-identical between
 * timing@0 and timing@1: the family is selected by one byte, 6c 02 for 120 Hz
 * and 6c 03 for 90 Hz, in the power-on, timing-switch and adfr-on sets.  Each
 * set is self-contained (120 Hz writes d1/d2/d5/d7, 90 Hz d3/d4/d6/d8); d1 00 77
 * and d3 00 59 are the family ceilings, d2/d4 00 0X the floor codes.
 */
/* timing@0 timing-switch-command: ADFR off, family 120 (6c 02), 71 00. */
static const struct m2582_cmd m2582_timing_switch_120_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd0, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x71, 0x00 } },
};

/* timing@1 timing-switch-command: identical to timing@0 except 6c 03. */
static const struct m2582_cmd m2582_timing_switch_90_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd0, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x03 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x71, 0x00 } },
};

/* timing@0 adfr-on-command: page 0x10, cf 09, family 120, 71 00, 75 00, ADFR on (d0 11). */
static const struct m2582_cmd m2582_adfr_on_120_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xcf, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x02 } },
	{ .type = 0x39, .delay_ms = 9, .len = 2,
	  .data = { 0x71, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x75, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd0, 0x11 } },
};

/* timing@1 adfr-on-command: same as timing@0 except 6c 03 and 75 02. */
static const struct m2582_cmd m2582_adfr_on_90_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xcf, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x6c, 0x03 } },
	{ .type = 0x39, .delay_ms = 9, .len = 2,
	  .data = { 0x71, 0x00 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x75, 0x02 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd0, 0x11 } },
};

/* adfr-120HZ-30HZ: d1 00 77 = 119 Hz ceiling, d2 00 03 = 30 Hz floor. */
static const struct m2582_cmd m2582_mf_120_30_cmds[] = {
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
};

/* adfr-120HZ-20HZ: same as 120HZ-30HZ with the floor code 04. */
static const struct m2582_cmd m2582_mf_120_20_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd1, 0x00, 0x77 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd1, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd2, 0x00, 0x04 } },
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
	  .data = { 0xd5, 0x04, 0x04, 0x04, 0x04 } },
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
	  .data = { 0xd7, 0x04, 0x04, 0x04, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd7, 0x00, 0x55, 0xaa, 0xff } },
};

/* adfr-120HZ-10HZ: same as 120HZ-30HZ with the floor code 05. */
static const struct m2582_cmd m2582_mf_120_10_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd1, 0x00, 0x77 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd1, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd2, 0x00, 0x05 } },
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
	  .data = { 0xd5, 0x05, 0x05, 0x05, 0x05 } },
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
	  .data = { 0xd7, 0x05, 0x05, 0x05, 0x05 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd7, 0x00, 0x55, 0xaa, 0xff } },
};

/* adfr-90HZ-30HZ: d3 00 59 = 89 Hz ceiling, d4 00 02 = 30 Hz floor. */
static const struct m2582_cmd m2582_mf_90_30_cmds[] = {
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
};

/* adfr-90HZ-18HZ: same as 90HZ-30HZ with the floor code 04. */
static const struct m2582_cmd m2582_mf_90_18_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd3, 0x00, 0x59 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd3, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd4, 0x00, 0x04 } },
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
	  .data = { 0xd6, 0x04, 0x04, 0x04, 0x04 } },
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
	  .data = { 0xd8, 0x04, 0x04, 0x04, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd8, 0x00, 0x55, 0xaa, 0xff } },
};

/* A frame-rate family and a floor, with the set that programs both. */
struct m2582_rate {
	const char *name;
	unsigned int family;
	unsigned int min_hz;
	const struct m2582_cmd *cmds;
	size_t n_cmds;
};

/*
 * adfr-90HZ-10HZ-mode-command, transcribed from the factory's running device
 * tree: the sixth of its six mode sets.
 */
static const struct m2582_cmd m2582_mf_90_10_cmds[] = {
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xf0, 0xaa, 0x1b } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd3, 0x00, 0x59 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x09 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0xd3, 0x10 } },
	{ .type = 0x39, .delay_ms = 0, .len = 3,
	  .data = { 0xd4, 0x00, 0x04 } },
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
	  .data = { 0xd6, 0x04, 0x04, 0x04, 0x04 } },
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
	  .data = { 0xd8, 0x04, 0x04, 0x04, 0x04 } },
	{ .type = 0x39, .delay_ms = 0, .len = 2,
	  .data = { 0x65, 0x18 } },
	{ .type = 0x39, .delay_ms = 0, .len = 5,
	  .data = { 0xd8, 0x00, 0x55, 0xaa, 0xff } },
};

static const struct m2582_rate m2582_rates[] = {
	{ "120-30", 120, 30, m2582_mf_120_30_cmds, ARRAY_SIZE(m2582_mf_120_30_cmds) },
	{ "120-20", 120, 20, m2582_mf_120_20_cmds, ARRAY_SIZE(m2582_mf_120_20_cmds) },
	{ "120-10", 120, 10, m2582_mf_120_10_cmds, ARRAY_SIZE(m2582_mf_120_10_cmds) },
	{ "90-30",   90, 30, m2582_mf_90_30_cmds,  ARRAY_SIZE(m2582_mf_90_30_cmds) },
	{ "90-18",   90, 18, m2582_mf_90_18_cmds,  ARRAY_SIZE(m2582_mf_90_18_cmds) },
	{ "90-10",   90, 10, m2582_mf_90_10_cmds,  ARRAY_SIZE(m2582_mf_90_10_cmds) },
};

/*
 * The factory's off-command set is two packets in two groups: each of 0x28 and
 * 0x10 is released to LP-11 after it, with the 15 ms and 106 ms waits in
 * between.  Both packets therefore carry the "last of the group" flag.
 */
static const struct m2582_cmd m2582_off_cmds[] = {
	{ .type = 0x5, .delay_ms = 15, .len = 2, .end = 1,
	  .data = { 0x28, 0x00 } },
	{ .type = 0x5, .delay_ms = 106, .len = 2, .end = 1,
	  .data = { 0x10, 0x00 } },
};

struct m2582_panel {
	struct drm_panel panel;
	struct device *dev;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset;
	struct regulator_bulk_data *supplies;
	struct drm_dsc_config dsc;
	/* Serialises command sets between the modeset path and the backlight path. */
	struct mutex lock;
	/* Frame-rate family (120 or 90) and floor, as last programmed. */
	unsigned int family;
	unsigned int min_hz;
	/*
	 * The family the driver is being asked for.  It is what the
	 * initialisation tail programs, so a rate requested at runtime survives
	 * the next unprepare()/prepare() cycle: unprepare() puts the DDIC back
	 * to the power-on family, which is only correct while nobody has asked
	 * for another one.
	 */
	unsigned int req_hz;
	/*
	 * The connector this panel is attached to, remembered from get_modes()
	 * so prepare() can read which mode the commit selected.  The atomic
	 * state swap happens before commit_tail, so by the time prepare() runs
	 * connector->state->mode is the new mode.
	 */
	struct drm_connector *connector;
	/* Rails are up: power-on happens in prepare(), so unprepare() must not guess. */
	bool powered;
};

static const struct regulator_bulk_data m2582_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vci" },
	{ .supply = "vdd" },
};

/*
 * The panel's decoder wants first_line_bpg_offset = 15 even though DSC 1.2 (and
 * the factory's own derivation) gives 13 for this panel's 30-line slices.  Both
 * encoder and decoder work from this field, so a wrong value desynchronises
 * their rate-buffer models once per slice line; measured with 13, the lower
 * five sixths of the picture are noise, with 15 the whole picture is correct.
 */
static void m2582_apply_table_rc(struct m2582_panel *ctx)
{
	ctx->dsc.first_line_bpg_offset = 15;
}

static int m2582_send(struct m2582_panel *ctx, const struct m2582_cmd *cmds, size_t count)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;

	for (i = 0; i < count; i++) {
		const struct m2582_cmd *cmd = &cmds[i];
		u8 ppsbuf[M2582_CMD_BUF];
		/*
		 * One DSI transfer per command in LP mode (the factory marks every set
		 * dsi_lp_mode): the framing a cold panel is known to accept.
		 */
		struct mipi_dsi_msg msg = {
			.channel = dsi->channel,
			.type = cmd->type,
			.flags = MIPI_DSI_MSG_USE_LPM,
			.tx_buf = cmd->data,
			.tx_len = cmd->type == MIPI_DSI_DCS_SHORT_WRITE ? 1 : cmd->len,
		};

		ssize_t ret;

		/*
		 * The device tree's 0x70 packet is a placeholder: repack its payload
		 * from the same drm_dsc_config the encoder is programmed from, so
		 * encoder and decoder are described by one struct by construction.
		 */
		if (cmd->data[0] == 0x70 && cmd->len > 1) {
			struct drm_dsc_picture_parameter_set pps;
			size_t n = min_t(size_t, cmd->len - 1, sizeof(pps));

			drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
			memcpy(ppsbuf, cmd->data, cmd->len);
			memcpy(ppsbuf + 1, &pps, n);
			msg.tx_buf = ppsbuf;
		}
		if (!dsi->host->ops->transfer)
			return -EOPNOTSUPP;
		ret = dsi->host->ops->transfer(dsi->host, &msg);
		if (ret < 0)
			return ret;
		/*
		 * msm's dsi_cmds2buf_tx() returns the size of its DMA buffer rather
		 * than the payload length, so only a negative return is a real
		 * failure here.
		 */
		if (cmd->delay_ms)
			msleep(cmd->delay_ms);
	}
	return 0;
}

static int m2582_set_min_fps(struct m2582_panel *ctx, unsigned int family,
			     unsigned int min_hz)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(m2582_rates); i++) {
		if (m2582_rates[i].family == family &&
		    m2582_rates[i].min_hz == min_hz)
			return m2582_send(ctx, m2582_rates[i].cmds,
					  m2582_rates[i].n_cmds);
	}
	return -EINVAL;
}

/*
 * Move the panel to a frame-rate family and set its floor: the target timing
 * node's timing-switch-command (which turns ADFR off), then its adfr-on-command,
 * then the family's mode set.  Staying in the current family only re-sends the
 * mode set, so the power-on sequence the panel has already accepted is left
 * untouched.
 */
static int m2582_set_rate(struct m2582_panel *ctx, unsigned int family,
			  unsigned int min_hz)
{
	int ret;

	if (family != ctx->family) {
		if (family == 90)
			ret = m2582_send(ctx, m2582_timing_switch_90_cmds,
					 ARRAY_SIZE(m2582_timing_switch_90_cmds));
		else
			ret = m2582_send(ctx, m2582_timing_switch_120_cmds,
					 ARRAY_SIZE(m2582_timing_switch_120_cmds));
		if (ret)
			return ret;

		if (family == 90)
			ret = m2582_send(ctx, m2582_adfr_on_90_cmds,
					 ARRAY_SIZE(m2582_adfr_on_90_cmds));
		else
			ret = m2582_send(ctx, m2582_adfr_on_120_cmds,
					 ARRAY_SIZE(m2582_adfr_on_120_cmds));
		if (ret)
			return ret;

		ctx->family = family;
	}

	ret = m2582_set_min_fps(ctx, family, min_hz);
	if (!ret)
		ctx->min_hz = min_hz;

	if (!ret) {
		static const struct m2582_cmd exit_adfr_cmds[] = {
			{ .type = 0x39, .delay_ms = 0, .len = 3,
			  .data = { 0xf0, 0xaa, 0x1b } },
			{ .type = 0x39, .delay_ms = 0, .len = 3, .end = 1,
			  .data = { 0xd0, 0x00, 0x00 } },
		};
		/*
		 * Leave ADFR on the page the panel's own sequences use.  The generic
		 * vtdr6130 nodes in this tree exit with a page-0 write that this DDIC
		 * never selects.
		 */
		m2582_send(ctx, exit_adfr_cmds, ARRAY_SIZE(exit_adfr_cmds));
	}

	return ret;
}

#define M2582_BL_OS_MAX		8191	/* max_os_bl */
#define M2582_BL_DBV_MIN	10	/* factory minimum_bl */

/*
 * The panel's brightness register is 13 bits (the factory's bl-max-level is
 * 0x1fff, i.e. DBV 10..8191), but this project drives nothing past DBV 4095 --
 * the highest value the factory calibration is measured for -- and never
 * touches HBM, because the panel's thermal state cannot be judged here.
 */
static const unsigned int m2582_dbv_max = 4095;

/*
 * The factory brightness curve, from the vendor calibration
 * (backlight_calib_vtdr6130_amoled_cmd_mode_dsi_visionox_panel_with_DSC.xml):
 * max_os_bl 8191 is the framework scale, minimum_bl 1200 the lowest DBV the
 * panel is calibrated for, max_bl 4095 its own maximum.  The (linearized_bl,
 * panel_bl) pairs both end at 4095, so nothing here exceeds the panel rating.
 * HBM is not implemented: the calibration says hbm_mode_supported="0".
 */
static const u16 m2582_bl_linearized[52] = {
	   0,   80,  160,  240,  321,  401,  481,  562,  642,  722,  802,  883,  963,
	1043, 1124, 1204, 1284, 1365, 1445, 1525, 1605, 1686, 1766, 1846, 1927, 2007,
	2087, 2167, 2248, 2328, 2408, 2489, 2569, 2649, 2730, 2810, 2890, 2970, 3051,
	3131, 3211, 3292, 3372, 3452, 3532, 3613, 3693, 3773, 3854, 3934, 4014, 4095,
};

static const u16 m2582_bl_panel[52] = {
	   0,  286,  425,  538,  627,  700,  768,  836,  910,  984, 1042, 1094, 1143,
	1190, 1234, 1276, 1315, 1353, 1389, 1424, 1457, 1489, 1521, 1552, 1583, 1613,
	1641, 1670, 1698, 1725, 1752, 1779, 1804, 1830, 1855, 1879, 1903, 1927, 1950,
	1973, 1995, 2026, 2065, 2200, 2445, 2700, 2951, 3170, 3405, 3633, 3863, 4095,
};

/*
 * OS level -> DBV through the factory curve.  Below the first table point the
 * panel is not calibrated to display anything, so anything short of 0 becomes
 * minimum_bl; 0 stays 0, which the factory driver does too (a zero request must
 * blank the panel, not light it at the floor).
 */
static u16 m2582_bl_to_dbv(unsigned int level)
{
	unsigned int x, i, span, num;
	u32 dbv;

	if (level == 0)
		return 0;
	if (level > M2582_BL_OS_MAX)
		level = M2582_BL_OS_MAX;

	x = DIV_ROUND_CLOSEST(level * m2582_dbv_max, M2582_BL_OS_MAX);

	if (x <= m2582_bl_linearized[0])
		dbv = m2582_bl_panel[0];
	else if (x >= m2582_bl_linearized[ARRAY_SIZE(m2582_bl_linearized) - 1])
		dbv = m2582_bl_panel[ARRAY_SIZE(m2582_bl_panel) - 1];
	else {
		for (i = 1; i < ARRAY_SIZE(m2582_bl_linearized); i++) {
			if (x <= m2582_bl_linearized[i])
				break;
		}
		span = m2582_bl_linearized[i] - m2582_bl_linearized[i - 1];
		num = x - m2582_bl_linearized[i - 1];
		dbv = m2582_bl_panel[i - 1] +
		      DIV_ROUND_CLOSEST(num * (m2582_bl_panel[i] -
					       m2582_bl_panel[i - 1]), span);
	}

	return clamp_val(dbv, M2582_BL_DBV_MIN, m2582_dbv_max);
}

static int __m2582_backlight_update(struct m2582_panel *ctx, u16 dbv)
{
	int ret;

	/*
	 * Stock Meizu vendor driver (msm_drm.ko dsi_panel_update_backlight)
	 * sends DCS 0x51 with 16-bit big-endian DBV.
	 * No manufacturer page0 writes or DCS 0x53 writes are sent.
	 */
	ret = mipi_dsi_dcs_set_display_brightness_large(ctx->dsi, dbv);
	if (ret)
		dev_err(ctx->dev, "M2582 backlight: DCS 0x51 failed: %d\n", ret);

	return ret;
}

static int m2582_backlight_update(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct m2582_panel *ctx = mipi_dsi_get_drvdata(dsi);
	unsigned int level = backlight_get_brightness(bl);
	u16 dbv = m2582_bl_to_dbv(level);
	int ret;

	mutex_lock(&ctx->lock);
	ret = __m2582_backlight_update(ctx, dbv);
	mutex_unlock(&ctx->lock);

	return ret;
}

/*
 * The factory node carries qcom,mdss-dsi-lp11-init and its driver implements it
 * by powering the panel on *after* the DSI host is up, so the DDIC comes out of
 * reset with the lanes already in LP-11; mainline expresses the same order with
 * panel.prepare_prev_first.  A DDIC released from reset with the link down comes
 * up unable to hear the commands that follow, so the reset pulse and the whole
 * command set stay together here, in the factory's order.
 */

/*
 * The 90 Hz member of the same panel mode.  The vertical blanking is stretched
 * by 120/90 so it is a real 90 Hz frame at the panel's line rate; that also
 * keeps it distinct from the 120 Hz mode, since DRM collapses two modes with
 * identical timings and the second one then cannot be selected.  The panel
 * scans at whichever rate its timing-switch set selected and the SoC follows
 * TE, so only the mode name matters -- prepare() reads it for the rate.
 */
static const struct drm_display_mode m2582_mode_90 = {
	.clock = 627126,
	.hdisplay = 1200,
	.hsync_start = 1220,
	.hsync_end = 1224,
	.htotal = 1244,
	.vdisplay = 2670,
	.vsync_start = 3604,
	.vsync_end = 3607,
	.vtotal = 3648,
	.width_mm = 65,
	.height_mm = 148,
};

static const struct drm_display_mode m2582_mode = {
	/*
	 * The pixel clock sets the DSI link rate and must cover the panel's own
	 * bandwidth: 1200 * 2670 * 120 Hz at 8 bpp (DSC) over four lanes is
	 * 769 Mbps per lane. The factory panel node declares
	 * qcom,mdss-dsi-panel-clockrate = 0x419fed40; worked back through this
	 * driver's clock chain that gives the value below. The visible timings
	 * are unchanged.
	 */
	.clock = 627126,
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
static int m2582_prepare(struct drm_panel *panel)
{
	struct m2582_panel *ctx = container_of(panel, struct m2582_panel, panel);
	int i, ret;

	/*
	 * Prepare runs in the DRM bridge chain's pre_enable, after the DSI host's
	 * pre_enable (msm_dsi_host_power_on) brought up the PHY, PLL and link
	 * clocks with the lanes in LP-11 -- the order prepare_prev_first sets --
	 * and before the DPU timing engine/encoder runs.  That is DisplayDxe's
	 * and vendor dsi_display_prepare()'s window: rails, reset pulse, all 107
	 * commands, TE, ADFR and backlight, with no active frames.
	 */
	mutex_lock(&ctx->lock);

	m2582_apply_table_rc(ctx);

	for (i = 0; i < ARRAY_SIZE(m2582_supplies); i++) {
		ret = regulator_enable(ctx->supplies[i].consumer);
		if (ret) {
			dev_err(ctx->dev, "M2582: supply %d failed: %d\n", i, ret);
			while (i--)
				regulator_disable(ctx->supplies[i].consumer);
			mutex_unlock(&ctx->lock);
			return ret;
		}
		msleep(i == 2 ? 20 : 2);
	}
	ctx->powered = true;

	/*
	 * The factory's reset sequence, from the panel node's
	 * qcom,mdss-dsi-reset-sequence <1 5 0 5 1 26>: 5/5/26 ms, active low.
	 */
	gpiod_set_value_cansleep(ctx->reset, 0);
	msleep(5);
	gpiod_set_value_cansleep(ctx->reset, 1);
	msleep(5);
	gpiod_set_value_cansleep(ctx->reset, 0);
	msleep(26);

	ret = m2582_send(ctx, m2582_on_cmds, ARRAY_SIZE(m2582_on_cmds));
	if (!ret) {
		/*
		 * Settle after the power-on set, documented with the cold-wake
		 * trio: without it the DDIC does not execute the tail 11/29 (bad
		 * boots read back 0x0c instead of 0x9c).
		 */
		msleep(300);

		/*
		 * The rate a modeset asks for is the one its mode carries: the
		 * 90 Hz mode and the 120 Hz mode share every timing except the
		 * vertical totals, so those identify it.  prepare() runs inside
		 * the modeset, i.e. after the SoC has stopped pushing frames --
		 * the window the timing switch needs.
		 */
		if (ctx->connector && ctx->connector->state &&
		    ctx->connector->state->crtc) {
			const struct drm_display_mode *sel =
				&ctx->connector->state->crtc->mode;

			if (!strcmp(sel->name, "1200x2670-90"))
				ctx->req_hz = 90;
			else
				ctx->req_hz = 120;
		}

		mipi_dsi_dcs_set_tear_on(ctx->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
		m2582_set_rate(ctx, ctx->req_hz, ctx->min_hz);
	}

	/*
	 * The power-on path stops where the factory bootloader's does: no page exit,
	 * no brightness, no DCS read and no adaptive-refresh traffic after the set.
	 * Extra traffic from the manufacturer page state the DDIC is left in has
	 * been measured to leave the panel unable to put frames on screen.
	 */
	mutex_unlock(&ctx->lock);

	return ret;
}

static int m2582_enable(struct drm_panel *panel)
{
	return 0;
}

static int m2582_unprepare(struct drm_panel *panel)
{
	struct m2582_panel *ctx = container_of(panel, struct m2582_panel, panel);
	int i;

	mutex_lock(&ctx->lock);
	if (ctx->powered) {
		m2582_send(ctx, m2582_off_cmds, ARRAY_SIZE(m2582_off_cmds));
		gpiod_set_value_cansleep(ctx->reset, 1);
		for (i = ARRAY_SIZE(m2582_supplies) - 1; i >= 0; i--) {
			msleep(3);
			regulator_disable(ctx->supplies[i].consumer);
		}
		ctx->powered = false;
	}
	/*
	 * Back to the power-on family, so the next prepare() sends only a mode
	 * set.  req_hz deliberately survives: it is the rate that prepare()'s
	 * tail must program, so a rate asked for at runtime is not undone by the
	 * next disable/enable cycle.
	 */
	ctx->family = 120;
	ctx->min_hz = 30;
	mutex_unlock(&ctx->lock);
	return 0;
}

static int m2582_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct m2582_panel *ctx = container_of(panel, struct m2582_panel, panel);
	struct drm_display_mode *mode;
	unsigned int n = 0;

	ctx->connector = connector;

	mode = drm_mode_duplicate(connector->dev, &m2582_mode);
	if (!mode)
		return -ENOMEM;
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	strscpy(mode->name, "1200x2670-120", sizeof(mode->name));
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);
	n++;

	mode = drm_mode_duplicate(connector->dev, &m2582_mode_90);
	if (!mode)
		return n;
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER;
	strscpy(mode->name, "1200x2670-90", sizeof(mode->name));
	drm_mode_probed_add(connector, mode);
	n++;

	return n;
}

static const struct drm_panel_funcs m2582_panel_funcs = {
	.prepare = m2582_prepare,
	.enable = m2582_enable,
	.unprepare = m2582_unprepare,
	.get_modes = m2582_get_modes,
};

static const struct backlight_ops m2582_bl_ops = { .update_status = m2582_backlight_update };

static int m2582_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct m2582_panel *ctx;
	struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		/*
		 * The scale the framework uses, i.e. the calibration's max_os_bl
		 * (and the factory node's qcom,mdss-brightness-max-level).  It is
		 * NOT a DBV range: m2582_bl_to_dbv() maps it onto the panel's own
		 * 1200..4095 and nothing here may exceed that.
		 */
		.max_brightness = M2582_BL_OS_MAX,
		/* Come up at safe factory default level (320 / 0x140) to protect AMOLED. */
		.brightness = 320,
	};
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct m2582_panel, panel, &m2582_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);
	ret = devm_regulator_bulk_get_const(dev, ARRAY_SIZE(m2582_supplies), m2582_supplies, &ctx->supplies);
	if (ret)
		return ret;
	/*
	 * GPIOD_OUT_LOW, not GPIOD_OUT_HIGH: the DT line is GPIO_ACTIVE_LOW, so
	 * GPIOD_OUT_HIGH would drive it physically low and hold the panel in
	 * reset from probe onwards.
	 */
	ctx->req_hz = 120;
	ctx->min_hz = 30;
	ctx->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset))
		return dev_err_probe(dev, PTR_ERR(ctx->reset), "Get reset GPIO\n");
	ctx->dev = dev;
	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	/*
	 * Command mode, as the factory node is: its label is
	 * qcom,mdss_dsi_meizu_amoled_m2582_vtdr6130a_dsc_cmd and its panel-type is
	 * "dsi_cmd_mode".
	 */
	dsi->mode_flags = MIPI_DSI_MODE_NO_EOT_PACKET |
		MIPI_DSI_CLOCK_NON_CONTINUOUS;
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 2;
	ctx->dsc.slice_height = 30;
	/* The factory geometry: two 600-pixel DSC slices per line. */
	ctx->dsc.slice_count = 2;
	ctx->dsc.slice_width = 1200 / ctx->dsc.slice_count;
	ctx->dsc.slice_chunk_size =
		DIV_ROUND_UP(ctx->dsc.slice_width * ctx->dsc.bits_per_pixel, 8 * 16);
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4;
	ctx->dsc.block_pred_enable = true;
	dsi->dsc = &ctx->dsc;
	/*
	 * The power-on sequence must travel over a live DSI link, so the panel has
	 * to be prepared after the DSI host is powered on; prepare_prev_first
	 * flips the bridge walk to that order.
	 */
	ctx->panel.prepare_prev_first = true;
	ctx->panel.backlight = devm_backlight_device_register(dev, dev_name(dev), dev, dsi, &m2582_bl_ops, &props);
	if (IS_ERR(ctx->panel.backlight))
		return PTR_ERR(ctx->panel.backlight);

	/*
	 * Power-on state, and the state prepare() returns to: the family it is
	 * already in, so that the first m2582_set_rate() sends only a mode set.
	 */
	mutex_init(&ctx->lock);
	ctx->family = 120;
	ctx->min_hz = 30;

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
