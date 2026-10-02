// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023, Linaro Limited
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>

#define EXYNOS_USB_PHY_HS_PHY_CTRL_RST	(0x0)
#define USB_PHY_RST_MASK		GENMASK(1, 0)
#define UTMI_PORT_RST_MASK		GENMASK(5, 4)

#define EXYNOS_USB_PHY_HS_PHY_CTRL_COMMON	(0x4)
#define RPTR_MODE			BIT(10)
#define FSEL_20_MHZ_VAL			(0x1)
#define FSEL_24_MHZ_VAL			(0x2)
#define FSEL_26_MHZ_VAL			(0x3)
#define FSEL_48_MHZ_VAL			(0x2)

#define EXYNOS_USB_PHY_CFG_PLLCFG0	(0x8)
#define PHY_CFG_PLL_FB_DIV_19_8_MASK	GENMASK(19, 8)
#define DIV_19_8_19_2_MHZ_VAL		(0x170)
#define DIV_19_8_20_MHZ_VAL		(0x160)
#define DIV_19_8_24_MHZ_VAL		(0x120)
#define DIV_19_8_26_MHZ_VAL		(0x107)
#define DIV_19_8_48_MHZ_VAL		(0x120)

#define EXYNOS_USB_PHY_CFG_PLLCFG1	(0xc)
#define EXYNOS_PHY_CFG_PLL_FB_DIV_11_8_MASK	GENMASK(11, 8)
#define EXYNOS_DIV_11_8_19_2_MHZ_VAL	(0x0)
#define EXYNOS_DIV_11_8_20_MHZ_VAL	(0x0)
#define EXYNOS_DIV_11_8_24_MHZ_VAL	(0x0)
#define EXYNOS_DIV_11_8_26_MHZ_VAL	(0x0)
#define EXYNOS_DIV_11_8_48_MHZ_VAL	(0x1)

#define EXYNOS_PHY_CFG_TX		(0x14)
#define EXYNOS_PHY_CFG_TX_FSLS_VREF_TUNE_MASK	GENMASK(2, 1)

#define EXYNOS_USB_PHY_UTMI_TESTSE	(0x20)
#define TEST_IDDQ			BIT(6)

#define QCOM_USB_PHY_UTMI_CTRL0		(0x3c)
#define SLEEPM				BIT(0)
#define OPMODE_MASK			GENMASK(4, 3)
#define OPMODE_NONDRIVING		BIT(3)

#define QCOM_USB_PHY_UTMI_CTRL5		(0x50)
#define POR				BIT(1)

#define QCOM_USB_PHY_HS_PHY_CTRL_COMMON0	(0x54)
#define PHY_ENABLE			BIT(0)
#define SIDDQ_SEL			BIT(1)
#define SIDDQ				BIT(2)
#define RETENABLEN			BIT(3)
#define FSEL_MASK			GENMASK(6, 4)
#define FSEL_19_2_MHZ_VAL		(0x0)
#define FSEL_38_4_MHZ_VAL		(0x4)

#define QCOM_USB_PHY_CFG_CTRL_1		(0x58)
#define PHY_CFG_PLL_CPBIAS_CNTRL_MASK	GENMASK(7, 1)

#define QCOM_USB_PHY_CFG_CTRL_2		(0x5c)
#define PHY_CFG_PLL_FB_DIV_7_0_MASK	GENMASK(7, 0)
#define DIV_7_0_19_2_MHZ_VAL		(0x90)
#define DIV_7_0_38_4_MHZ_VAL		(0xc8)

#define QCOM_USB_PHY_CFG_CTRL_3		(0x60)
#define PHY_CFG_PLL_FB_DIV_11_8_MASK	GENMASK(3, 0)
#define DIV_11_8_19_2_MHZ_VAL		(0x1)
#define DIV_11_8_38_4_MHZ_VAL		(0x0)

#define PHY_CFG_PLL_REF_DIV		GENMASK(7, 4)
#define PLL_REF_DIV_VAL			(0x0)

#define QCOM_USB_PHY_HS_PHY_CTRL2	(0x64)
#define VBUSVLDEXT0			BIT(0)
#define USB2_SUSPEND_N			BIT(2)
#define USB2_SUSPEND_N_SEL		BIT(3)
#define VBUS_DET_EXT_SEL		BIT(4)

#define QCOM_USB_PHY_CFG_CTRL_4		(0x68)
#define PHY_CFG_PLL_GMP_CNTRL_MASK	GENMASK(1, 0)
#define PHY_CFG_PLL_INT_CNTRL_MASK	GENMASK(7, 2)

#define QCOM_USB_PHY_CFG_CTRL_5		(0x6c)
#define PHY_CFG_PLL_PROP_CNTRL_MASK	GENMASK(4, 0)
#define PHY_CFG_PLL_VREF_TUNE_MASK	GENMASK(7, 6)

#define QCOM_USB_PHY_CFG_CTRL_6		(0x70)
#define PHY_CFG_PLL_VCO_CNTRL_MASK	GENMASK(2, 0)

#define QCOM_USB_PHY_CFG_CTRL_7		(0x74)

#define QCOM_USB_PHY_CFG_CTRL_8		(0x78)
#define PHY_CFG_TX_FSLS_VREF_TUNE_MASK	GENMASK(1, 0)
#define PHY_CFG_TX_FSLS_VREG_BYPASS	BIT(2)
#define PHY_CFG_TX_HS_VREF_TUNE_MASK	GENMASK(5, 3)
#define PHY_CFG_TX_HS_XV_TUNE_MASK	GENMASK(7, 6)

#define QCOM_USB_PHY_CFG_CTRL_9		(0x7c)
#define PHY_CFG_TX_PREEMP_TUNE_MASK	GENMASK(2, 0)
#define PHY_CFG_TX_RES_TUNE_MASK	GENMASK(4, 3)
#define PHY_CFG_TX_RISE_TUNE_MASK	GENMASK(6, 5)
#define PHY_CFG_RCAL_BYPASS		BIT(7)

#define QCOM_USB_PHY_CFG_CTRL_10	(0x80)

#define QCOM_USB_PHY_CFG0		(0x94)
#define DATAPATH_CTRL_OVERRIDE_EN	BIT(0)
#define CMN_CTRL_OVERRIDE_EN		BIT(1)

#define QCOM_UTMI_PHY_CMN_CTRL0		(0x98)
#define TESTBURNIN			BIT(6)

#define QCOM_USB_PHY_FSEL_SEL		(0xb8)
#define FSEL_SEL			BIT(0)

#define QCOM_USB_PHY_APB_ACCESS_CMD	(0x130)
#define RW_ACCESS			BIT(0)
#define APB_START_CMD			BIT(1)
#define APB_LOGIC_RESET			BIT(2)

#define QCOM_USB_PHY_APB_ACCESS_STATUS	(0x134)
#define ACCESS_DONE			BIT(0)
#define TIMED_OUT			BIT(1)
#define ACCESS_ERROR			BIT(2)
#define ACCESS_IN_PROGRESS		BIT(3)

#define QCOM_USB_PHY_APB_ADDRESS	(0x138)
#define APB_REG_ADDR_MASK		GENMASK(7, 0)

#define QCOM_USB_PHY_APB_WRDATA_LSB	(0x13c)
#define APB_REG_WRDATA_7_0_MASK		GENMASK(3, 0)

#define QCOM_USB_PHY_APB_WRDATA_MSB	(0x140)
#define APB_REG_WRDATA_15_8_MASK	GENMASK(7, 4)

