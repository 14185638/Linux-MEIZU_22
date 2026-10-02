// SPDX-License-Identifier: GPL-2.0-only
/*
 * Temporary M2582 bring-up bridge for ABLs without a fastboot boot command.
 * The kernel carries the mainline DT. Import firmware RAM and reservations,
 * never downstream device nodes or phandles. This is not an upstream binding.
 */
#ifdef M2582_FDT_HOST_TEST
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libfdt.h>
#define __init
#else
#include <linux/init.h>
#include <linux/libfdt.h>
#include <linux/printk.h>
#include <linux/console.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/dma-map-ops.h>
#include <linux/font.h>
#include <linux/io.h>
#include <linux/kstrtox.h>
#include <linux/memblock.h>
#include <linux/memremap.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched/clock.h>
#include <linux/seq_file.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/sections.h>
#include <asm/sysreg.h>
#endif
#include "meizu-m2582-fdt.h"

#ifndef M2582_FDT_HOST_TEST
#define M2582_RAMOOPS_PHYS		0xbffbff000ULL
#define M2582_MARKER_PHYS		(M2582_RAMOOPS_PHYS + 0x3ff000)
#define M2582_MARKER_SIZE		0x1000
#define M2582_MARKER_MAGIC		0x4d32524d
#define M2582_MARKER_VERSION		1

struct m2582_marker {
	__le32 magic;
	__le16 version;
	__le16 header_size;
	__le32 phase;
	__le32 sequence;
	__le64 phys;
	__le32 length;
	__le32 crc32;
	__le64 kernel_text;
	__le64 kernel_image;
	__le64 dtb_phys;
	__le32 dtb_size;
	__le32 reserved;
	u8 padding[16];
} __packed;

static u32 m2582_marker_sequence;
static struct m2582_marker m2582_previous_marker;
static bool m2582_previous_valid;

static void __init __maybe_unused m2582_marker_read_previous(void)
{
	struct m2582_marker marker;
	void *mapped;
	u32 crc;

	mapped = early_memremap(M2582_MARKER_PHYS, M2582_MARKER_SIZE);
	if (!mapped)
		return;
	memcpy(&marker, mapped, sizeof(marker));
	early_memunmap(mapped, M2582_MARKER_SIZE);
	if (le32_to_cpu(marker.magic) != M2582_MARKER_MAGIC ||
	    le16_to_cpu(marker.version) != M2582_MARKER_VERSION ||
	    le16_to_cpu(marker.header_size) != sizeof(marker) ||
	    le64_to_cpu(marker.phys) != M2582_MARKER_PHYS ||
	    le32_to_cpu(marker.length) != sizeof(marker))
		return;
	crc = crc32_le(~0, (u8 *)&marker, offsetof(struct m2582_marker, crc32));
	if (crc != le32_to_cpu(marker.crc32))
		return;
	m2582_previous_marker = marker;
	m2582_previous_valid = true;
	m2582_marker_sequence = le32_to_cpu(marker.sequence);
}

static void m2582_marker_fill(struct m2582_marker *marker, u32 phase)
{
	*marker = (struct m2582_marker) {
		.magic = cpu_to_le32(M2582_MARKER_MAGIC),
		.version = cpu_to_le16(M2582_MARKER_VERSION),
		.header_size = cpu_to_le16(sizeof(*marker)),
		.phase = cpu_to_le32(phase),
		.sequence = cpu_to_le32(++m2582_marker_sequence),
		.phys = cpu_to_le64(M2582_MARKER_PHYS),
		.length = cpu_to_le32(sizeof(*marker)),
		.kernel_text = cpu_to_le64((u64)(uintptr_t)_text),
		.kernel_image = cpu_to_le64((u64)(__bss_stop - _text)),
		.dtb_phys = cpu_to_le64(0),
		.dtb_size = cpu_to_le32(0),
	};
	marker->crc32 = cpu_to_le32(crc32_le(~0, (u8 *)marker,
					     offsetof(struct m2582_marker, crc32)));
	}

static void m2582_marker_store(void *mapped, const struct m2582_marker *marker)
{
	memcpy(mapped, marker, sizeof(*marker));
	/*
	 * Flush through the mapping we already hold. arch_sync_dma_for_device()
	 * would call phys_to_virt() on the raw physical address, and before
	 * paging_init() the linear map does not exist yet, so that cache
	 * maintenance runs on an unmapped address and can abort the kernel.
	 */
	dcache_clean_poc((unsigned long)mapped,
			 (unsigned long)mapped + sizeof(*marker));
}

static void m2582_marker_write_internal(u32 phase)
{
	struct m2582_marker marker;
	void *mapped = memremap(M2582_MARKER_PHYS, M2582_MARKER_SIZE, MEMREMAP_WB);

	if (!mapped)
		return;
	m2582_marker_fill(&marker, phase);
	m2582_marker_store(mapped, &marker);
	memunmap(mapped);
}

