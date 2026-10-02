/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MEIZU_M2582_FDT_H
#define __MEIZU_M2582_FDT_H

int __init meizu_m2582_prepare_fdt(const void *firmware, const void *built_in,
				 void *out, int capacity);
#ifndef M2582_FDT_HOST_TEST
void * __init meizu_m2582_embedded_dtb(int *size);
void * __init meizu_m2582_fixup_fdt(const void *firmware);
void meizu_m2582_marker_write(u32 phase);
void __init meizu_m2582_marker_early(void);
/* Driver-free boot progress beacon painted on the bootloader splash buffer. */
extern bool meizu_m2582_fdt_import_failed;
void __init meizu_m2582_splash_early(void);
void __init meizu_m2582_splash_console(void);
#endif

#endif
