// SPDX-License-Identifier: GPL-2.0-only
/*
 * RPMh ARC (Aggregated Resource Controller) level regulators: cx.lvl, mx.lvl,
 * gfx.lvl.  A vote is a level index into the firmware's command DB aux data
 * table, handed to CPR rather than a voltage; level 0 is the "no vote" entry
 * on tables that start there and doubles as the enable state (no enable bit).
 * The ops aggregate consumers by highest enabled level and touch RPMh only
 * when it moved; struct gdsc::supply naming one lets a rail follow its domain.
 */

#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>

#include <soc/qcom/cmd-db.h>
#include <soc/qcom/rpmh.h>

/* One command DB aux data entry per level. */
#define ARC_LEVEL_SIZE		2

/* The level is the first and only parameter of an ARC resource. */
#define ARC_LEVEL_OFFSET	0

/* More than the hardware has; the firmware tables are shorter than this. */
#define ARC_MAX_LEVELS		16

struct rpmh_arc;

/**
 * struct rpmh_arc_vreg - one consumer of an ARC resource
 * @arc:	the resource this consumer votes on
 * @rdesc:	regulator descriptor
 * @enabled:	whether this consumer currently votes
 * @sel:	level index this consumer votes for
 */
struct rpmh_arc_vreg {
	struct rpmh_arc		*arc;
	struct regulator_desc	rdesc;
	bool			enabled;
	unsigned int		sel;
};

/*
 * struct rpmh_arc - one ARC resource, shared by all of its consumers
 * @dev: RPMh device; @lock: serializes the aggregate; @resource: command DB
 * name ("cx.lvl"); @addr: ARC address; @always_wait_for_ack: write
 * synchronously; @level[]/@level_count: command DB level words and how many
 * are usable; @level_sent/@voted: the level last voted and whether one was;
 * @vregs/@num_vregs: the consumers, one per child node.
 */
struct rpmh_arc {
	struct device		*dev;
	struct mutex		lock;
	const char		*resource;
	u32			addr;
	bool			always_wait_for_ack;
	int			level_count;
	u32			level[ARC_MAX_LEVELS];
	bool			voted;
	u32			level_sent;
	int			num_vregs;
	struct rpmh_arc_vreg	*vregs;
};

/**
 * rpmh_arc_load_levels() - build the level table of an ARC resource
 * @arc:	the resource
 *
 * The table is one u16 per level, zero padded past the last real entry; a
 * zero after index 0 is the padding, not a level, and ends the walk.
 * Return: 0 on success, or a negative error number on failure
 */
static int rpmh_arc_load_levels(struct rpmh_arc *arc)
{
	const u8 *buf;
	size_t len;
	int i, j;

	buf = cmd_db_read_aux_data(arc->resource, &len);
	if (IS_ERR(buf))
		return dev_err_probe(arc->dev, PTR_ERR(buf),
				     "no command DB entry for %s\n", arc->resource);

	if (len == 0 || len % ARC_LEVEL_SIZE)
		return dev_err_probe(arc->dev, -EINVAL,
				     "%s has a %zu byte level table\n",
				     arc->resource, len);

	arc->level_count = min_t(size_t, len / ARC_LEVEL_SIZE,
				 ARRAY_SIZE(arc->level));

	for (i = 0; i < arc->level_count; i++) {
		for (j = 0; j < ARC_LEVEL_SIZE; j++)
			arc->level[i] |= buf[i * ARC_LEVEL_SIZE + j] << (8 * j);

		if (i > 0 && arc->level[i] == 0) {
			arc->level_count = i;
			break;
		}
	}

	if (!arc->level_count)
		return dev_err_probe(arc->dev, -EINVAL,
				     "%s has no levels\n", arc->resource);

	dev_dbg(arc->dev, "%s: %d levels, %u to %u\n", arc->resource,
		arc->level_count, arc->level[0],
		arc->level[arc->level_count - 1]);

	return 0;
}

/**
 * rpmh_arc_aggregate() - highest level any consumer of the resource votes for
 * @arc:	the resource
 *
 * Level 0 is "off" where the table starts at zero; elsewhere having no vote
 * is left out of the request rather than sent as zero.
 * Return: a level index, or -1 when there is no vote to cast
 */
static int rpmh_arc_aggregate(struct rpmh_arc *arc)
{
	unsigned int sel = 0;
	bool any = false;
	int i;

	for (i = 0; i < arc->num_vregs; i++) {
		if (!arc->vregs[i].enabled)
			continue;

		sel = max(sel, arc->vregs[i].sel);
		any = true;
	}

	if (!any && arc->level[0] != 0)
		return -1;

	/*
	 * The vendor's live FDT floors cx.lvl with a proxy consumer
	 * (qcom,proxy-consumer-voltage = <0x180 0xffff>, live-fdt.dts:2500);
	 * this driver and rpmhpd both write res_name "cx.lvl".  No floor here:
	 * the aggregate has to reach level 0 for the last consumer's stop to
	 * switch the rail off; with such a floor the CX GDSC was measured at
	 * gdscr state=2, cfg=050e0000, PWR_ON_STATUS clear.
	 */

	return sel;
}