static void __init __maybe_unused m2582_marker_write_early(void)
{
	struct m2582_marker marker;
	void *mapped = early_memremap(M2582_MARKER_PHYS, M2582_MARKER_SIZE);

	if (!mapped)
		return;
	m2582_marker_fill(&marker, 1);
	m2582_marker_store(mapped, &marker);
	early_memunmap(mapped, M2582_MARKER_SIZE);
}

void meizu_m2582_marker_write(u32 phase)
{
	m2582_marker_write_internal(phase);
}

void __init meizu_m2582_marker_early(void)
{
	/* Retired: the on-screen console is the diagnostic channel now. */
}

static int m2582_marker_proc_show(struct seq_file *m, void *v)
{
	if (!m2582_previous_valid) {
		seq_puts(m, "valid=0\n");
		return 0;
	}
	seq_printf(m, "valid=1 phase=%u sequence=%u phys=0x%llx crc=0x%08x\n",
		   le32_to_cpu(m2582_previous_marker.phase),
		   le32_to_cpu(m2582_previous_marker.sequence),
		   le64_to_cpu(m2582_previous_marker.phys),
		   le32_to_cpu(m2582_previous_marker.crc32));
	return 0;
}

static int m2582_marker_reboot(struct notifier_block *nb, unsigned long action,
			       void *data)
{
	meizu_m2582_marker_write(3);
	return NOTIFY_DONE;
}

static struct notifier_block __maybe_unused m2582_marker_reboot_nb = {
	.notifier_call = m2582_marker_reboot,
	.priority = INT_MAX,
};

static int __init __maybe_unused m2582_marker_late_init(void)
{
	meizu_m2582_marker_write(2);
	proc_create_single("m2582_marker", 0444, NULL, m2582_marker_proc_show);
	return register_reboot_notifier(&m2582_marker_reboot_nb);
}

#endif /* !M2582_FDT_HOST_TEST */

static int m2582_firmware_ramoops(const void *firmware,
				   unsigned long long target_base,
				   unsigned long long *base,
				   unsigned long long *size)
{
	const fdt32_t *memory_region;
	const fdt64_t *reg, *alloc_ranges, *size_prop;
	unsigned long long start, length, end;
	int node, region, len, i;

	node = fdt_node_offset_by_compatible(firmware, -1, "qcom,ramoops");
	if (node < 0)
		return node;
	memory_region = fdt_getprop(firmware, node, "memory-region", &len);
	if (!memory_region || len != 4)
		return -FDT_ERR_BADVALUE;
	region = fdt_node_offset_by_phandle(firmware,
					   fdt32_to_cpu(memory_region[0]));
	if (region < 0)
		return region;

	reg = fdt_getprop(firmware, region, "reg", &len);
	if (reg && len == 16) {
		*base = fdt64_to_cpu(reg[0]);
		*size = fdt64_to_cpu(reg[1]);
		return *size ? 0 : -FDT_ERR_BADVALUE;
	}

	size_prop = fdt_getprop(firmware, region, "size", &len);
	alloc_ranges = fdt_getprop(firmware, region, "alloc-ranges", &len);
	if (!size_prop || !alloc_ranges || len < 16 || len % 16)
		return -FDT_ERR_BADVALUE;
	*base = 0;
	*size = fdt64_to_cpu(size_prop[0]);
	if (!*size)
		return -FDT_ERR_BADVALUE;
	/* Validate that the recorded ramoops allocation could contain the target. */
	for (i = 0; i < len / 8; i += 2) {
		start = fdt64_to_cpu(alloc_ranges[i]);
		length = fdt64_to_cpu(alloc_ranges[i + 1]);
		end = start + length;
		if (end < start)
			end = ~0ULL;
		if (length && end > start && target_base >= start &&
		    target_base + *size >= target_base &&
		    target_base + *size <= end)
			return 0;
	}
	return -FDT_ERR_BADVALUE;
}