#define QCOM_USB_PHY_APB_RDDATA_LSB	(0x144)
#define APB_REG_RDDATA_7_0_MASK		GENMASK(3, 0)

#define QCOM_USB_PHY_APB_RDDATA_MSB	(0x148)
#define APB_REG_RDDATA_15_8_MASK	GENMASK(7, 4)

static const char * const eusb2_hsphy_vreg_names[] = {
	"vdd", "vdda12",
};

#define EUSB2_NUM_VREGS		ARRAY_SIZE(eusb2_hsphy_vreg_names)

struct snps_eusb2_phy_drvdata {
	int (*phy_init)(struct phy *p);
	const char * const *clk_names;
	int num_clks;
};

struct snps_eusb2_hsphy {
	struct phy *phy;
	void __iomem *base;

	struct clk *ref_clk;
	struct clk_bulk_data *clks;
	struct reset_control *phy_reset;

	struct regulator_bulk_data vregs[EUSB2_NUM_VREGS];

	enum phy_mode mode;

	struct phy *repeater;

	/*
	 * M2582: the factory node has three regions, and the vendor's init calls
	 * msm_eusb2_phy_power() before its register sequence -- a function that
	 * writes eud_enable_reg, the second region. Map it so the same thing can
	 * be done here; NULL when the device tree supplies only the first region,
	 * which keeps every other platform working.
	 */
	void __iomem *eud_enable;

	const struct snps_eusb2_phy_drvdata *data;

	/*
	 * M2582: the vendor's eusb2 PHY applies a "parameter override" sequence
	 * after the PHY is up. From disassembly of phy-msm-snps-eusb2.ko:
	 *
	 *     value  = seq[i]
	 *     offset = seq[i + 1]
	 *     writel(value, base + offset)
	 *
	 * i.e. the device tree list is (value, offset) pairs, and the factory node
	 * carries <0x00 0x58>, which clears register 0x58. Without it the HS link
	 * does not hold: DSTS drops from high speed (0x0) to full speed (0x1)
	 * straight after CONNECT_DONE, the host gives up, and EP0 never sees a
	 * setup packet.
	 */
	u32 *param_override_seq;
	int param_override_seq_cnt;

	/*
	 * M2582: the vendor's eusb2 node has a third supply, vdd_refgen, feeding
	 * the PHY's reference generator (measured in the factory device tree as
	 * the PMXR2230 L2B rail, 0.88 V). Mainline's driver only knows "vdd" and
	 * "vdda12", so this rail was never enabled -- and a dead reference
	 * generator leaves the UTMI/data path without its clock while the PLL and
	 * the link state machine keep working. That is exactly this board: RESET,
	 * CONNECT_DONE and SUSPEND all arrive, but no packet ever reaches the
	 * core. Requested optionally so other platforms are unaffected.
	 */
	struct regulator *vdd_refgen;
};

static int snps_eusb2_hsphy_set_mode(struct phy *p, enum phy_mode mode, int submode)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);

	phy->mode = mode;

	return phy_set_mode_ext(phy->repeater, mode, submode);
}

/*
 * M2582: this PHY's registers are 8-bit and a 32-bit read presents the byte
 * replicated four times (0x90 reads as 0x90909090, 0x02 as 0x02020202). A
 * plain writel() therefore does not produce the intended value: writing 0xc8
 * into the low byte leaves whatever the other three lanes held, and the
 * read-back shows 0x0c0c0c0c rather than 0xc8c8c8c8 -- the value appears
 * shifted by four. Write the byte replicated across all four lanes instead, so
 * the hardware sees the same thing in every lane.
 */
static void m2582_write_byte_rep(void __iomem *base, u32 offset, u8 val)
{
	writel_relaxed((u32)val * 0x01010101u, base + offset);
}

/*
 * The vendor waits for each programming step to take effect before moving on:
 * after every register write it polls a specific bit until it reads back as
 * expected, and it also delays 43 ms after releasing UTMI POR. Mainline does
 * none of that -- it writes the whole sequence back to back and never confirms
 * anything -- which is consistent with an eUSB2 PLL that never locks even
 * though every value written matches the vendor's.
 */
static int m2582_poll(void __iomem *base, u32 offset, u32 mask, bool want_set,
		      int tries)
{
	int i;

	for (i = 0; i < tries; i++) {
		u32 v = readl_relaxed(base + offset);
		bool set = !!(v & mask);

		if (set == want_set)
			return 0;
		udelay(10);
	}
	return -ETIMEDOUT;
}

/*
 * M2582: the PHY's APB mailbox. Registers beyond the directly mapped window are
 * reached through it:
 *
 *   0x130  command   (1 = prepare, 3 = execute write, 0 = done)
 *   0x138  address   (which internal PHY register)
 *   0x13c  write data
 *   0x134  status
 *   0x144  read data
 *
 * Transcribed from the vendor's msm_eusb2_phy_notify_connect(), which runs at
 * CONNECT time -- not during init -- and writes internal register 5 twice, first
 * with 0xc0 and then with 0x00. That post-connect tuning is the one part of the
 * vendor's bring-up we had never replicated, and it is the most likely reason
 * this board's link comes up and then cannot carry a transfer.
 */
static void m2582_apb_write(struct snps_eusb2_hsphy *phy, u8 addr, u8 val)
{
	void __iomem *b = phy->base;
	u32 r;

	r = readl_relaxed(b + 0x130) & ~0xffu;
	writel_relaxed(r | 0x1, b + 0x130);

	r = readl_relaxed(b + 0x138) & ~0xffu;
	writel_relaxed(r | addr, b + 0x138);

	r = readl_relaxed(b + 0x13c) & ~0xffu;
	writel_relaxed(r | val, b + 0x13c);

	r = readl_relaxed(b + 0x130) & ~0xffu;
	writel_relaxed(r | 0x3, b + 0x130);

	r = readl_relaxed(b + 0x130) & ~0xffu;
	writel_relaxed(r, b + 0x130);
}

void m2582_eusb2_notify_connect(struct phy *p)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);

	if (!phy || !phy->base)
		return;

	m2582_apb_write(phy, 0x5, 0xc0);
	m2582_apb_write(phy, 0x5, 0x00);

	dev_info(&p->dev,
		 "M2582: connect-time APB tuning done -> 130=%08x 134=%08x 138=%08x 13c=%08x\n",
		 readl_relaxed(phy->base + 0x130), readl_relaxed(phy->base + 0x134),
		 readl_relaxed(phy->base + 0x138), readl_relaxed(phy->base + 0x13c));
}
EXPORT_SYMBOL_GPL(m2582_eusb2_notify_connect);

static void snps_eusb2_hsphy_write_mask(void __iomem *base, u32 offset,
					u32 mask, u32 val)
{
	u32 reg;

	reg = readl_relaxed(base + offset);
	reg &= ~mask;
	reg |= val & mask;
	writel_relaxed(reg, base + offset);

	/* Ensure above write is completed */
	readl_relaxed(base + offset);
}

static void qcom_eusb2_default_parameters(struct snps_eusb2_hsphy *phy)
{
	/* default parameters: tx pre-emphasis */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_PREEMP_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_PREEMP_TUNE_MASK, 0));

	/* tx rise/fall time */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_RISE_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_RISE_TUNE_MASK, 0x2));

	/* source impedance adjustment */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_RES_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_RES_TUNE_MASK, 0x1));

	/* dc voltage level adjustement */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_8,
				    PHY_CFG_TX_HS_VREF_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_HS_VREF_TUNE_MASK, 0x3));

	/* transmitter HS crossover adjustement */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_8,
				    PHY_CFG_TX_HS_XV_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_HS_XV_TUNE_MASK, 0x0));
}