/**
 * rpmh_arc_send() - cast the aggregated vote, if it moved
 * @arc:	the resource; its lock must be held
 *
 * Return: 0 on success, or a negative error number on failure
 */
static int rpmh_arc_send(struct rpmh_arc *arc)
{
	struct tcs_cmd cmd = {
		.addr = arc->addr + ARC_LEVEL_OFFSET,
		.wait = true,
	};
	int sel, ret;

	lockdep_assert_held(&arc->lock);

	sel = rpmh_arc_aggregate(arc);
	if (sel < 0)
		return 0;

	cmd.data = arc->level[sel];
	if (arc->voted && arc->level_sent == cmd.data)
		return 0;

	/*
	 * Vote asynchronously unless the resource wants the ack: rpmh_write()
	 * waits on the TCS completion with no timeout, and a blocking raise
	 * from the GPU's resume path wedges the board (the CX rail reads
	 * disabled at the GDSC's "up pre" point, so the enable really is a
	 * raise).  The vendor bounds that wait -- kgsl_pwrctrl_enable_cx_gdsc
	 * waits 1250 ms and carries on; a bounded wait would need a completion.
	 */
	if (arc->always_wait_for_ack)
		ret = rpmh_write(arc->dev, RPMH_ACTIVE_ONLY_STATE, &cmd, 1);
	else
		ret = rpmh_write_async(arc->dev, RPMH_ACTIVE_ONLY_STATE, &cmd, 1);
	if (ret)
		return ret;

	arc->voted = true;
	arc->level_sent = cmd.data;

	return 0;
}

/**
 * rpmh_arc_set_enabled() - start or stop one consumer's vote
 * @vreg:	the consumer
 * @enable:	whether it should vote
 *
 * Return: 0 on success, or a negative error number on failure
 */
static int rpmh_arc_set_enabled(struct rpmh_arc_vreg *vreg, bool enable)
{
	struct rpmh_arc *arc = vreg->arc;
	bool was = vreg->enabled;
	bool voted = arc->voted;
	u32 sent = arc->level_sent;
	int ret;

	guard(mutex)(&arc->lock);

	vreg->enabled = enable;
	ret = rpmh_arc_send(arc);
	if (ret) {
		vreg->enabled = was;
		arc->voted = voted;
		arc->level_sent = sent;
	}

	return ret;
}

static int rpmh_arc_enable(struct regulator_dev *rdev)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);

	return rpmh_arc_set_enabled(vreg, true);
}

static int rpmh_arc_disable(struct regulator_dev *rdev)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);

	return rpmh_arc_set_enabled(vreg, false);
}

static int rpmh_arc_is_enabled(struct regulator_dev *rdev)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);

	return vreg->enabled;
}

static int rpmh_arc_set_voltage_sel(struct regulator_dev *rdev, unsigned int sel)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);
	struct rpmh_arc *arc = vreg->arc;
	unsigned int was = vreg->sel;
	bool voted = arc->voted;
	u32 sent = arc->level_sent;
	int ret;

	guard(mutex)(&arc->lock);

	vreg->sel = sel;
	ret = rpmh_arc_send(arc);
	if (ret) {
		vreg->sel = was;
		arc->voted = voted;
		arc->level_sent = sent;
	}

	return ret;
}

static int rpmh_arc_get_voltage_sel(struct regulator_dev *rdev)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);

	return vreg->sel;
}

static int rpmh_arc_list_voltage(struct regulator_dev *rdev, unsigned int sel)
{
	struct rpmh_arc_vreg *vreg = rdev_get_drvdata(rdev);

	if (sel >= vreg->arc->level_count)
		return -EINVAL;

	return vreg->arc->level[sel];
}

static const struct regulator_ops rpmh_arc_ops = {
	.enable			= rpmh_arc_enable,
	.disable		= rpmh_arc_disable,
	.is_enabled		= rpmh_arc_is_enabled,
	.set_voltage_sel	= rpmh_arc_set_voltage_sel,
	.get_voltage_sel	= rpmh_arc_get_voltage_sel,
	.list_voltage		= rpmh_arc_list_voltage,
};

/**
 * rpmh_arc_init_vreg() - register one consumer of an ARC resource
 * @arc:	the resource
 * @node:	the consumer's device tree node
 *
 * Return: 0 on success, or a negative error number on failure
 */