static int __init m2582_copy_ramoops(const void *firmware, const void *built_in,
				   void *out)
{
	const fdt64_t *reg, *banks;
	const void *data;
	const char *name;
	unsigned long long base, end, start, stop, marker_base, marker_size;
	unsigned long long firmware_base, firmware_size;
	int node, source, target, child, memory, len, i, property, ret;
	int contained = 0;

	source = fdt_node_offset_by_compatible(built_in, -1, "ramoops");
	if (source < 0)
		return source;
	reg = fdt_getprop(built_in, source, "reg", &len);
	if (!reg || len != 16)
		return -FDT_ERR_BADVALUE;
	base = fdt64_to_cpu(reg[0]);
	end = base + fdt64_to_cpu(reg[1]);
	if (end <= base)
		return -FDT_ERR_BADVALUE;
	memory = fdt_node_offset_by_prop_value(out, -1, "device_type", "memory", 7);
	banks = fdt_getprop(out, memory, "reg", &len);
	if (!banks || len % 16)
		return -FDT_ERR_BADVALUE;
	for (i = 0; i < len / 8; i += 2) {
		start = fdt64_to_cpu(banks[i]);
		stop = start + fdt64_to_cpu(banks[i + 1]);
		if (base >= start && end <= stop)
			contained = 1;
	}
	if (!contained)
		return -FDT_ERR_BADVALUE;
	/* Never place diagnostic storage on top of a firmware reservation. */
	for (i = 0; i < fdt_num_mem_rsv(firmware); i++) {
		fdt64_t address, size;

		ret = fdt_get_mem_rsv(firmware, i, &address, &size);
		if (ret || (base < address + size && address < end))
			return -FDT_ERR_BADVALUE;
	}
	target = fdt_path_offset(out, "/reserved-memory");
	child = -1;
	fdt_for_each_subnode(node, out, target) {
		banks = fdt_getprop(out, node, "reg", &len);
		if (!banks || len % 16)
			return -FDT_ERR_BADVALUE;
		for (i = 0; i < len / 8; i += 2) {
			start = fdt64_to_cpu(banks[i]);
			stop = start + fdt64_to_cpu(banks[i + 1]);
			if (base < stop && start < end) {
				/* The firmware ramoops reservation includes our marker tail. */
				if (len == 16 && base == start && end <= stop)
					child = node;
				else
					return -FDT_ERR_BADVALUE;
			}
		}
	}
	if (child < 0)
		child = fdt_add_subnode(out, target, fdt_get_name(built_in, source, NULL));
	if (child < 0)
		return child;
	fdt_for_each_property_offset(property, built_in, source) {
		data = fdt_getprop_by_offset(built_in, property, &name, &len);
		if (!data)
			return len;
		if (!strcmp(name, "phandle") || !strcmp(name, "linux,phandle"))
			continue;
		if ((ret = fdt_setprop(out, child, name, data, len)))
			return ret;
	}

	/* Reserve the tail separately so the early marker is never page-allocated. */
	source = fdt_path_offset(built_in,
				 "/reserved-memory/m2582-marker@bffffe000");
	if (source < 0)
		return source;
	reg = fdt_getprop(built_in, source, "reg", &len);
	if (!reg || len != 16)
		return -FDT_ERR_BADVALUE;
	marker_base = fdt64_to_cpu(reg[0]);
	marker_size = fdt64_to_cpu(reg[1]);
	if (!marker_size || marker_base != end || marker_base + marker_size <= marker_base)
		return -FDT_ERR_BADVALUE;

	/* The full marker must remain within the firmware's original ramoops range. */
	ret = m2582_firmware_ramoops(firmware, base, &firmware_base,
				     &firmware_size);
	if (ret || firmware_size < end - base + marker_size)
		return -FDT_ERR_BADVALUE;
	if (firmware_base && firmware_base != base)
		return -FDT_ERR_BADVALUE;

	memory = fdt_node_offset_by_prop_value(out, -1, "device_type", "memory", 7);
	banks = fdt_getprop(out, memory, "reg", &len);
	if (!banks || len % 16)
		return -FDT_ERR_BADVALUE;
	contained = 0;
	for (i = 0; i < len / 8; i += 2) {
		start = fdt64_to_cpu(banks[i]);
		stop = start + fdt64_to_cpu(banks[i + 1]);
		if (marker_base >= start && marker_base + marker_size <= stop)
			contained = 1;
	}
	if (!contained)
		return -FDT_ERR_BADVALUE;

	target = fdt_path_offset(out, "/reserved-memory");
	fdt_for_each_subnode(node, out, target) {
		banks = fdt_getprop(out, node, "reg", &len);
		if (!banks || len % 16)
			return -FDT_ERR_BADVALUE;
		for (i = 0; i < len / 8; i += 2) {
			start = fdt64_to_cpu(banks[i]);
			stop = start + fdt64_to_cpu(banks[i + 1]);
			if (marker_base < stop && start < marker_base + marker_size)
				return -FDT_ERR_BADVALUE;
		}
	}
	name = fdt_get_name(built_in, source, NULL);
	child = fdt_add_subnode(out, target, name);
	if (child < 0)
		return child;
	reg = fdt_getprop(built_in, source, "reg", &len);
	if ((ret = fdt_setprop(out, child, "reg", reg, len)) ||
	    (ret = fdt_setprop(out, child, "no-map", NULL, 0)))
		return ret;
	return 0;
}

