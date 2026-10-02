/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Compatibility shims for the M2582 port of Qualcomm's dwc3-msm glue.
 *
 * The vendor sources are written against a 6.6 vendor tree; mainline has since
 * removed or renamed a few APIs they use, and a handful of Qualcomm-only
 * helpers have no mainline equivalent. Everything here is either a mechanical
 * rename or a no-op for hardware this board does not use.
 */
#ifndef __DWC3_MSM_M2582_COMPAT_H
#define __DWC3_MSM_M2582_COMPAT_H

#include <linux/string.h>
#include <linux/gpio.h>

/* strtobool() was replaced by kstrtobool() upstream. */
static inline int m2582_strtobool(const char *s, bool *res)
{
	return kstrtobool(s, res);
}
#define strtobool	m2582_strtobool

/*
 * Legacy integer-GPIO helpers, used by the vendor driver for an optional
 * cable-detect line. This board does not wire one up, so report it absent and
 * let the driver take the "no gpio" path.
 */
static inline int of_get_named_gpio(struct device_node *np,
				    const char *name, int index)
{ return -ENOENT; }
#ifndef GPIOF_IN
#define GPIOF_IN	0
#endif

/*
 * PMIC USB/DPDM switch. The vendor driver calls this when the role or the
 * charger state changes; not needed for a plain peripheral CDC ACM gadget.
 */
#define WCD_USBSS_USB			0
#define WCD_USBSS_CABLE_CONNECT		1
#define WCD_USBSS_CABLE_DISCONNECT	0
static inline int wcd_usbss_switch_update(int sw, int state) { return 0; }
static inline int wcd_usbss_dpdm_switch_update(bool a, bool b) { return 0; }

#endif /* __DWC3_MSM_M2582_COMPAT_H */
