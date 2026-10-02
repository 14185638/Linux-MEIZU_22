/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal stand-in for the Android Generic Kernel Image ABI padding header.
 * The vendor dwc3 sources only use ANDROID_KABI_RESERVE(), which exists purely
 * to keep ABI layout stable across GKI updates. Expanding it to nothing is
 * correct for a kernel that is not built as a GKI and never has to preserve
 * that ABI.
 */
#ifndef _LINUX_ANDROID_KABI_H
#define _LINUX_ANDROID_KABI_H

#define ANDROID_KABI_RESERVE(n)
#define ANDROID_KABI_USE(n, ...)
#define ANDROID_KABI_USE_2(n, ...)
#define ANDROID_KABI_REPLACE(_orig, _new)	_orig
#define ANDROID_KABI_RO_64(_a, _b, _c)		u64 _b
#define ANDROID_KABI_RO_32(_a, _b, _c)		u32 _b
#define ANDROID_KABI_RO_16(_a, _b, _c)		u16 _b
#define ANDROID_KABI_RO_8(_a, _b, _c)		u8 _b

#endif /* _LINUX_ANDROID_KABI_H */