int __init meizu_m2582_prepare_fdt(const void *firmware, const void *built_in,
				 void *out, int capacity)
{
	const fdt32_t *prop;
	const void *reg;
	const char *model, *type, *name;
	fdt64_t address, size;
	unsigned char cmd_db_reg[16];
	int ret, len, node, target, parent, child, memory, chosen;
	int banks = 0, cmd_db = 0, i, count;

	if (fdt_check_header(firmware) || fdt_check_header(built_in))
		return -FDT_ERR_BADMAGIC;
	model = fdt_getprop(firmware, 0, "model", &len);
	if (!model || len < 6 || model[len - 1] || !strstr(model, "M2582"))
		return -FDT_ERR_BADVALUE;
	prop = fdt_getprop(firmware, 0, "meizu,board-id", &len);
	if (!prop || len != 8 || fdt32_to_cpu(prop[0]) != 2)
		return -FDT_ERR_BADVALUE;
	prop = fdt_getprop(firmware, 0, "qcom,msm-id", &len);
	if (!prop || len < 8 || len % 8)
		return -FDT_ERR_BADVALUE;
	for (i = 0; i < len / 4; i += 2)
		if ((fdt32_to_cpu(prop[i]) & 0xffff) == 655)
			break;
	if (i == len / 4 || fdt_address_cells(firmware, 0) != 2 ||
	    fdt_size_cells(firmware, 0) != 2)
		return -FDT_ERR_BADVALUE;
	ret = fdt_open_into(built_in, out, capacity);
	if (ret)
		return ret;

	/* Locate the one imported platform dependency before replacing reserves. */
	node = fdt_node_offset_by_compatible(out, -1, "qcom,cmd-db");
	reg = fdt_getprop(out, node, "reg", &len);
	if (!reg || len != sizeof(cmd_db_reg))
		return -FDT_ERR_BADVALUE;
	memcpy(cmd_db_reg, reg, sizeof(cmd_db_reg));
	memory = fdt_node_offset_by_prop_value(out, -1, "device_type", "memory", 7);
	if (memory < 0)
		return memory;
	fdt_for_each_subnode(node, firmware, 0) {
		type = fdt_getprop(firmware, node, "device_type", &len);
		if (!type || len != 7 || memcmp(type, "memory", 7))
			continue;
		reg = fdt_getprop(firmware, node, "reg", &len);
		if (!reg || len <= 0 || len % 16)
			return -FDT_ERR_BADVALUE;
		if (banks)
			ret = fdt_appendprop(out, memory, "reg", reg, len);
		else
			ret = fdt_setprop(out, memory, "reg", reg, len);
		if (ret)
			return ret;
		banks += len / 16;
	}
	if (!banks)
		return -FDT_ERR_BADVALUE;

	/* No mainline devices in this bring-up DT reference reserved phandles. */
	parent = fdt_path_offset(out, "/reserved-memory");
	if (parent < 0 || (ret = fdt_del_node(out, parent)))
		return parent < 0 ? parent : ret;
	parent = fdt_path_offset(firmware, "/reserved-memory");
	if (parent < 0 || fdt_address_cells(firmware, parent) != 2 ||
	    fdt_size_cells(firmware, parent) != 2)
		return -FDT_ERR_BADVALUE;
	reg = fdt_getprop(firmware, parent, "ranges", &len);
	if (!reg || len)
		return -FDT_ERR_BADVALUE;
	target = fdt_add_subnode(out, 0, "reserved-memory");
	if (target < 0)
		return target;
	if ((ret = fdt_setprop_u32(out, target, "#address-cells", 2)) ||
	    (ret = fdt_setprop_u32(out, target, "#size-cells", 2)) ||
	    (ret = fdt_setprop(out, target, "ranges", NULL, 0)))
		return ret;
	fdt_for_each_subnode(node, firmware, parent) {
		reg = fdt_getprop(firmware, node, "reg", &len);
		if (!reg)
			continue; /* Downstream dynamically allocated pools aren't used. */
		if (len <= 0 || len % 16)
			return -FDT_ERR_BADVALUE;
		name = fdt_get_name(firmware, node, NULL);
		child = fdt_add_subnode(out, target, name);
		if (child < 0)
			return child;
		if ((ret = fdt_setprop(out, child, "reg", reg, len)))
			return ret;
		if (fdt_getprop(firmware, node, "no-map", NULL) &&
		    (ret = fdt_setprop(out, child, "no-map", NULL, 0)))
			return ret;
		if (len == sizeof(cmd_db_reg) && !memcmp(reg, cmd_db_reg, len)) {
			if ((ret = fdt_setprop_string(out, child, "compatible", "qcom,cmd-db")))
				return ret;
			cmd_db++;
		}
	}
	if (cmd_db != 1)
		return -FDT_ERR_BADVALUE;
	if ((ret = m2582_copy_ramoops(firmware, built_in, out)))
		return ret;
	while (fdt_num_mem_rsv(out) > 0)
		if ((ret = fdt_del_mem_rsv(out, 0)))
			return ret;
	count = fdt_num_mem_rsv(firmware);
	if (count < 0)
		return count;
	for (i = 0; i < count; i++) {
		ret = fdt_get_mem_rsv(firmware, i, &address, &size);
		if (ret || (ret = fdt_add_mem_rsv(out, address, size)))
			return ret;
	}
	chosen = fdt_path_offset(firmware, "/chosen");
	target = fdt_path_offset(out, "/chosen");
	if (chosen >= 0 && target >= 0) {
		static const char * const properties[] = {
			"linux,initrd-start", "linux,initrd-end", "rng-seed",
			"linux,uefi-system-table", "linux,uefi-mmap-start",
			"linux,uefi-mmap-size", "linux,uefi-mmap-desc-size",
			"linux,uefi-mmap-desc-ver", "linux,uefi-stub-kern-ver",
		};
		for (i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
			reg = fdt_getprop(firmware, chosen, properties[i], &len);
			if (reg && (ret = fdt_setprop(out, target, properties[i], reg, len)))
				return ret;
		}
	}
	if ((ret = fdt_setprop_u32(out, 0, "meizu,imported-memory-banks", banks)))
		return ret;
	if ((ret = fdt_setprop(out, 0, "meizu,embedded-dtb-active", NULL, 0)))
		return ret;
	return fdt_pack(out);
}