struct snps_eusb2_ref_clk {
	unsigned long freq;
	u32 fsel_val;
	u32 div_7_0_val;
	u32 div_11_8_val;
};

static const struct snps_eusb2_ref_clk exynos_eusb2_ref_clk[] = {
	{ 19200000, FSEL_19_2_MHZ_VAL, DIV_19_8_19_2_MHZ_VAL, EXYNOS_DIV_11_8_19_2_MHZ_VAL },
	{ 20000000, FSEL_20_MHZ_VAL, DIV_19_8_20_MHZ_VAL, EXYNOS_DIV_11_8_20_MHZ_VAL },
	{ 24000000, FSEL_24_MHZ_VAL, DIV_19_8_24_MHZ_VAL, EXYNOS_DIV_11_8_24_MHZ_VAL },
	{ 26000000, FSEL_26_MHZ_VAL, DIV_19_8_26_MHZ_VAL, EXYNOS_DIV_11_8_26_MHZ_VAL },
	{ 48000000, FSEL_48_MHZ_VAL, DIV_19_8_48_MHZ_VAL, EXYNOS_DIV_11_8_48_MHZ_VAL },
};

static int exynos_eusb2_ref_clk_init(struct snps_eusb2_hsphy *phy)
{
	const struct snps_eusb2_ref_clk *config = NULL;
	unsigned long ref_clk_freq = clk_get_rate(phy->ref_clk);

	for (int i = 0; i < ARRAY_SIZE(exynos_eusb2_ref_clk); i++) {
		if (exynos_eusb2_ref_clk[i].freq == ref_clk_freq) {
			config = &exynos_eusb2_ref_clk[i];
			break;
		}
	}

	if (!config) {
		dev_err(&phy->phy->dev, "unsupported ref_clk_freq: %lu\n", ref_clk_freq);
		return -EINVAL;
	}

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_COMMON,
				    FSEL_MASK,
				    FIELD_PREP(FSEL_MASK, config->fsel_val));

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_CFG_PLLCFG0,
				    PHY_CFG_PLL_FB_DIV_19_8_MASK,
				    FIELD_PREP(PHY_CFG_PLL_FB_DIV_19_8_MASK,
					       config->div_7_0_val));

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_CFG_PLLCFG1,
				    EXYNOS_PHY_CFG_PLL_FB_DIV_11_8_MASK,
				    config->div_11_8_val);
	return 0;
}

static const struct snps_eusb2_ref_clk qcom_eusb2_ref_clk[] = {
	{ 19200000, FSEL_19_2_MHZ_VAL, DIV_7_0_19_2_MHZ_VAL, DIV_11_8_19_2_MHZ_VAL },
	{ 38400000, FSEL_38_4_MHZ_VAL, DIV_7_0_38_4_MHZ_VAL, DIV_11_8_38_4_MHZ_VAL },
};

static int qcom_eusb2_ref_clk_init(struct snps_eusb2_hsphy *phy)
{
	const struct snps_eusb2_ref_clk *config = NULL;
	unsigned long ref_clk_freq = clk_get_rate(phy->ref_clk);

	for (int i = 0; i < ARRAY_SIZE(qcom_eusb2_ref_clk); i++) {
		if (qcom_eusb2_ref_clk[i].freq == ref_clk_freq) {
			config = &qcom_eusb2_ref_clk[i];
			break;
		}
	}

	if (!config) {
		dev_err(&phy->phy->dev, "unsupported ref_clk_freq: %lu\n", ref_clk_freq);
		return -EINVAL;
	}

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL_COMMON0,
				    FSEL_MASK,
				    FIELD_PREP(FSEL_MASK, config->fsel_val));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_2,
				    PHY_CFG_PLL_FB_DIV_7_0_MASK,
				    config->div_7_0_val);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_3,
				    PHY_CFG_PLL_FB_DIV_11_8_MASK,
				    config->div_11_8_val);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_3,
				    PHY_CFG_PLL_REF_DIV, PLL_REF_DIV_VAL);

	return 0;
}

static int exynos_snps_eusb2_hsphy_init(struct phy *p)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);
	int ret;

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_RST,
				    USB_PHY_RST_MASK | UTMI_PORT_RST_MASK,
				    USB_PHY_RST_MASK | UTMI_PORT_RST_MASK);
	fsleep(50); /* required after holding phy in reset */

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_COMMON,
				    RPTR_MODE, RPTR_MODE);

	/* update ref_clk related registers */
	ret = exynos_eusb2_ref_clk_init(phy);
	if (ret)
		return ret;

	/* default parameter: tx fsls-vref */
	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_PHY_CFG_TX,
				    EXYNOS_PHY_CFG_TX_FSLS_VREF_TUNE_MASK,
				    FIELD_PREP(EXYNOS_PHY_CFG_TX_FSLS_VREF_TUNE_MASK, 0x0));

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_UTMI_TESTSE,
				    TEST_IDDQ, 0);
	fsleep(10); /* required after releasing test_iddq */

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_RST,
				    USB_PHY_RST_MASK, 0);

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_COMMON,
				    PHY_ENABLE, PHY_ENABLE);

	snps_eusb2_hsphy_write_mask(phy->base, EXYNOS_USB_PHY_HS_PHY_CTRL_RST,
				    UTMI_PORT_RST_MASK, 0);

	return 0;
}

static const char * const exynos_eusb2_hsphy_clock_names[] = {
	"ref", "bus", "ctrl",
};

static const struct snps_eusb2_phy_drvdata exynos2200_snps_eusb2_phy = {
	.phy_init	= exynos_snps_eusb2_hsphy_init,
	.clk_names	= exynos_eusb2_hsphy_clock_names,
	.num_clks	= ARRAY_SIZE(exynos_eusb2_hsphy_clock_names),
};

