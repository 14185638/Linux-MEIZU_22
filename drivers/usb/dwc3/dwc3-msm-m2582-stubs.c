// SPDX-License-Identifier: GPL-2.0-only
/*
 * No-op replacements for the two entry points dwc3-msm-core.c calls that live
 * in the vendor's dwc3-msm-ops.c. That file is the Android kprobe-based USB
 * state reporter and is not built here, because it needs
 * CONFIG_ANDROID_USB_CONFIGFS_UEVENT, which mainline does not provide.
 */
#include <linux/kernel.h>

int dwc3_msm_kretprobe_init(void) { return 0; }
void dwc3_msm_kretprobe_exit(void) { }