#ifndef M2582_FDT_HOST_TEST
#ifdef CONFIG_ARM64_MEIZU_M2582_BUILTIN_DTB
#ifdef CONFIG_ARM64_MEIZU_M2582_BOOT_PROBE
extern const unsigned char __dtb_meizu_m2582_boot_probe_begin[];
#define m2582_builtin_dtb __dtb_meizu_m2582_boot_probe_begin
#else
extern const unsigned char __dtb_meizu_m2582_begin[];
#define m2582_builtin_dtb __dtb_meizu_m2582_begin
#endif
/* Keep raw FDT storage after free_initmem(), including for /sys/firmware/fdt. */
static unsigned char m2582_fdt[128 * 1024] __aligned(8) __maybe_unused;
#endif

/*
 * Boot progress beacon.
 *
 * ABL hands the kernel a live display that keeps scanning out of the
 * continuous-splash reserved region (cont_splash_region). The panel is
 * command mode, so the logo would otherwise stay on screen forever and hide
 * every early failure. Painting that buffer needs no clock, regulator, DPU,
 * DSI or panel driver, which makes it the only output channel that works
 * before any peripheral is up.
 *
 * Fills use a single byte repeated across the whole region, so the result is
 * the same shade for 3bpp, 4bpp, RGB and BGR layouts alike.
 */
#define M2582_SPLASH_PHYS_DEFAULT	0xfc800000ULL
#define M2582_SPLASH_SIZE_DEFAULT	0x2b00000ULL
#define M2582_SPLASH_SIZE_MAX		0x4000000ULL

bool meizu_m2582_fdt_import_failed;
static phys_addr_t m2582_splash_phys = M2582_SPLASH_PHYS_DEFAULT;
static size_t m2582_splash_size = M2582_SPLASH_SIZE_DEFAULT;
#define M2582_SPLASH_CHUNK	SZ_256K

static void __init __maybe_unused m2582_splash_locate(const void *firmware)
{
	const fdt64_t *reg;
	const char *name;
	int parent, node, len;

	parent = fdt_path_offset(firmware, "/reserved-memory");
	if (parent < 0)
		return;
	fdt_for_each_subnode(node, firmware, parent) {
		name = fdt_get_name(firmware, node, NULL);
		if (!name || (strncmp(name, "splash", 6) &&
			      strncmp(name, "cont_splash", 11)))
			continue;
		reg = fdt_getprop(firmware, node, "reg", &len);
		if (!reg || len != 16)
			continue;
		m2582_splash_phys = fdt64_to_cpu(reg[0]);
		m2582_splash_size = fdt64_to_cpu(reg[1]);
		if (!m2582_splash_size || m2582_splash_size > M2582_SPLASH_SIZE_MAX)
			m2582_splash_size = M2582_SPLASH_SIZE_DEFAULT;
		pr_info("M2582: splash beacon buffer 0x%llx size 0x%zx\n",
			(unsigned long long)m2582_splash_phys, m2582_splash_size);
		return;
	}
}