static int qcom_snps_eusb2_hsphy_init(struct phy *p)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);
	int ret, i;

	/*
	 * M2582: the vendor's eUSB2 sequence, transcribed instruction by
	 * instruction from phy-msm-snps-eusb2.ko's msm_eusb2_phy_init() and
	 * replayed verbatim, in order, before anything mainline does.
	 *
	 * Measurements that motivated this:
	 *   - the register block is writable (0x54 went 0x42 -> 0x4b when we
	 *     re-asserted PHY_ENABLE|RETENABLEN);
	 *   - PLL_STATUS (0x14 bit 6) stays 0 through 1000 polls of 10 us, i.e.
	 *     the PLL never locks;
	 *   - our own sequence *clears* 0x58 (it reads 0x02 before init and 0x00
	 *     after), while the vendor sets it to 0x02 at a specific point;
	 *   - POWER_DOWN_CONTROL (0x40) and START_CONTROL (0x44) both read 0, so
	 *     the PHY is never actually started.
	 *
	 * An unlocked PLL leaves the UTMI data path without a clock, which is
	 * exactly this board: RESET/CONNECT_DONE arrive, no packet ever does.
	 */
	{
		u32 r;

		/*
		 * The vendor's very first action, before touching any PHY register:
		 * msm_eusb2_phy_power(phy, 1), which writes 1 to eud_enable_reg.
		 */
		if (phy->eud_enable) {
			writel_relaxed(1, phy->eud_enable);
			dev_info(&p->dev, "M2582: eud_enable written (region 1 mapped)\n");
		} else {
			dev_warn(&p->dev, "M2582: no eud_enable region mapped\n");
		}

		/*
		 * The vendor's first block, with its handshakes. After each write it
		 * polls a specific bit until it reads back as expected, and it delays
		 * 43 ms after releasing UTMI POR:
		 *
		 *   0x94 |= 2  -> poll bit 1 set
		 *   0x50 |= 2  -> poll bit 1 set
		 *   udelay(42950)
		 *   0x54 |= 9  -> poll
		 *   0x130 |= 4 -> poll bit 2 set   (APB logic reset done)
		 *   0x98 &= ~0x40 -> poll bit 6 clear
		 *   0xb8 |= 1  -> poll bit 0 set
		 *
		 * Mainline skips every one of these, which is why its register writes
		 * all look correct while the PLL never locks.
		 */
		r = readl_relaxed(phy->base + 0x94) | 0x2;
		writel_relaxed(r, phy->base + 0x94);
		m2582_poll(phy->base, 0x94, BIT(1), true, 1000);

		r = readl_relaxed(phy->base + 0x50) | 0x2;
		writel_relaxed(r, phy->base + 0x50);
		m2582_poll(phy->base, 0x50, BIT(1), true, 1000);

		/*
		 * The vendor's settle: mov w0, #0xa7c6 ; __const_udelay, i.e. 42950
		 * microseconds of raw busy-wait. udelay() rejects values that large,
		 * so sleep instead -- 43 ms is far longer than any scheduling delay.
		 */
		msleep(43);

		r = readl_relaxed(phy->base + 0x54) | 0x9;
		writel_relaxed(r, phy->base + 0x54);
		m2582_poll(phy->base, 0x54, 0x9, true, 1000);

		r = readl_relaxed(phy->base + 0x130) | 0x4;
		writel_relaxed(r, phy->base + 0x130);
		m2582_poll(phy->base, 0x130, BIT(2), true, 1000);

		r = readl_relaxed(phy->base + 0x98) & ~0x40;
		writel_relaxed(r, phy->base + 0x98);
		m2582_poll(phy->base, 0x98, BIT(6), false, 1000);

		r = readl_relaxed(phy->base + 0xb8) | 0x1;
		writel_relaxed(r, phy->base + 0xb8);
		m2582_poll(phy->base, 0xb8, BIT(0), true, 1000);

		/*
		 * Reference-clock dependent branch. Our device tree reports 19.2 MHz
		 * (unless qcom,ref-clk-38m4 is set), and the vendor's values are
		 * 19.2 MHz -> 0x5c=0x90, 38.4 MHz -> 0x5c=0xc8.
		 */
		if (clk_get_rate(phy->ref_clk) == 38400000) {
			r = readl_relaxed(phy->base + 0x54) & ~0x8f;
			r |= 0x40;
			writel_relaxed(r, phy->base + 0x54);

			m2582_write_byte_rep(phy->base, 0x5c, 0xc8);
		} else {
			r = readl_relaxed(phy->base + 0x54) & ~0x70;
			writel_relaxed(r, phy->base + 0x54);

			m2582_write_byte_rep(phy->base, 0x5c, 0x90);
			m2582_write_byte_rep(phy->base, 0x60, 0x01);
		}

		/* 0x60 &= ~0xf0, then 0x58 |= 0x2 (byte-replicated writes) */
		m2582_write_byte_rep(phy->base, 0x60,
				     (u8)(readl_relaxed(phy->base + 0x60) & ~0xf0));
		m2582_write_byte_rep(phy->base, 0x58,
				     (u8)(readl_relaxed(phy->base + 0x58) | 0x2));

		/* PLL integer / GMP control, each step confirmed like the vendor */
		r = readl_relaxed(phy->base + 0x68) | 0x20;
		writel_relaxed(r, phy->base + 0x68);
		m2582_poll(phy->base, 0x68, BIT(5), true, 1000);

		r = readl_relaxed(phy->base + 0x68) | 0x1;
		writel_relaxed(r, phy->base + 0x68);
		m2582_poll(phy->base, 0x68, BIT(0), true, 1000);

		/* PLL proportional / VCO control */
		r = readl_relaxed(phy->base + 0x6c) | 0x10;
		writel_relaxed(r, phy->base + 0x6c);
		m2582_poll(phy->base, 0x6c, BIT(4), true, 1000);

		r = readl_relaxed(phy->base + 0x70) & ~0x7;
		writel_relaxed(r, phy->base + 0x70);

		r = readl_relaxed(phy->base + 0x6c) | 0x40;
		writel_relaxed(r, phy->base + 0x6c);
		m2582_poll(phy->base, 0x6c, BIT(6), true, 1000);

		/* HS_PHY_CTRL2 bits[3:2], then the vendor's tail */
		r = readl_relaxed(phy->base + 0x64) | 0xc;
		writel_relaxed(r, phy->base + 0x64);

		r = readl_relaxed(phy->base + 0x3c) | 0x1;
		writel_relaxed(r, phy->base + 0x3c);
		r = readl_relaxed(phy->base + 0x50) & ~0x2;
		writel_relaxed(r, phy->base + 0x50);
		r = readl_relaxed(phy->base + 0x64) & ~0x8;
		writel_relaxed(r, phy->base + 0x64);
		r = readl_relaxed(phy->base + 0x64) & ~0x2;
		writel_relaxed(r, phy->base + 0x64);

		/* 0x7c: clear[2:0], set bit6, set bit3 */
		r = readl_relaxed(phy->base + 0x7c) & ~0x7;
		writel_relaxed(r, phy->base + 0x7c);
		r = readl_relaxed(phy->base + 0x7c) | 0x40;
		writel_relaxed(r, phy->base + 0x7c);
		r = readl_relaxed(phy->base + 0x7c) | 0x8;
		writel_relaxed(r, phy->base + 0x7c);

		/* 0x78: set bits 3,4 ; clear bits 6,7 */
		r = readl_relaxed(phy->base + 0x78) | 0x18;
		writel_relaxed(r, phy->base + 0x78);
		r = readl_relaxed(phy->base + 0x78) & ~0xc0;
		writel_relaxed(r, phy->base + 0x78);

		dev_info(&p->dev,
			 "M2582: vendor sequence replayed -> 40=%08x 44=%08x 50=%08x 54=%08x 58=%08x 5c=%08x 60=%08x 14=%08x\n",
			 readl_relaxed(phy->base + 0x40), readl_relaxed(phy->base + 0x44),
			 readl_relaxed(phy->base + 0x50), readl_relaxed(phy->base + 0x54),
			 readl_relaxed(phy->base + 0x58), readl_relaxed(phy->base + 0x5c),
			 readl_relaxed(phy->base + 0x60), readl_relaxed(phy->base + 0x14));
	}

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG0,
				    CMN_CTRL_OVERRIDE_EN, CMN_CTRL_OVERRIDE_EN);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_UTMI_CTRL5, POR, POR);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL_COMMON0,
				    PHY_ENABLE | RETENABLEN, PHY_ENABLE | RETENABLEN);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_APB_ACCESS_CMD,
				    APB_LOGIC_RESET, APB_LOGIC_RESET);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_UTMI_PHY_CMN_CTRL0, TESTBURNIN, 0);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_FSEL_SEL,
				    FSEL_SEL, FSEL_SEL);

	/* update ref_clk related registers */
	ret = qcom_eusb2_ref_clk_init(phy);
	if (ret)
		return ret;

	/*
	 * M2582: the vendor's TUNA-specific data-path tuning, transcribed from
	 * the disassembly of phy-msm-snps-eusb2.ko's msm_eusb2_phy_init().
	 *
	 * The vendor branches on the reference clock rate and writes registers
	 * that mainline never touches at all:
	 *
	 *   ref = 19200000 (ours):
	 *       +0x54 &= ~0x70
	 *       +0x5c  = (old & ~0xff) | 0x90
	 *       +0x60  = (old & ~0xf)  | 0x1
	 *   ref = 38400000:
	 *       +0x54 &= ~0x8f ; +0x54 |= 0x40
	 *       +0x5c  = (old & ~0xff) | 0xc8
	 *
	 *   both:  +0x60 &= ~0xf0 ; +0x58 |= 0x2 ; +0x7c &= ~0x7
	 *
	 * qcom_eusb2_ref_clk_init() only programs 0x54's FSEL and the CFG_CTRL_2/3
	 * PLL dividers, so 0x5c, 0x60, 0x58 and 0x7c were left at their reset
	 * values. The symptom of that is exactly what this board shows: the link
	 * state machine works (RESET/CONNECT_DONE/SUSPEND all arrive) but no packet
	 * ever reaches the core, so EP0 never raises XferNotReady(Setup) and no
	 * endpoint event is ever generated.
	 *
	 * 0x58 is also the register the factory device tree patches through
	 * qcom,param-override-seq = <0x00 0x58>.
	 */
	/*
	 * M2582: DISABLED -- this "vendor datapath tune" writes values that do not
	 * match the vendor's working PHY, and on two registers it actively destroys
	 * a state that was already correct.
	 *
	 * Measured, side by side (our post-write readback vs the live registers read
	 * from inside the running vendor kernel):
	 *
	 *   reg    our pre-init   after this tune   vendor live (working)
	 *   0x54   09             06                4b
	 *   0x58   02             02                00
	 *   0x5c   c8             09                 c8     <- vendor == our pre-init
	 *   0x60   00             01                 00     <- vendor == our pre-init
	 *   0x64   07             15                 17
	 *
	 * On 0x5c and 0x60 the vendor's healthy state is exactly what the PHY
	 * already has before this block runs, so the block is a regression: it
	 * overwrites a correct configuration with different values. It is also a
	 * late addition to this driver, which fits the observation that the board
	 * enumerated once at 09:58 and never did again.
	 *
	 * Leave the PHY as mainline left it. Re-enable only with fresh evidence that
	 * one of these values is genuinely needed, and change one register at a time.
	 */
	/*
	 * M2582: write the vendor's LIVE register values, read out of its running
	 * kernel, and log what the PHY had before.
	 *
	 * With the tune block above disabled the PHY keeps its pre-init state, which
	 * already matches the vendor on 0x5c (c8) and 0x60 (00). Four registers are
	 * still different, and the host's "new high-speed device ... then new
	 * full-speed device" flip-flop is what an incorrectly configured USB2 analog
	 * front end looks like:
	 *
	 *   reg    ours   vendor live
	 *   0x54   09     4b
	 *   0x58   00     00
	 *   0x64   07     17
	 *   0x68   ?      21
	 *
	 * Write exactly the vendor's values and see whether the link then holds high
	 * speed long enough to answer GET_DESCRIPTOR.
	 */
	{
		static const struct { u8 off, val; } v[] = {
			{ 0x54, 0x4b }, { 0x58, 0x00 }, { 0x64, 0x17 }, { 0x68, 0x21 },
		};
		char l[256];
		int n = 0, k;

		n += scnprintf(l + n, sizeof(l) - n, "M2582: eusb2 before");
		for (k = 0; k < ARRAY_SIZE(v); k++)
			n += scnprintf(l + n, sizeof(l) - n, " %02x=%02x",
				       v[k].off, (u8)readl_relaxed(phy->base + v[k].off));
		dev_info(&p->dev, "%s\n", l);

		for (k = 0; k < ARRAY_SIZE(v); k++)
			m2582_write_byte_rep(phy->base, v[k].off, v[k].val);

		n = 0;
		n += scnprintf(l + n, sizeof(l) - n, "M2582: eusb2 after ");
		for (k = 0; k < ARRAY_SIZE(v); k++)
			n += scnprintf(l + n, sizeof(l) - n, " %02x=%02x",
				       v[k].off, (u8)readl_relaxed(phy->base + v[k].off));
		dev_info(&p->dev, "%s\n", l);
	}

	if (0) {
		unsigned long rate = clk_get_rate(phy->ref_clk);
		u32 r;

		/*
		 * M2582: the factory eusb2 node names its clocks "ref_clk_src" /
		 * "ref_clk" while our tuna drvdata expects "ref" / "ref_gate", and
		 * the two are not the same source: ours is a 19.2 MHz fixed clock on
		 * the TCSR USB2 gate, the factory's second clock is a GCC one. The
		 * vendor driver picks its tuning from clk_get_rate(ref_clk), and the
		 * two branches write different PHY values, so this is a genuine
		 * unknown. Allow forcing the 38.4 MHz branch with a device-tree flag
		 * so both can be measured without rebuilding.
		 */
		if (of_property_read_bool(p->dev.of_node, "qcom,ref-clk-38m4"))
			rate = 38400000;

		if (rate == 19200000) {
			r = readl_relaxed(phy->base + 0x54) & ~0x70;
			writel_relaxed(r, phy->base + 0x54);

			m2582_write_byte_rep(phy->base, 0x5c, 0x90);
			m2582_write_byte_rep(phy->base, 0x60, 0x01);
		} else if (rate == 38400000) {
			r = readl_relaxed(phy->base + 0x54) & ~0x8f;
			r |= 0x40;
			writel_relaxed(r, phy->base + 0x54);

			m2582_write_byte_rep(phy->base, 0x5c, 0xc8);
		} else {
			dev_warn(&p->dev, "M2582: unexpected ref rate %lu\n", rate);
		}

		m2582_write_byte_rep(phy->base, 0x60,
				     (u8)(readl_relaxed(phy->base + 0x60) & ~0xf0));
		m2582_write_byte_rep(phy->base, 0x58,
				     (u8)(readl_relaxed(phy->base + 0x58) | 0x2));

		r = readl_relaxed(phy->base + 0x6c) | 0x40;
		writel_relaxed(r, phy->base + 0x6c);

		r = readl_relaxed(phy->base + 0x64) | 0x10;
		writel_relaxed(r, phy->base + 0x64);

		r = readl_relaxed(phy->base + 0x7c) & ~0x7;
		writel_relaxed(r, phy->base + 0x7c);
		r = readl_relaxed(phy->base + 0x7c) | 0x40;
		writel_relaxed(r, phy->base + 0x7c);
		r = readl_relaxed(phy->base + 0x7c) | 0x8;
		writel_relaxed(r, phy->base + 0x7c);

		r = readl_relaxed(phy->base + 0x78) | 0x18;
		writel_relaxed(r, phy->base + 0x78);
		r = readl_relaxed(phy->base + 0x78) & ~0xc0;
		writel_relaxed(r, phy->base + 0x78);

		/*
		 * The vendor's tail, transcribed verbatim. Mainline writes these
		 * same registers but with different bit values, so the PHY does not
		 * end up in the state the vendor leaves it in.
		 */
		r = readl_relaxed(phy->base + 0x64) | 0xc;
		writel_relaxed(r, phy->base + 0x64);

		r = readl_relaxed(phy->base + 0x3c) | 0x1;
		writel_relaxed(r, phy->base + 0x3c);

		r = readl_relaxed(phy->base + 0x54) | 0x2;
		writel_relaxed(r, phy->base + 0x54);
		r = readl_relaxed(phy->base + 0x54) & ~0x4;
		writel_relaxed(r, phy->base + 0x54);

		r = readl_relaxed(phy->base + 0x50) & ~0x2;
		writel_relaxed(r, phy->base + 0x50);

		r = readl_relaxed(phy->base + 0x64) & ~0x8;
		writel_relaxed(r, phy->base + 0x64);
		r = readl_relaxed(phy->base + 0x64) & ~0x2;
		writel_relaxed(r, phy->base + 0x64);

		dev_info(&p->dev,
			 "M2582: vendor datapath tune at %lu Hz -> 54=%08x 58=%08x 5c=%08x 60=%08x 64=%08x 6c=%08x 78=%08x 7c=%08x\n",
			 rate,
			 readl_relaxed(phy->base + 0x54),
			 readl_relaxed(phy->base + 0x58),
			 readl_relaxed(phy->base + 0x5c),
			 readl_relaxed(phy->base + 0x60),
			 readl_relaxed(phy->base + 0x64),
			 readl_relaxed(phy->base + 0x6c),
			 readl_relaxed(phy->base + 0x78),
			 readl_relaxed(phy->base + 0x7c));
	}

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_1,
				    PHY_CFG_PLL_CPBIAS_CNTRL_MASK,
				    FIELD_PREP(PHY_CFG_PLL_CPBIAS_CNTRL_MASK, 0x0));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_4,
				    PHY_CFG_PLL_INT_CNTRL_MASK,
				    FIELD_PREP(PHY_CFG_PLL_INT_CNTRL_MASK, 0x8));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_4,
				    PHY_CFG_PLL_GMP_CNTRL_MASK,
				    FIELD_PREP(PHY_CFG_PLL_GMP_CNTRL_MASK, 0x1));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_5,
				    PHY_CFG_PLL_PROP_CNTRL_MASK,
				    FIELD_PREP(PHY_CFG_PLL_PROP_CNTRL_MASK, 0x10));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_6,
				    PHY_CFG_PLL_VCO_CNTRL_MASK,
				    FIELD_PREP(PHY_CFG_PLL_VCO_CNTRL_MASK, 0x0));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_5,
				    PHY_CFG_PLL_VREF_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_PLL_VREF_TUNE_MASK, 0x1));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL2,
				    VBUS_DET_EXT_SEL, VBUS_DET_EXT_SEL);

	/*
	 * M2582: the vendor's TX tuning registers are all ZERO on this board --
	 * read straight off the running vendor stack through its debugfs:
	 *
	 *     /sys/kernel/debug/88e3000.hsphy/tx_pre_emphasis   = 0x00
	 *     /sys/kernel/debug/88e3000.hsphy/tx_rise_fall_time = 0x00
	 *     /sys/kernel/debug/88e3000.hsphy/tx_src_imp        = 0x00
	 *     /sys/kernel/debug/88e3000.hsphy/tx_dc_vref        = 0x00
	 *     /sys/kernel/debug/88e3000.hsphy/tx_xv             = 0x00
	 *
	 * mainline's qcom_eusb2_default_parameters() writes non-zero values for
	 * three of those (rise/fall time 0x2, source impedance 0x1, DC vref 0x3),
	 * which shape the high-speed transmit signal -- rise time, source
	 * impedance and DC level. On this board that is a real deviation from a
	 * known-working configuration and a plausible reason the link comes up and
	 * then cannot carry a transfer.
	 *
	 * Zero them to match the vendor, keeping the writes in the same registers
	 * rather than skipping the call so the intent is explicit.
	 */
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_PREEMP_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_PREEMP_TUNE_MASK, 0));
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_RISE_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_RISE_TUNE_MASK, 0));
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_9,
				    PHY_CFG_TX_RES_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_RES_TUNE_MASK, 0));
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_8,
				    PHY_CFG_TX_HS_VREF_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_HS_VREF_TUNE_MASK, 0));
	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG_CTRL_8,
				    PHY_CFG_TX_HS_XV_TUNE_MASK,
				    FIELD_PREP(PHY_CFG_TX_HS_XV_TUNE_MASK, 0));

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL2,
				    USB2_SUSPEND_N_SEL | USB2_SUSPEND_N,
				    USB2_SUSPEND_N_SEL | USB2_SUSPEND_N);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_UTMI_CTRL0, SLEEPM, SLEEPM);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL_COMMON0,
				    SIDDQ_SEL, SIDDQ_SEL);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL_COMMON0,
				    SIDDQ, 0);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_UTMI_CTRL5, POR, 0);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_HS_PHY_CTRL2,
				    USB2_SUSPEND_N_SEL, 0);

	snps_eusb2_hsphy_write_mask(phy->base, QCOM_USB_PHY_CFG0,
				    CMN_CTRL_OVERRIDE_EN, 0);

	/*
	 * M2582: poll the eUSB2 PLL for lock and dump the state either way.
	 *
	 * The vendor polls bit 6 of 0x14 for up to 1000 iterations after its
	 * sequence. Nothing in mainline's driver ever checks it, which is why the
	 * board looks healthy while the PHY is in fact unlocked: an unlocked PLL
	 * leaves the UTMI data path without its clock, the link state machine
	 * still reports RESET/CONNECT_DONE, and no packet ever reaches the core.
	 */
	{
		int i;
		u32 st = 0;

		/*
		 * Poll far longer than the vendor does (500 ms rather than its 10 ms)
		 * and log the value as it changes. If a PLL with a marginal reference
		 * merely takes longer to lock, this will show it; if 0x14 never moves
		 * at all, the PLL is not running rather than merely slow.
		 */
		for (i = 0; i < 5000; i++) {
			st = readl_relaxed(phy->base + 0x14);
			if (st & BIT(6))
				break;
			if (i == 500 || i == 1500 || i == 3000)
				dev_dbg(&p->dev, "M2582: PLL 0x14 still %08x at %d ms\n",
					 st, i / 10);
			udelay(100);
		}

		dev_dbg(&p->dev,
			 "M2582: eusb2 PLL lock: 0x14=%08x locked=%d after %d polls\n",
			 st, !!(st & BIT(6)), i);
		dev_info(&p->dev,
			 "M2582: eusb2 post-init 00=%08x 04=%08x 08=%08x 0c=%08x 10=%08x 14=%08x 18=%08x\n",
			 readl_relaxed(phy->base + 0x00), readl_relaxed(phy->base + 0x04),
			 readl_relaxed(phy->base + 0x08), readl_relaxed(phy->base + 0x0c),
			 readl_relaxed(phy->base + 0x10), readl_relaxed(phy->base + 0x14),
			 readl_relaxed(phy->base + 0x18));
		dev_info(&p->dev,
			 "M2582: eusb2 post-init 40=%08x 44=%08x 48=%08x 4c=%08x 50=%08x 54=%08x 58=%08x 5c=%08x 60=%08x\n",
			 readl_relaxed(phy->base + 0x40), readl_relaxed(phy->base + 0x44),
			 readl_relaxed(phy->base + 0x48), readl_relaxed(phy->base + 0x4c),
			 readl_relaxed(phy->base + 0x50), readl_relaxed(phy->base + 0x54),
			 readl_relaxed(phy->base + 0x58), readl_relaxed(phy->base + 0x5c),
			 readl_relaxed(phy->base + 0x60));
	}

	/*
	 * M2582: re-assert PHY_ENABLE and RETENABLEN (0x54 bits 0 and 3) now that
	 * everything else is programmed, and verify it sticks.
	 *
	 * Mainline does set PHY_ENABLE|RETENABLEN early in this sequence, but the
	 * post-init read-back of 0x54 is 0x42 -- bits 0 and 3 are gone. Without
	 * PHY_ENABLE the eUSB2 PLL cannot lock (0x14 bit 6 stays 0), and without a
	 * locked PLL the UTMI data path has no clock: the link state machine still
	 * reports RESET and CONNECT_DONE while no packet ever reaches the core.
	 */
	{
		u32 before = readl_relaxed(phy->base + 0x54);
		u32 after;

		writel_relaxed(before | 0x9, phy->base + 0x54);
		after = readl_relaxed(phy->base + 0x54);
		dev_info(&p->dev,
			 "M2582: eusb2 0x54 re-enable: before=%08x after=%08x (want bit0|bit3)\n",
			 before, after);
	}

	/* M2582: apply the vendor's (value, offset) override pairs. */
	for (i = 0; i + 1 < phy->param_override_seq_cnt; i += 2) {
		writel_relaxed(phy->param_override_seq[i],
			       phy->base + phy->param_override_seq[i + 1]);
		dev_info(&p->dev, "M2582: param override %08x -> +0x%x\n",
			 phy->param_override_seq[i],
			 phy->param_override_seq[i + 1]);
	}

	return 0;
}

