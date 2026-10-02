// SPDX-License-Identifier: GPL-2.0-only
/*
 * Experimental TUNA bring-up: PMIC5 LDO votes through the existing RPMh API.
 * Protocol: Qualcomm rpmh-regulator.c (2024), VRM voltage/enable/mode words.
 * Board limits and resource names must come from the actual firmware DT.
 * This intentionally supports active-state LDO votes only; no suspend support.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/rpmh.h>

struct tuna_boot_vreg {
	struct device *dev;
	struct regulator_desc desc;
	u32 address;
	int voltage;
	int enabled;
};

static int tuna_vreg_write(struct tuna_boot_vreg *vreg, u32 offset, u32 value)
{
	struct tcs_cmd cmd = {
		.addr = vreg->address + offset,
		.data = value,
		.wait = true,
	};

	return rpmh_write(vreg->dev, RPMH_ACTIVE_ONLY_STATE, &cmd, 1);
}

static int tuna_vreg_set_voltage(struct regulator_dev *rdev, int min_uV,
				 int max_uV, unsigned int *selector)
{
	struct tuna_boot_vreg *vreg = rdev_get_drvdata(rdev);
	int mv = DIV_ROUND_UP(min_uV, 1000);
	int ret;

	if (mv * 1000 > max_uV)
		return -EINVAL;
	ret = tuna_vreg_write(vreg, 0, mv);
	if (!ret)
		vreg->voltage = mv * 1000;
	return ret;
}

static int tuna_vreg_get_voltage(struct regulator_dev *rdev)
{
	struct tuna_boot_vreg *vreg = rdev_get_drvdata(rdev);

	return vreg->voltage;
}

static int tuna_vreg_enable(struct regulator_dev *rdev)
{
	struct tuna_boot_vreg *vreg = rdev_get_drvdata(rdev);
	int ret;

	/* PMIC5 LDO high-power mode, as used by the downstream PMIC5 map. */
	ret = tuna_vreg_write(vreg, 8, 7);
	if (ret)
		return ret;
	ret = tuna_vreg_write(vreg, 4, 1);
	if (!ret)
		vreg->enabled = 1;
	return ret;
}

static int tuna_vreg_disable(struct regulator_dev *rdev)
{
	struct tuna_boot_vreg *vreg = rdev_get_drvdata(rdev);
	int ret = tuna_vreg_write(vreg, 4, 0);

	if (!ret)
		vreg->enabled = 0;
	return ret;
}

static int tuna_vreg_is_enabled(struct regulator_dev *rdev)
{
	struct tuna_boot_vreg *vreg = rdev_get_drvdata(rdev);

	return vreg->enabled;
}

static const struct regulator_ops tuna_vreg_ops = {
	.set_voltage = tuna_vreg_set_voltage,
	.get_voltage = tuna_vreg_get_voltage,
	.enable = tuna_vreg_enable,
	.disable = tuna_vreg_disable,
	.is_enabled = tuna_vreg_is_enabled,
};

static int tuna_boot_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	unsigned int id = 0;
	int ret;

	ret = cmd_db_ready();
	if (ret)
		return dev_err_probe(dev, ret, "Command DB not ready\n");

	for_each_available_child_of_node_scoped(dev->of_node, child) {
		struct regulator_config config = { .dev = dev, .of_node = child };
		struct regulator_dev *rdev;
		struct regulator_init_data *init_data;
		struct tuna_boot_vreg *vreg;
		const char *resource;

		ret = of_property_read_string(child, "qcom,resource-name", &resource);
		if (ret)
			return ret;
		if (strncmp(resource, "ldo", 3))
			return dev_err_probe(dev, -EINVAL, "Only PMIC5 LDOs supported\n");
		vreg = devm_kzalloc(dev, sizeof(*vreg), GFP_KERNEL);
		if (!vreg)
			return -ENOMEM;
		vreg->dev = dev;
		vreg->address = cmd_db_read_addr(resource);
		if (!vreg->address)
			return dev_err_probe(dev, -ENODEV, "Missing resource %s\n", resource);
		if (cmd_db_read_slave_id(resource) != CMD_DB_HW_VRM)
			return dev_err_probe(dev, -EINVAL, "Resource %s is not VRM\n", resource);
		/* No readable voltage register: ask the core to initialize it. */
		vreg->voltage = -ENOTRECOVERABLE;
		vreg->enabled = -EINVAL;
		vreg->desc.name = resource;
		vreg->desc.id = id++;
		vreg->desc.owner = THIS_MODULE;
		vreg->desc.type = REGULATOR_VOLTAGE;
		vreg->desc.ops = &tuna_vreg_ops;
		vreg->desc.continuous_voltage_range = true;
		config.driver_data = vreg;
		init_data = of_get_regulator_init_data(dev, child, &vreg->desc);
		if (!init_data || init_data->constraints.min_uV <= 0 ||
		    init_data->constraints.max_uV < init_data->constraints.min_uV)
			return dev_err_probe(dev, -EINVAL, "Missing limits for %s\n", resource);
		init_data->constraints.apply_uV = true;
		config.init_data = init_data;
		rdev = devm_regulator_register(dev, &vreg->desc, &config);
		if (IS_ERR(rdev))
			return dev_err_probe(dev, PTR_ERR(rdev), "Register %s\n", resource);
	}
	return 0;
}

static const struct of_device_id tuna_boot_regulator_match[] = {
	{ .compatible = "qcom,tuna-boot-regulators" },
	{ }
};
MODULE_DEVICE_TABLE(of, tuna_boot_regulator_match);

static struct platform_driver tuna_boot_regulator_driver = {
	.probe = tuna_boot_regulator_probe,
	.driver = {
		.name = "qcom-tuna-boot-regulator",
		.of_match_table = tuna_boot_regulator_match,
	},
};
module_platform_driver(tuna_boot_regulator_driver);
MODULE_DESCRIPTION("Experimental TUNA boot-time RPMh LDO votes");
MODULE_LICENSE("GPL");