/*
 * Driver-free text console on the bootloader's continuous-splash buffer.
 *
 * The panel is 1200x2670 at 24bpp and ABL leaves the display engine scanning
 * that carveout, so plain memory writes are visible without any clock,
 * regulator, DPU, DSI or panel driver.
 *
 * There are two backends. Until paging_init() installs the linear map, each
 * text row is reached with early_memremap(); a text row is exactly 16 pixel
 * rows and therefore one contiguous 57,600 byte block, so a single mapping
 * serves a whole row of characters. Afterwards the linear map is used
 * directly, which also makes scrolling cheap. Registering with
 * CON_PRINTBUFFER makes register_console() replay the whole printk ring
 * buffer, so nothing logged before the console existed is lost.
 */
#define M2582_FB_WIDTH		1200
#define M2582_FB_HEIGHT		2670
#define M2582_FB_BPP		4
#define M2582_FB_STRIDE		(M2582_FB_WIDTH * M2582_FB_BPP)
#define M2582_FONT_W		8
#define M2582_FONT_H		16
/*
 * Glyphs are blown up by an integer factor. The panel is 1200x2670, so the
 * native 8x16 cell gives 150x166 characters of ~0.5 mm text, which is not
 * legible in a photograph of a screen that dies the instant the kernel hangs.
 * At 2x the tail of the log is readable and 83 lines still fit.
 */
#define M2582_FONT_SCALE	2
#define M2582_CELL_W		(M2582_FONT_W * M2582_FONT_SCALE)
#define M2582_CELL_H		(M2582_FONT_H * M2582_FONT_SCALE)
#define M2582_CON_COLS		(M2582_FB_WIDTH / M2582_CELL_W)
#define M2582_CON_ROWS		(M2582_FB_HEIGHT / M2582_CELL_H)
#define M2582_FB_FRAME		((size_t)M2582_FB_STRIDE * M2582_FB_HEIGHT)
#define M2582_ROW_BYTES		((size_t)M2582_CELL_H * M2582_FB_STRIDE)

static u8 *m2582_fb;			/* linear-map backend once available */
static bool m2582_fb_early = true;	/* early_memremap backend */
static bool m2582_console_up;
static unsigned int m2582_con_x, m2582_con_y;
static size_t m2582_dirty_lo, m2582_dirty_hi;
static void *m2582_row_map;
static unsigned int m2582_row_idx = ~0u;
/*
 * early_memremap() is __init, but the console callbacks can run long after
 * initmem is freed. Reach it indirectly so the early backend never leaves a
 * relocation from .text into .init.text; the pointers are only installed by
 * the __init setup and cleared as soon as the linear map takes over.
 */
static void *(*m2582_row_map_fn)(resource_size_t, unsigned long);
static void (*m2582_row_unmap_fn)(void *, unsigned long);

static void m2582_fb_dirty(size_t lo, size_t hi)
{
	if (lo < m2582_dirty_lo)
		m2582_dirty_lo = lo;
	if (hi > m2582_dirty_hi)
		m2582_dirty_hi = hi;
}

static void m2582_fb_flush(void)
{
	if (m2582_dirty_hi > m2582_dirty_lo)
		dcache_clean_poc((unsigned long)(m2582_fb + m2582_dirty_lo),
				 (unsigned long)(m2582_fb + m2582_dirty_hi));
	m2582_dirty_lo = ~(size_t)0;
	m2582_dirty_hi = 0;
}

/* Keep the current text row mapped so a whole line costs one remap. */
static u8 *m2582_row_get(unsigned int row)
{
	if (row == m2582_row_idx && m2582_row_map)
		return m2582_row_map;
	if (m2582_row_map) {
		dcache_clean_poc((unsigned long)m2582_row_map,
				 (unsigned long)m2582_row_map + M2582_ROW_BYTES);
		if (m2582_row_unmap_fn)
			m2582_row_unmap_fn(m2582_row_map, M2582_ROW_BYTES);
		m2582_row_map = NULL;
	}
	if (!m2582_fb_early || !m2582_row_map_fn)
		return NULL;
	m2582_row_map = m2582_row_map_fn(m2582_splash_phys +
					 (size_t)row * M2582_ROW_BYTES,
					 M2582_ROW_BYTES);
	m2582_row_idx = m2582_row_map ? row : ~0u;
	return m2582_row_map;
}

static void m2582_row_release(void)
{
	if (!m2582_row_map)
		return;
	dcache_clean_poc((unsigned long)m2582_row_map,
			 (unsigned long)m2582_row_map + M2582_ROW_BYTES);
	if (m2582_row_unmap_fn)
		m2582_row_unmap_fn(m2582_row_map, M2582_ROW_BYTES);
	m2582_row_map = NULL;
	m2582_row_idx = ~0u;
}