static const char * const qcom_eusb2_hsphy_clock_names[] = {
	"ref",
};

static const struct snps_eusb2_phy_drvdata sm8550_snps_eusb2_phy = {
	.phy_init	= qcom_snps_eusb2_hsphy_init,
	.clk_names      = qcom_eusb2_hsphy_clock_names,
	.num_clks       = ARRAY_SIZE(qcom_eusb2_hsphy_clock_names),
};

/* TUNA also gates its 38.4 MHz reference through TCSR. */
static const char * const tuna_eusb2_clock_names[] = { "ref", "ref_gate" };
static const struct snps_eusb2_phy_drvdata tuna_snps_eusb2_phy = {
	.phy_init = qcom_snps_eusb2_hsphy_init,
	.clk_names = tuna_eusb2_clock_names,
	.num_clks = ARRAY_SIZE(tuna_eusb2_clock_names),
};

static int snps_eusb2_hsphy_init(struct phy *p)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(phy->vregs), phy->vregs);
	if (ret)
		return ret;

	/*
	 * M2582: open the TCSR USB2 clock-reference gate before touching the PHY.
	 *
	 * Our device tree feeds this PHY two fixed-clock stubs instead of the real
	 * TCSR gate, on the theory (written into tuna.dtsi) that "the TCSR block is
	 * closed to the non-secure kernel". That theory is wrong: the QMP combo
	 * driver already pokes the same block successfully, which is how the SS
	 * PHY's PLL ever locks. With the gate shut the eUSB2 register file does not
	 * respond at all -- it reads back repeated-byte patterns such as 0x42424242
	 * and 0x0c0c0c0c, which is what an unclocked block looks like -- and the
	 * data path never comes up, which is exactly this board's symptom: link
	 * state events arrive but no packet ever reaches the core.
	 *
	 * TCSR_USB2_CLKREF_EN is bit 0 of 0x1fbf004.
	 */
	{
		void __iomem *tcsr = ioremap(0x01fbf000, 0x20);

		if (tcsr) {
			u32 before = readl_relaxed(tcsr + 0x04);

			writel_relaxed(before | BIT(0), tcsr + 0x04);
			dev_info(&p->dev,
				 "M2582: TCSR usb2 clkref 0x04: before=%08x after=%08x\n",
				 before, readl_relaxed(tcsr + 0x04));
			iounmap(tcsr);
		}
	}

	/*
	 * M2582: report the reference-generator rail but do NOT switch it here.
	 *
	 * Enabling it at this point made the DWC3 core soft reset time out
	 * (probe returned -110, "DWC3 controller soft reset failed"), so the rail
	 * is instead left always-on from the device tree via regulator-always-on.
	 * All this does is make the state visible in the report.
	 */
	if (phy->vdd_refgen)
		dev_info(&p->dev, "M2582: vdd_refgen present, %d uV, is_enabled=%d\n",
			 regulator_get_voltage(phy->vdd_refgen),
			 regulator_is_enabled(phy->vdd_refgen));
	else
		dev_warn(&p->dev, "M2582: no vdd_refgen supply in the device tree\n");

	/* M2582: is the companion repeater actually present and initialised? */
	dev_info(&p->dev, "M2582: eusb2 init, repeater=%px\n", phy->repeater);

	ret = phy_init(phy->repeater);
	if (ret) {
		dev_err(&p->dev, "repeater init failed: %d\n", ret);
		goto disable_vreg;
	}
	dev_info(&p->dev, "M2582: eusb2 repeater init ok\n");

	ret = clk_bulk_prepare_enable(phy->data->num_clks, phy->clks);
	if (ret) {
		dev_err(&p->dev, "failed to enable ref clock: %d\n", ret);
		goto exit_repeater;
	}

	/*
	 * M2582: read-only snapshot of the PHY before anything is written, so the
	 * pristine state (PLL status, power state, the tuning registers mainline
	 * never touches) is visible instead of inferred.
	 */
	dev_dbg(&p->dev,
		 "M2582: eusb2 pre-init 00=%08x 04=%08x 08=%08x 0c=%08x 14=%08x 18=%08x\n",
		 readl_relaxed(phy->base + 0x00), readl_relaxed(phy->base + 0x04),
		 readl_relaxed(phy->base + 0x08), readl_relaxed(phy->base + 0x0c),
		 readl_relaxed(phy->base + 0x14), readl_relaxed(phy->base + 0x18));
	/*
	 * M2582: is the register block writable at all? Write a distinctive value
	 * to the (currently zero) APB access command register and read it straight
	 * back. If it does not stick, the PHY is in an access-inhibited state and
	 * none of the programming above is reaching the hardware.
	 */
	{
		u32 before = readl_relaxed(phy->base + 0x130);

		writel_relaxed(0x00000004, phy->base + 0x130);
		dev_dbg(&p->dev,
			 "M2582: writable test reg130 before=%08x after=%08x (raw readback)\n",
			 before, readl_relaxed(phy->base + 0x130));
	}

	dev_dbg(&p->dev,
		 "M2582: eusb2 pre-init 3c=%08x 50=%08x 54=%08x 58=%08x 5c=%08x 60=%08x 64=%08x 6c=%08x 70=%08x 78=%08x 7c=%08x 94=%08x 98=%08x b8=%08x 130=%08x\n",
		 readl_relaxed(phy->base + 0x3c), readl_relaxed(phy->base + 0x50),
		 readl_relaxed(phy->base + 0x54), readl_relaxed(phy->base + 0x58),
		 readl_relaxed(phy->base + 0x5c), readl_relaxed(phy->base + 0x60),
		 readl_relaxed(phy->base + 0x64), readl_relaxed(phy->base + 0x6c),
		 readl_relaxed(phy->base + 0x70), readl_relaxed(phy->base + 0x78),
		 readl_relaxed(phy->base + 0x7c), readl_relaxed(phy->base + 0x94),
		 readl_relaxed(phy->base + 0x98), readl_relaxed(phy->base + 0xb8),
		 readl_relaxed(phy->base + 0x130));

	/*
	 * M2582: the reset pulse stays -- it is required.
	 *
	 * Skipping it (preserving the bootloader's configuration instead) made the
	 * board fail to come up at all, so the PHY does need a fresh start under
	 * Linux. What matters is that everything the vendor programs AFTER the
	 * reset must be replicated, in the vendor's order: the pre-init snapshot
	 * shows PLL_STATUS (0x14) reading 0 once our sequence has run, i.e. the PLL
	 * never locks, and an unlocked PLL leaves the UTMI data path without its
	 * clock -- which is why the link state machine works but no packet ever
	 * reaches the core.
	 */
	ret = reset_control_assert(phy->phy_reset);
	if (ret) {
		dev_err(&p->dev, "failed to assert phy_reset: %d\n", ret);
		goto disable_clks;
	}

	usleep_range(100, 150);

	ret = reset_control_deassert(phy->phy_reset);
	if (ret) {
		dev_err(&p->dev, "failed to de-assert phy_reset: %d\n", ret);
		goto disable_clks;
	}

	ret = phy->data->phy_init(p);
	if (ret)
		goto disable_clks;

	return 0;