static int rpmh_arc_init_vreg(struct rpmh_arc *arc, struct device_node *node)
{
	struct rpmh_arc_vreg *vreg = &arc->vregs[arc->num_vregs];
	struct regulator_init_data *init_data;
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	u32 want;
	int i, ret;

	vreg->arc = arc;

	ret = of_property_read_string(node, "regulator-name", &vreg->rdesc.name);
	if (ret)
		return dev_err_probe(arc->dev, ret, "%pOFn: no regulator-name\n",
				     node);

	vreg->rdesc.owner = THIS_MODULE;
	vreg->rdesc.type = REGULATOR_VOLTAGE;
	vreg->rdesc.ops = &rpmh_arc_ops;
	vreg->rdesc.n_voltages = arc->level_count;

	/*
	 * The vote a consumer starts with.  It carries more weight here than it
	 * does for a voltage regulator: the level is the whole of the request,
	 * so a consumer that only ever enables its supply -- as a GDSC does --
	 * has nothing else to say what it is asking for.  Without the property
	 * the consumer asks for the highest level the resource has.
	 */
	if (of_property_read_u32(node, "qcom,init-voltage-level", &want))
		want = arc->level[arc->level_count - 1];

	for (i = 0; i < arc->level_count; i++)
		if (want <= arc->level[i])
			break;

	if (i == arc->level_count)
		return dev_err_probe(arc->dev, -EINVAL,
				     "%pOFn: qcom,init-voltage-level = %u is above the highest level %u\n",
				     node, want, arc->level[arc->level_count - 1]);

	vreg->sel = i;
	vreg->enabled = false;

	init_data = of_get_regulator_init_data(arc->dev, node, &vreg->rdesc);
	if (!init_data)
		return -ENOMEM;

	config.dev = arc->dev;
	config.init_data = init_data;
	config.of_node = node;
	config.driver_data = vreg;

	rdev = devm_regulator_register(arc->dev, &vreg->rdesc, &config);
	if (IS_ERR(rdev))
		return dev_err_probe(arc->dev, PTR_ERR(rdev),
				     "could not register %pOFn\n", node);

	arc->num_vregs++;

	return 0;
}

static int rpmh_arc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *child;
	struct rpmh_arc *arc;
	int ret, count = 0;

	arc = devm_kzalloc(dev, sizeof(*arc), GFP_KERNEL);
	if (!arc)
		return -ENOMEM;

	arc->dev = dev;
	mutex_init(&arc->lock);

	ret = of_property_read_string(dev->of_node, "qcom,resource-name",
				      &arc->resource);
	if (ret)
		return dev_err_probe(dev, ret, "no qcom,resource-name\n");

	arc->addr = cmd_db_read_addr(arc->resource);
	if (!arc->addr)
		return dev_err_probe(dev, -ENODEV,
				     "command DB has no address for %s\n",
				     arc->resource);

	if (cmd_db_read_slave_id(arc->resource) != CMD_DB_HW_ARC)
		return dev_err_probe(dev, -EINVAL, "%s is not an ARC resource\n",
				     arc->resource);

	arc->always_wait_for_ack =
		of_property_read_bool(dev->of_node, "qcom,always-wait-for-ack");

	ret = rpmh_arc_load_levels(arc);
	if (ret)
		return ret;

	for_each_available_child_of_node(dev->of_node, child)
		count++;

	if (!count)
		return dev_err_probe(dev, -EINVAL, "no consumers\n");

	arc->vregs = devm_kcalloc(dev, count, sizeof(*arc->vregs), GFP_KERNEL);
	if (!arc->vregs)
		return -ENOMEM;

	for_each_available_child_of_node_scoped(dev->of_node, child) {
		ret = rpmh_arc_init_vreg(arc, child);
		if (ret)
			return ret;
	}

	platform_set_drvdata(pdev, arc);

	dev_dbg(dev, "%s @ 0x%05x, %d consumers\n", arc->resource, arc->addr,
		arc->num_vregs);

	return 0;
}

static const struct of_device_id rpmh_arc_match_table[] = {
	{ .compatible = "qcom,rpmh-arc-regulator" },
	{ }
};
MODULE_DEVICE_TABLE(of, rpmh_arc_match_table);

static struct platform_driver rpmh_arc_driver = {
	.probe = rpmh_arc_probe,
	.driver = {
		.name = "qcom-rpmh-arc-regulator",
		.of_match_table = rpmh_arc_match_table,
	},
};
module_platform_driver(rpmh_arc_driver);

MODULE_DESCRIPTION("Qualcomm RPMh ARC level regulator driver");
MODULE_LICENSE("GPL");