/* Blank one text row so wrapped-around lines never mix with old text. */
static void m2582_row_clear(unsigned int row)
{
	if (m2582_fb) {
		memset(m2582_fb + (size_t)row * M2582_ROW_BYTES, 0,
		       M2582_ROW_BYTES);
		dcache_clean_poc((unsigned long)(m2582_fb + (size_t)row * M2582_ROW_BYTES),
				 (unsigned long)(m2582_fb + (size_t)(row + 1) * M2582_ROW_BYTES));
	} else {
		u8 *p = m2582_row_get(row);

		if (p)
			memset(p, 0, M2582_ROW_BYTES);
	}
}

static void m2582_fb_clear_all(void)
{
	unsigned int r;

	if (m2582_fb) {
		memset(m2582_fb, 0, M2582_FB_FRAME);
		dcache_clean_poc((unsigned long)m2582_fb,
				 (unsigned long)m2582_fb + M2582_FB_FRAME);
		return;
	}
	for (r = 0; r < M2582_CON_ROWS; r++)
		m2582_row_clear(r);
	m2582_row_release();
}

static void m2582_fb_newline(void)
{
	m2582_con_x = 0;
	if (++m2582_con_y >= M2582_CON_ROWS) {
		if (m2582_fb) {
			/* Scroll one text row; the frame is far too tall to keep. */
			memmove(m2582_fb, m2582_fb + M2582_ROW_BYTES,
				(size_t)(M2582_CON_ROWS - 1) * M2582_ROW_BYTES);
			memset(m2582_fb + (size_t)(M2582_CON_ROWS - 1) * M2582_ROW_BYTES,
			       0, M2582_ROW_BYTES);
			m2582_fb_dirty(0, (size_t)(M2582_CON_ROWS - 1) * M2582_ROW_BYTES);
			m2582_con_y = M2582_CON_ROWS - 1;
		} else {
			/* A fixmap window cannot scroll: wrap and overwrite. */
			m2582_con_y = 0;
		}
	}
	m2582_row_clear(m2582_con_y);
}

static void m2582_fb_putc(char c)
{
	const u8 *glyph;
	u8 *cell;
	unsigned int x, y;
	size_t off, span;

	if (!m2582_fb && !m2582_fb_early)
		return;
	switch (c) {
	case '\n':
		m2582_fb_newline();
		return;
	case '\r':
		m2582_con_x = 0;
		return;
	case '\t':
		do {
			m2582_fb_putc(' ');
		} while (m2582_con_x % 8);
		return;
	}
	if ((unsigned char)c < 0x20 || (unsigned char)c > 0x7e)
		c = '?';

	glyph = font_vga_8x16.data + (unsigned char)c * M2582_FONT_H;
	off = (size_t)m2582_con_y * M2582_ROW_BYTES +
	      (size_t)m2582_con_x * M2582_CELL_W * M2582_FB_BPP;
	span = (size_t)(M2582_CELL_H - 1) * M2582_FB_STRIDE +
	       M2582_CELL_W * M2582_FB_BPP;

	if (m2582_fb) {
		cell = m2582_fb + off;
	} else {
		cell = m2582_row_get(m2582_con_y);
		if (!cell)
			return;
		cell += (size_t)m2582_con_x * M2582_CELL_W * M2582_FB_BPP;
	}
	for (y = 0; y < M2582_FONT_H; y++) {
		u8 bits = glyph[y];
		unsigned int sy, sx, b;

		for (sy = 0; sy < M2582_FONT_SCALE; sy++) {
			u8 *px = cell + (size_t)(y * M2582_FONT_SCALE + sy) *
					M2582_FB_STRIDE;

			for (x = 0; x < M2582_FONT_W; x++) {
				/* Grey value: RGB/BGR/alpha order all agree. */
				u8 v = (bits & (0x80 >> x)) ? 0xff : 0x00;

				for (sx = 0; sx < M2582_FONT_SCALE; sx++)
					for (b = 0; b < M2582_FB_BPP; b++)
						px[(x * M2582_FONT_SCALE + sx) *
						   M2582_FB_BPP + b] = v;
			}
		}
	}
	if (m2582_fb)
		m2582_fb_dirty(off, off + span);
	if (++m2582_con_x >= M2582_CON_COLS)
		m2582_fb_newline();
}

static void m2582_splash_con_write(struct console *co, const char *s,
				   unsigned int count)
{
	unsigned int i;

	if (!m2582_fb && !m2582_fb_early)
		return;
	for (i = 0; i < count; i++)
		m2582_fb_putc(s[i]);
	if (m2582_fb)
		m2582_fb_flush();
}

static struct console m2582_splash_console = {
	.name = "ttyM2582",
	.write = m2582_splash_con_write,
	.flags = CON_PRINTBUFFER | CON_ENABLED,
	.index = -1,
};