disable_clks:
	clk_bulk_disable_unprepare(phy->data->num_clks, phy->clks);
exit_repeater:
	phy_exit(phy->repeater);
disable_vreg:
	regulator_bulk_disable(ARRAY_SIZE(phy->vregs), phy->vregs);

	return ret;
}

static int snps_eusb2_hsphy_exit(struct phy *p)
{
	struct snps_eusb2_hsphy *phy = phy_get_drvdata(p);

	clk_bulk_disable_unprepare(phy->data->num_clks, phy->clks);

	regulator_bulk_disable(ARRAY_SIZE(phy->vregs), phy->vregs);

	phy_exit(phy->repeater);

	return 0;
}

static const struct phy_ops snps_eusb2_hsphy_ops = {
	.init		= snps_eusb2_hsphy_init,
	.exit		= snps_eusb2_hsphy_exit,
	.set_mode	= snps_eusb2_hsphy_set_mode,
	.owner		= THIS_MODULE,
};

static int snps_eusb2_hsphy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct snps_eusb2_hsphy *phy;
	struct phy_provider *phy_provider;
	struct phy *generic_phy;
	int ret, i;
	int num;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	phy->data = device_get_match_data(dev);
	if (!phy->data)
		return -EINVAL;

	phy->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(phy->base))
		return PTR_ERR(phy->base);

	/* M2582: optional EUD enable region (see the struct member). */
	{
		struct resource *res = platform_get_resource(pdev,
							     IORESOURCE_MEM, 1);

		if (res) {
			phy->eud_enable = devm_ioremap_resource(dev, res);
			if (IS_ERR(phy->eud_enable))
				phy->eud_enable = NULL;
		}
	}

	phy->phy_reset = devm_reset_control_get_optional_exclusive(dev, NULL);
	if (IS_ERR(phy->phy_reset))
		return PTR_ERR(phy->phy_reset);

	phy->clks = devm_kcalloc(dev, phy->data->num_clks, sizeof(*phy->clks),
				 GFP_KERNEL);
	if (!phy->clks)
		return -ENOMEM;

	for (i = 0; i < phy->data->num_clks; ++i)
		phy->clks[i].id = phy->data->clk_names[i];

	ret = devm_clk_bulk_get(dev, phy->data->num_clks, phy->clks);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to get phy clock(s)\n");

	phy->param_override_seq_cnt = of_property_count_elems_of_size(
			np, "qcom,param-override-seq", sizeof(u32));
	if (phy->param_override_seq_cnt > 0) {
		if (phy->param_override_seq_cnt & 1) {
			dev_err(dev, "invalid param_override_seq_len\n");
			return -EINVAL;
		}

		phy->param_override_seq = devm_kcalloc(dev,
				phy->param_override_seq_cnt,
				sizeof(*phy->param_override_seq), GFP_KERNEL);
		if (!phy->param_override_seq)
			return -ENOMEM;

		ret = of_property_read_variable_u32_array(np,
				"qcom,param-override-seq",
				phy->param_override_seq, 0,
				phy->param_override_seq_cnt);
		if (ret < 0) {
			dev_err(dev, "qcom,param-override-seq read failed %d\n",
				ret);
			return ret;
		}
	}

	phy->ref_clk = NULL;
	for (i = 0; i < phy->data->num_clks; ++i) {
		if (!strcmp(phy->clks[i].id, "ref")) {
			phy->ref_clk = phy->clks[i].clk;
			break;
		}
	}

	if (IS_ERR_OR_NULL(phy->ref_clk)) {
		ret = phy->ref_clk ? PTR_ERR(phy->ref_clk) : -ENOENT;
		return dev_err_probe(dev, ret,
				     "failed to get ref clk\n");
	}

	num = ARRAY_SIZE(phy->vregs);
	for (i = 0; i < num; i++)
		phy->vregs[i].supply = eusb2_hsphy_vreg_names[i];

	ret = devm_regulator_bulk_get(dev, num, phy->vregs);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to get regulator supplies\n");

	phy->vdd_refgen = devm_regulator_get_optional(dev, "vdd_refgen");
	if (IS_ERR(phy->vdd_refgen)) {
		if (PTR_ERR(phy->vdd_refgen) == -ENODEV)
			phy->vdd_refgen = NULL;
		else
			return dev_err_probe(dev, PTR_ERR(phy->vdd_refgen),
					     "failed to get vdd_refgen\n");
	}

	phy->repeater = devm_of_phy_optional_get(dev, np, NULL);
	if (IS_ERR(phy->repeater))
		return dev_err_probe(dev, PTR_ERR(phy->repeater),
				     "failed to get repeater\n");

	generic_phy = devm_phy_create(dev, NULL, &snps_eusb2_hsphy_ops);
	if (IS_ERR(generic_phy)) {
		dev_err(dev, "failed to create phy: %d\n", ret);
		return PTR_ERR(generic_phy);
	}

	dev_set_drvdata(dev, phy);
	phy_set_drvdata(generic_phy, phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	return 0;
}

static const struct of_device_id snps_eusb2_hsphy_of_match_table[] = {
	{
		.compatible = "qcom,tuna-snps-eusb2-phy",
		.data = &tuna_snps_eusb2_phy,
	}, {
		.compatible = "qcom,sm8550-snps-eusb2-phy",
		.data = &sm8550_snps_eusb2_phy,
	}, {
		.compatible = "samsung,exynos2200-eusb2-phy",
		.data = &exynos2200_snps_eusb2_phy,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, snps_eusb2_hsphy_of_match_table);

static struct platform_driver snps_eusb2_hsphy_driver = {
	.probe		= snps_eusb2_hsphy_probe,
	.driver = {
		.name	= "snps-eusb2-hsphy",
		.of_match_table = snps_eusb2_hsphy_of_match_table,
	},
};

module_platform_driver(snps_eusb2_hsphy_driver);
MODULE_DESCRIPTION("Synopsys eUSB2 HS PHY driver");
MODULE_LICENSE("GPL");
