/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal stand-in for the Qualcomm clock extensions header. The ported
 * dwc3-msm sources only use qcom_clk_set_flags() with the two retention hints,
 * which are performance/retention advice rather than functional requirements,
 * so a no-op is sufficient.
 */
#ifndef _LINUX_CLK_QCOM_H
#define _LINUX_CLK_QCOM_H

#include <linux/clk.h>

#define CLKFLAG_RETAIN_MEM	BIT(0)
#define CLKFLAG_NORETAIN_MEM	BIT(1)

static inline int qcom_clk_set_flags(struct clk *clk, unsigned long flags)
{
	return 0;
}

#endif /* _LINUX_CLK_QCOM_H */