/*
 * Live USB clock/power register dump, exposed as /proc/m2582_usbdump so the
 * initramfs can print it as the very last thing on screen (a late_initcall
 * would be scrolled away by the rest of the diagnostics).
 *
 * The GCC register map for TUNA was reconstructed rather than taken from a
 * vendor driver, and the USB branch offsets match gcc-sm8650 byte for byte.
 * Writing the CBCR enable bit and reading it back tells us whether the USB30
 * core clock can actually be un-halted; if it cannot, that alone explains the
 * DWC3 soft-reset timeout.
 */

void __init meizu_m2582_splash_early(void)
{
	/*
	 * No beacon flash here any more. It used to paint the panel white, black
	 * and white again (440 ms total) to prove from the sofa that the kernel had
	 * reached setup_arch and the display was scanning. The console below works,
	 * so the flash is only a distracting black/white blink on every boot.
	 * Clear straight to the console background instead.
	 */
	m2582_fb_clear_all();
	m2582_con_x = 0;
	m2582_con_y = 0;
	m2582_dirty_lo = ~(size_t)0;
	m2582_dirty_hi = 0;
	m2582_row_map_fn = early_memremap;
	m2582_row_unmap_fn = early_memunmap;
	m2582_fb_early = true;
	m2582_console_up = true;
	/* CON_PRINTBUFFER replays everything logged since start_kernel(). */
	register_console(&m2582_splash_console);
}

/*
 * Right after paging_init(). Switch to the linear map when it covers the
 * carveout, because early ioremap is torn down later in setup_arch and a
 * mounted fixmap window would then dangle.
 */
void __init meizu_m2582_splash_console(void)
{
	if (!m2582_splash_size || M2582_FB_FRAME > m2582_splash_size)
		return;
	if (memblock_is_map_memory(m2582_splash_phys) &&
	    memblock_is_map_memory(m2582_splash_phys + M2582_FB_FRAME - 1)) {
		m2582_row_release();
		m2582_fb = (u8 *)__va(m2582_splash_phys);
		memset(m2582_fb, 0, M2582_FB_FRAME);
		dcache_clean_poc((unsigned long)m2582_fb,
				 (unsigned long)m2582_fb + M2582_FB_FRAME);
	} else {
		/* Stop using a mapping that early_ioremap_reset() will drop. */
		m2582_row_release();
		m2582_fb_early = false;
		m2582_row_map_fn = NULL;
		m2582_row_unmap_fn = NULL;
	}
}

/* Last resort for a no-map carveout: map it explicitly, then replay. */
static int __init m2582_splash_console_initcall(void)
{
	void *base;

	if (m2582_fb || !m2582_console_up || !m2582_splash_size ||
	    M2582_FB_FRAME > m2582_splash_size)
		return 0;
	base = memremap(m2582_splash_phys, M2582_FB_FRAME, MEMREMAP_WB);
	if (!base)
		base = ioremap(m2582_splash_phys, M2582_FB_FRAME);
	if (!base)
		return 0;
	m2582_fb = base;
	memset(m2582_fb, 0, M2582_FB_FRAME);
	dcache_clean_poc((unsigned long)m2582_fb,
			 (unsigned long)m2582_fb + M2582_FB_FRAME);
	m2582_con_x = 0;
	m2582_con_y = 0;
	register_console(&m2582_splash_console);
	return 0;
}
postcore_initcall(m2582_splash_console_initcall);


#ifdef CONFIG_ARM64_MEIZU_M2582_BUILTIN_DTB
/* Use the embedded mainline DT verbatim, without any firmware import. */
void * __init meizu_m2582_embedded_dtb(int *size)
{
	m2582_splash_locate(m2582_builtin_dtb);
	*size = fdt_totalsize(m2582_builtin_dtb);
	pr_info("M2582: embedded mainline DTB (verbatim, no firmware import), %d bytes\n",
		*size);
	return (void *)m2582_builtin_dtb;
}

#ifdef CONFIG_ARM64_MEIZU_M2582_DT_IMPORT
void * __init meizu_m2582_fixup_fdt(const void *firmware)
{
	int ret;

	m2582_splash_locate(firmware);
	meizu_m2582_marker_early();
	ret = meizu_m2582_prepare_fdt(firmware, m2582_builtin_dtb,
					m2582_fdt, sizeof(m2582_fdt));

	if (ret) {
		/*
		 * Do not hang here: with no console a rejected handoff is
		 * invisible. Flag it so the beacon shows a black screen and
		 * let the kernel continue on the firmware DT.
		 */
		pr_crit("M2582: embedded DTB import failed: %s\n", fdt_strerror(ret));
		meizu_m2582_fdt_import_failed = true;
		return NULL;
	}
	pr_info("M2582: embedded mainline DTB; imported firmware RAM/reservations\n");
	return m2582_fdt;
}
#endif /* CONFIG_ARM64_MEIZU_M2582_DT_IMPORT */
#endif /* CONFIG_ARM64_MEIZU_M2582_BUILTIN_DTB */
#endif
